
#include <cassert>
#include <iostream>

#include "Gomoku.class.hpp"

std::ostream &operator<<(std::ostream &o, Gomoku const &gomoku) {
	o << "Turn : " << gomoku.turn() << " " <<
		( gomoku.player() ? "(white to play)" : "(black to play)" );
//#if R_CAPTURE
//	o << Stone(Stone::WHITE) << gomoku.score(0) << "-" <<
//		Stone(Stone::BLACK) << gomoku.score(1);
//#endif
	o << std::endl;
	o << "Score: " << gomoku._score << std::endl;

	//o << gomoku.player_info(gomoku.player()).stones;
	for (Pos const pos : Pos::all()) {
		o << gomoku.stone(pos) << (pos.x == (SIZE-1) ? "\n" : "─");
	}
//	for (int i = 0; i < 4; i++) {
//		for (Pos const pos : Pos::all())
//			o << (gomoku._info[0].lines[i].of(0)[pos] == 0 ? "0" : "1" ) << (pos.x == 18 ? "\n" : "─");
//			//o << (gomoku.LineStart[i][pos] == 0 ? "0" : "1" ) << (pos.x == 18 ? "\n" : "─");
//		o << std::endl;
//	}
	return o;
}

void Gomoku::restrictToBoard() {
	template for (constexpr size_t AX : index_of(AXES)) {
		// Meme critere que LinesStart, avec la taille reelle du plateau.
		BitBoard<AX> const valid = makeLines<AX>(_rules.size);
		std::get<AX>(_info[0].lines).restrict(valid);
		std::get<AX>(_info[1].lines).restrict(valid);
	}
}

void Gomoku::place(Pos pos, bool P) {
	contract_assert(pos.valid());
	contract_assert(stone(pos).empty());
	auto &p0 = _info[ P];
	auto &p1 = _info[!P];
	p0.stones += pos;
	_hash ^= zobristKey(pos, P);   // XOR involutif : symétrique d'un futur take()
	template for (constexpr size_t AX : index_of(AXES)) {
		auto const line = Line<AX>(pos);
		auto &p0lines = std::get<AX>(p0.lines);
		auto &p1lines = std::get<AX>(p1.lines);
		p0lines.score( compute(p1lines.of(0) & line), _score.upgrade_updater(P) );
		// block_updater reçoit CELUI QUI BLOQUE, pas celui qui subit. Les
		// fenêtres adverses tuées doivent être retirées du compte de
		// l'adversaire, donc ajoutées au signe du bloqueur — et sign_P vaut
		// -sign_{!P}. Passer !P inversait le terme : bloquer une menace
		// dégradait l'évaluation de celui qui bloque, ce qui rendait la
		// défense invisible au tri des candidats et faisait annoncer des mats
		// forcés inexistants (voir tests/score_debug.cpp).
		p1lines.score( compute(p0lines.of(0) & line), _score.block_updater(P)   );
		p0lines += line;
	}
}

// Inverse exact de place() — nommée unplace car `take` est une macro du DSL
// maison (macros.hpp). L'ordre compte : les masques dépendent de l'état
// courant, donc il faut d'abord défaire le comptage (p0lines -= line) pour
// retrouver la configuration d'avant la pose, puis retrancher les mêmes termes.
//
// Aucun updater supplémentaire n'est nécessaire : sign_{!P} = -sign_P, donc
// passer !P donne exactement le terme opposé.
void Gomoku::unplace(Pos pos, bool P) {
	contract_assert(pos.valid());
	auto &p0 = _info[ P];
	auto &p1 = _info[!P];
	template for (constexpr size_t AX : index_of(AXES)) {
		auto const line = Line<AX>(pos);
		auto &p0lines = std::get<AX>(p0.lines);
		auto &p1lines = std::get<AX>(p1.lines);
		p0lines -= line;
		p1lines.score( compute(p0lines.of(0) & line), _score.block_updater(!P)   );
		p0lines.score( compute(p1lines.of(0) & line), _score.upgrade_updater(!P) );
	}
	p0.stones -= pos;
	_hash ^= zobristKey(pos, P);
}

// `pos` etant vide, les fenetres qui la couvrent gagneraient une pierre de P.
// Une fenetre ou P a deja `n` pierres et l'adversaire aucune en aurait n+1.
// On teste donc les fenetres a n pierres, vivantes, couvrant pos.
bool Gomoku::threatensAt(Pos pos, bool P, size_t n) const {
	auto const &p0 = _info[ P];
	auto const &p1 = _info[!P];
	bool found = false;
	template for (constexpr size_t AX : index_of(AXES)) {
		if (!found) {
			auto const line = Line<AX>(pos);
			auto const &p0lines = std::get<AX>(p0.lines);
			auto const &p1lines = std::get<AX>(p1.lines);
			if (vec::any( vec::compute(
					p0lines.of(n) & p1lines.of(0) & line) ))
				found = true;
		}
	}
	return found;
}

