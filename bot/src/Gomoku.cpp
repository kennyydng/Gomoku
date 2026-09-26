
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

	//o << gomoku.player_info(gomoku.player())._stones;
	for (Pos const pos : Pos::all()) {
		o << gomoku.stone(pos) << (pos.x == (SIZE-1) ? "\n" : "─");
	}
//	for (int i = 0; i < 4; i++) {
//		for (Pos const pos : Pos::all())
//			o << (gomoku._info[0].fives[i].of(0)[pos] == 0 ? "0" : "1" ) << (pos.x == 18 ? "\n" : "─");
//			//o << (gomoku.LineStart[i][pos] == 0 ? "0" : "1" ) << (pos.x == 18 ? "\n" : "─");
//		o << std::endl;
//	}
	return o;
}

// Clés de Zobrist : une par (case, joueur), tirées une fois pour toutes.
// Le XOR étant involutif, poser puis retirer une pierre rend le hachage
// initial — ce qui rend le hachage indépendant de l'ordre des coups.
uint64_t Gomoku::zobristKey(Pos pos, bool player) {
	static struct keys_t {
		uint64_t of[SIZE][SIZE][2];
		constexpr keys_t() {
			uint64_t s = 0x9E3779B97F4A7C15ull;
			for (Take [x,y,p] : sugar::product(
				sugar::natural_index<SIZE>,
				sugar::natural_index<SIZE>,
				sugar::natural_index<2>
			)) {
				s ^= s << 13; s ^= s >> 7; s ^= s << 17;
				of[x][y][p] = s;
			}
		}
	} keys = {};
	return keys.of[pos.x][pos.y][player];
}

template<size_t AX>
INLINE score_t Gomoku::deltaAlongAxis(Pos pos, bool P) const {
	auto const line = Line<AX,5>(pos);

	score_t score{};

	Let [fives,... _] = std::get<AX>(_lines);
	fives[ P].score( compute(fives[!P].of(0) & line), score.upgrade_updater(P) );
	fives[!P].score( compute(fives[ P].of(0) & line), score.block_updater(P)   );

	return score;
}

template<size_t AX>
INLINE void Gomoku::placeAlongAxis(Pos pos, bool P) {
	auto const line5 = Line<AX,5>(pos);
	auto const line2 = Line<AX,2>(pos+AXES[AX]);
	auto const ends2 = compute( vec::reindex(Line<AX,4>(pos) ^ line2, line2.vindex()) );

	Var [
		fives,
		vulnerable,
		flanked
	] = std::get<AX>(_lines);

	fives[P]      += line5;
	vulnerable[P] += line2;
	flanked[!P]   += ends2;
}

template<size_t AX>
INLINE void Gomoku::unplaceAlongAxis(Pos pos, bool P) {
	auto const line5 = Line<AX,5>(pos);
	auto const line2 = Line<AX,2>(pos+AXES[AX]);
	auto const ends2 = compute( vec::reindex(Line<AX,4>(pos) ^ line2, line2.vindex()) );

	Var [
		fives,
		vulnerable,
		flanked
	] = std::get<AX>(_lines);

	fives[P]      -= line5;
	vulnerable[P] -= line2;
	flanked[!P]   -= ends2;
}

// Update incremental de score, utilisable independament de place/unplace
score_t Gomoku::moveDelta(Pos pos, bool P) const {
	contract_assert(pos.valid());
	contract_assert(stone(pos).empty());

	score_t tmp{};
	template for (constexpr size_t AX : index_of(AXES))
		tmp += deltaAlongAxis<AX>(pos, P);
	return tmp;
}

void Gomoku::place(Pos pos, bool P) {
	contract_assert(pos.valid());
	contract_assert(stone(pos).empty());

	_stones[P] += pos;
	_hash ^= zobristKey(pos, P);   // XOR involutif : symétrique d'un futur unplace()

	template for (constexpr size_t AX : index_of(AXES)) {
		_score += deltaAlongAxis<AX>(pos, P);
		placeAlongAxis<AX>(pos, P);
	}
}

// Inverse exact de place() — nommée unplace car `take` est une macro du DSL
// maison (macros.hpp). L'ordre compte : les masques dépendent de l'état
// courant, donc il faut d'abord défaire le comptage (unplaceAlongAxis) pour
// retrouver la configuration d'avant la pose, puis retrancher le delta.
void Gomoku::unplace(Pos pos, bool P) {
	contract_assert(pos.valid());
	contract_assert(!stone(pos).empty() && stone(pos).player() == P);

	_stones[P] -= pos;
	_hash ^= zobristKey(pos, P);

	template for (constexpr size_t AX : index_of(AXES)) {
		unplaceAlongAxis<AX>(pos, P);
		_score -= deltaAlongAxis<AX>(pos, P);
	}
}