bool Gomoku::wouldThreatenFive(Pos pos, bool P) const {
	// TROIS, pas quatre : of(n) compte les pierres DEJA presentes. Poser une
	// pierre dans une fenetre ou P en a trois la porte a quatre, donc a une
	// menace de cinq. Tester of(4) reviendrait a chercher un coup qui gagne
	// tout de suite, pas un coup forcant — et le VCF ne trouverait rien.
	return threatensAt(pos, P, 3);
}

bool Gomoku::wouldWin(Pos pos, bool P) const {
	if (_rules.capture && _captures[P] + wouldCapture(pos, P) >= 10)
		return true;
	// Une fenetre ou P a deja 4 pierres et l'adversaire aucune : la remplir
	// fait cinq.
	return threatensAt(pos, P, 4);
}

// Fenetres vivantes pour P (P y a n pierres, l'adversaire aucune), puis
// paires de departs consecutifs : voir Gomoku::Threats pour le pourquoi.
Gomoku::Threats Gomoku::threats(bool P) const {
	auto const &p0 = _info[ P];
	auto const &p1 = _info[!P];
	Threats t;
	template for (constexpr size_t AX : index_of(AXES)) {
		auto const &p0lines = std::get<AX>(p0.lines);
		auto const &p1lines = std::get<AX>(p1.lines);
		BitBoard<AX> const alive = p1lines.of(0);
		BitBoard<AX> const a4{ p0lines.of(4) & alive };
		BitBoard<AX> const a3{ p0lines.of(3) & alive };
		// LAYOUT_AXIS et non AXES : sur les diagonales, les deux ne
		// designent pas la meme direction (voir Gomoku.class.hpp).
		int const n4 = vec::sum(vec::popcount( a4 & a4.shift(LAYOUT_AXIS<AX>) ));
		int const n3 = vec::sum(vec::popcount( a3 & a3.shift(LAYOUT_AXIS<AX>) ));
		t.open4 += n4;
		t.open3 += n3;
		t.axes  += (n4 + n3 > 0);
	}
	return t;
}

unsigned Gomoku::capturable(bool taker) const {
	if (!_rules.capture)
		return 0;
	BitBoard<0> const &T = _info[ taker].stones;
	BitBoard<0> const &V = _info[!taker].stones;
	BitBoard<0> const occupied{ T + V };
	// Complementer un vec creux n'a pas de sens (les mots hors index sont
	// inconnus, pas nuls) : on part du plateau plein et on retire.
	BitBoard<0> empty{true};
	empty -= occupied;

	unsigned n = 0;
	template for (constexpr auto d : DIRECTIONS) {
		BitBoard<0> const v1 = V.shift(d * -1);
		BitBoard<0> const v2 = V.shift(d * -2);
		BitBoard<0> const t3 = T.shift(d * -3);
		n += (unsigned)vec::sum(vec::popcount(
			empty & v1.get_words() & v2.get_words() & t3.get_words() ));
	}
	return n;
}

unsigned Gomoku::wouldCapture(Pos pos, bool P) const {
	if (!_rules.capture)
		return 0;
	unsigned taken = 0;
	template for (constexpr auto dir : DIRECTIONS) {
		const Pos p1 = pos + dir;
		const Pos p2 = p1  + dir;
		const Pos p3 = p2  + dir;
		if (p3.valid()
		 && !stone(p1).empty() && stone(p1).player() != P
		 && !stone(p2).empty() && stone(p2).player() != P
		 && !stone(p3).empty() && stone(p3).player() == P)
			taken += 2;
	}
	return taken;
}

// Les deux termes de place(), appliques a un score temporaire au lieu du
// score reel : aucun CountBoard n'est modifie, aucun etat n'est copie.
long Gomoku::moveDelta(Pos pos, bool P) const {
	score_t tmp{};
	auto const &p0 = _info[ P];
	auto const &p1 = _info[!P];
	template for (constexpr size_t AX : index_of(AXES)) {
		auto const line = Line<AX>(pos);
		auto const &p0lines = std::get<AX>(p0.lines);
		auto const &p1lines = std::get<AX>(p1.lines);
		p0lines.score( vec::compute(p1lines.of(0) & line), tmp.upgrade_updater(P) );
		p1lines.score( vec::compute(p0lines.of(0) & line), tmp.block_updater(P)   );
	}
	return tmp.raw();
}

// --- Detection de menaces et legalite -------------------------------------
//
// Portage depuis la branche main. Le code y a ete audite et corrige (deux
// bugs : alignements comptes deux fois selon la direction, et confusion
// overline/O4/C4 apres retrait du flanquement). Le reecrire en bitboards
// aurait refait les memes erreurs pour un gain nul — il ne tourne qu'a la
// racine, une fois par coup reellement joue.

Pos Gomoku::runStart(Pos pos, Dir dir, bool P) const {
	Pos p = pos, back = p - dir;
	while (back.valid() && _info[P].stones[back]) {
		p = back;
		back = p - dir;
	}
	return p;
}

Pos Gomoku::runEnd(Pos pos, Dir dir, bool P) const {
	Pos p = pos, fwd = p + dir;
	while (fwd.valid() && _info[P].stones[fwd]) {
		p = fwd;
		fwd = p + dir;
	}
	return p;
}

// Suppose que `pos` porte deja la pierre de `player` (comme getThreats).
// `min` borne la recursion : inutile de tester l'extension d'un alignement
// deja trop court pour produire la menace cherchee.
Threat Gomoku::threatAt(Pos pos, Dir dir, bool player, int min) {
	const Pos start = runStart(pos, dir, player);
	const Pos end   = runEnd  (pos, dir, player);
	const int len   = runLenOf(start, end, dir);

	const Pos beforeStart = start - dir;
	const Pos afterEnd    = end   + dir;

	// `Overline` n'est ici qu'un constat geometrique (six ou plus) : c'est a
	// isLegalMove et a play() de decider, independamment, si cela gagne
	// (overlineWins) et/ou si le coup est interdit (overlineForbidden).
	if (len > 5)
		return {ThreatType::Overline, start, end, dir};
	if (len == 5)
		return {ThreatType::Five, start, end, dir};
	if (min >= 5)
		return {};

	const Threat ext0 = threatOf(beforeStart, dir, player, min + 1);
	const Threat ext1 = threatOf(afterEnd,    dir, player, min + 1);
	const bool is5_0 = ext0.type == ThreatType::Five;
	const bool is5_1 = ext1.type == ThreatType::Five;

	if (is5_0 || is5_1) {
		const Pos line4start = is5_0 ? ext0.start : start;
		const Pos line4end   = is5_1 ? ext1.end   : end;
		if (is5_0 && is5_1)
			return {len == 4 ? ThreatType::O4 : ThreatType::FourFour,
				line4start, line4end, dir};
		return {ThreatType::C4, start, end, dir};
	}
	if (min >= 4)
		return {};

	const bool isO4_0 = ext0.type == ThreatType::O4;
	const bool isO4_1 = ext1.type == ThreatType::O4;
	if (isO4_0 || isO4_1) {
		const Pos line3start = isO4_0 ? ext0.start : start;
		const Pos line3end   = isO4_1 ? ext1.end   : end;
		return {ThreatType::O3, line3start, line3end, dir};
	}

	return {};
}

Threat Gomoku::threatOf(Pos pos, Dir dir, bool player, int min) {
	if (!pos.valid() || !stone(pos).empty())
		return {};
	rawPlace(pos, player);
	Threat t = threatAt(pos, dir, player, min);
	rawRemove(pos, player);
	return t;
}

std::vector<Threat> Gomoku::getThreats(Pos pos, bool player, int min) {
	std::vector<Threat> threats;
	// AXES et non DIRECTIONS : un alignement et son oppose sont le MEME
	// alignement. Le compter deux fois transformerait tout trois simple en
	// double-trois — c'est exactement le bug corrige sur main.
	for (Dir const &dir : AXES) {
		Threat t = threatAt(pos, dir, player, min);
		if (t.type != ThreatType::None)
			threats.push_back(t);
	}
	return threats;
}

bool Gomoku::isLegalMove(Pos pos, bool player) {
	if (!pos.valid() || !onBoard(pos) || !stone(pos).empty())
		return false;

	auto const &pr = _rules.players[player];
	if (!_rules.restricted(player))
		return true;

	rawPlace(pos, player);
	std::vector<Threat> threats = getThreats(pos, player, 3);
	rawRemove(pos, player);

	bool winning = false;
	for (Threat const &t : threats) {
		if (t.type == ThreatType::Five) { winning = true; break; }
		if (t.type == ThreatType::Overline && pr.overlineWins) { winning = true; break; }
	}

	// Un coup qui gagne ou qui capture echappe aux formes interdites — meme
	// regle que le frontend et que main.
	if (winning || (_rules.capture && wouldCapture(pos, player)))
		return true;

	if (pr.overlineForbidden)
		for (Threat const &t : threats)
			if (t.type == ThreatType::Overline)
				return false;

	if (pr.fourFour) {
		int fours = 0;
		bool doubled = false;
		for (Threat const &t : threats) {
			if (t.type == ThreatType::O4 || t.type == ThreatType::C4)
				fours++;
			if (t.type == ThreatType::FourFour)
				doubled = true;
		}
		if (fours > 1 || doubled)
			return false;
	}

	if (pr.threeThree) {
		int threes = 0;
		for (Threat const &t : threats)
			if (t.type == ThreatType::O3)
				threes++;
		if (threes > 1)
			return false;
	}

	return true;
}