// `pos` etant vide, les fenetres qui la couvrent gagneraient une pierre de P.
// Une fenetre ou P a deja `n` pierres et l'adversaire aucune en aurait n+1.
// On teste donc les fenetres a n pierres, vivantes, couvrant pos.
bool Gomoku::threatensAt(Pos pos, bool P, size_t n) const {
	bool found = false;
	template for (constexpr size_t AX : index_of(AXES)) {
		if (!found) {
			auto const line = Line<AX,5>(pos);

			Let [fives,... _] = std::get<AX>(_lines);
			if (vec::any( fives[P].of(n) & fives[!P].of(0) & line ))
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
	if (_rules.capture && captures(P) + wouldCapture(pos, P) >= 10)
		return true;
	// Une fenetre ou P a deja 4 pierres et l'adversaire aucune : la remplir
	// fait cinq.
	return threatensAt(pos, P, 4);
}

// Fenetres vivantes pour P (P y a n pierres, l'adversaire aucune), puis
// paires de departs consecutifs : voir Gomoku::Threats pour le pourquoi.
Gomoku::Threats Gomoku::threats(bool P) const {
	Threats t;
	template for (constexpr size_t AX : index_of(AXES)) {
		Let [fives,... _] = std::get<AX>(_lines);

		BitBoard<AX> const alive = fives[!P].of(0);
		BitBoard<AX> const a4{ fives[P].of(4) & alive };
		BitBoard<AX> const a3{ fives[P].of(3) & alive };
		int const n4 = vec::sum(vec::popcount( a4 & a4.shift(AXES[AX]) ));
		int const n3 = vec::sum(vec::popcount( a3 & a3.shift(AXES[AX]) ));
		t.open4 += n4;
		t.open3 += n3;
		t.axes  += (n4 + n3 > 0);
	}
	return t;
}

unsigned Gomoku::capturable(bool taker) const {
	if (!_rules.capture)
		return 0;

	unsigned n = 0;
	template for (constexpr auto AX : index_of(AXES)) {
		Let pairs      = std::get<AX>(_lines).vulnerable[!taker].of(2);
		Let vulnerable = std::get<AX>(_lines).flanked   [!taker].of(0);
		Let attacks    = std::get<AX>(_lines).flanked   [ taker].of(1);
		n += (unsigned)vec::sum(vec::popcount(
			pairs & (vulnerable & attacks)
		));
	}
	return n;
}

unsigned Gomoku::wouldCapture(Pos pos, bool taker) const {
	if (!_rules.capture)
		return 0;

	unsigned taken = 0;
	template for (constexpr auto AX : index_of(AXES)) {
		Let pairs      = std::get<AX>(_lines).vulnerable[!taker].of(2);
		Let vulnerable = std::get<AX>(_lines).flanked   [!taker].of(0);
		Let attacks    = std::get<AX>(_lines).flanked   [ taker].of(1);

		auto const line2 = Line<AX,2>(pos+AXES[AX]);
		auto const ends2 = compute( vec::reindex(Line<AX,4>(pos) ^ line2, line2.vindex()) );
		taken += (unsigned)vec::sum(vec::popcount(
			pairs & (vulnerable & attacks) & ends2
		));
	}
	return taken;
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
	while (back.valid() && _stones[P][back]) {
		p = back;
		back = p - dir;
	}
	return p;
}

Pos Gomoku::runEnd(Pos pos, Dir dir, bool P) const {
	Pos p = pos, fwd = p + dir;
	while (fwd.valid() && _stones[P][fwd]) {
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
	if (!pos.valid() || !stone(pos).empty())
		return false;

	Let pr = _rules.players[player];
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
			if (!_stones[P][pos1])
				continue;

			const bool f0Opp   = _stones[opp][flank0];
			const bool f0Empty = stone(flank0).empty();
			const bool f1Opp   = _stones[opp][flank1];
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
	if (_rules.capture && captures(player) >= 10) {
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
			if (!_stones[player][stepPos(_dStart, _dDir, i)])
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
		// Le sujet donne DEUX facons de survivre a un cinq adverse, et elles
		// ne se recouvrent pas.
		//
		// La premiere : casser la ligne en prenant une paire DEDANS.
		const bool breakable = _rules.captureUnperfect
			&& isUnperfect5(fStart, fEnd, fDir, player);

		// La seconde : compter. « If the player has already lost four pairs
		// and the opponent can capture one more, the opponent wins by
		// capture. » A huit pierres perdues, n'importe quelle paire prise
		// AILLEURS sur le plateau porte l'adversaire a dix — il gagne sans
		// toucher a l'alignement, qui peut etre parfaitement inattaquable.
		//
		// capturable() n'est evalue que sur le fil des huit pierres, donc
		// presque jamais : la condition de gauche court-circuite.
		const bool outcounted = _rules.captureUnperfect
			&& captures(!player) >= 8 && capturable(!player) > 0;

		if (breakable || outcounted) {
			// Meme mecanisme dans les deux cas, et c'est la troisieme puce du
			// sujet qui le dit : « If there is no possibility of this
			// happening, there is no need to continue the game. » S'il y a une
			// possibilite, on joue le coup de plus qui tranche. La resolution
			// est en tete de play() — l'adversaire qui atteint dix prises
			// gagne avant meme qu'on regarde si la ligne a tenu.
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