void Gomoku::pass() {
	_turn++;
}

// Portage depuis main. Une pierre du cinq est prenable si elle forme une
// paire avec une voisine et que les deux flancs sont l'un adverse, l'autre
// vide — le motif de capture, vu depuis la paire menacee.
bool Gomoku::isUnperfect5(Pos start, Pos end, Dir dir, bool P) const {
	const bool opp = !P;
	const int len = runLenOf(start, end, dir);

	for (int i = 0; i < len; i++) {
		const Pos pos = stepPos(start, dir, i);
		for (Dir const &delta : DIRECTIONS) {
			const Pos flank0 = pos - delta;
			const Pos pos1   = pos + delta;
			const Pos flank1 = pos + delta * 2;

			if (!flank0.valid() || !flank1.valid())
				continue;
			if (!_info[P].stones[pos1])
				continue;

			const bool f0Opp   = _info[opp].stones[flank0];
			const bool f0Empty = stone(flank0).empty();
			const bool f1Opp   = _info[opp].stones[flank1];
			const bool f1Empty = stone(flank1).empty();

			if ((f0Opp && f1Empty) || (f1Opp && f0Empty))
				return true;
		}
	}
	return false;
}

void Gomoku::play(Pos pos) {
	const bool player = this->player();
	const bool wasDelayed = _delayed;

	place(pos, player);

	// Capture : le motif est moi, adversaire, adversaire, moi. On teste les 8
	// directions autour de la pierre posée. Le test case par case est direct
	// et vérifiable ; la version bitboard (adv.shift(-d) & adv.shift(-2d) &
	// moi.shift(-3d) donne toutes les cases de capture du plateau d'un coup)
	// vaudra le coup pour noter les candidats, pas pour un seul coup.
	if (_rules.capture) {
		template for (constexpr auto dir : DIRECTIONS) {
			const Pos p1 = pos + dir;
			const Pos p2 = p1  + dir;
			const Pos p3 = p2  + dir;
			if (p3.valid()
			 && !stone(p1).empty() && stone(p1).player() != player
			 && !stone(p2).empty() && stone(p2).player() != player
			 && !stone(p3).empty() && stone(p3).player() == player) {
				unplace(p1, !player);
				unplace(p2, !player);
				_captures[player] += 2;
			}
		}
	}

	_turn++;

	if (_resolved >= 0)
		return;

	// 1. Victoire par capture : cinq paires prises.
	if (_rules.capture && _captures[player] >= 10) {
		_resolved = player;
		return;
	}

	// 2. Un cinq etait en attente : ce coup etait la seule chance de le
	//    casser. Intact => la victoire est acquise ; casse => oubliee.
	if (wasDelayed) {
		_delayed = false;
		const int len = runLenOf(_dStart, _dEnd, _dDir);
		bool intact = true;
		for (int i = 0; i < len && intact; i++)
			if (!_info[_dPlayer].stones[stepPos(_dStart, _dDir, i)])
				intact = false;
		if (intact) {
			_resolved = _dPlayer;
			return;
		}
	}

	// 3. Ce coup cree-t-il un cinq (ou un overline) ? Marche scalaire sur les
	//    quatre axes, bornee par la longueur de l'alignement : moins cher que
	//    l'ancien balayage de huit CountBoard, et surtout capable de faire la
	//    difference entre cinq exactement et six ou plus.
	//
	//    La ligne n'est retenue que pour le CINQ : c'est elle, et pas celle
	//    d'un eventuel overline sur un autre axe, qui devra survivre au coup
	//    adverse.
	bool five = false, overline = false;
	Pos fStart{}, fEnd{};
	Dir fDir{};
	for (Dir const &d : AXES) {
		const Pos s = runStart(pos, d, player);
		const Pos e = runEnd  (pos, d, player);
		const int len = runLenOf(s, e, d);
		if (len > 5) {
			overline = true;
		} else if (len == 5) {
			five = true;
			fStart = s; fEnd = e; fDir = d;
		}
	}

	if (overline && _rules.players[player].overlineWins) {
		_resolved = player;
		return;
	}
	if (five) {
		if (_rules.captureUnperfect && isUnperfect5(fStart, fEnd, fDir, player)) {
			_delayed  = true;
			_dStart   = fStart;
			_dEnd     = fEnd;
			_dDir     = fDir;
			_dPlayer  = player;
		} else {
			_resolved = player;
		}
	}
}
