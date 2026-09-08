
#pragma once

#include <iostream>
#include <cstdint>
#include <vector>
#include <string>
#include <stdexcept>

#include "sugar.hpp"
#include "parsing_utils.hpp"
#include "gomoku_types.hpp"
#include "BitBoard.class.hpp"

using namespace sugar;

class score_t {
	long value;

	static constexpr long line_values[6] = {0, 1, 5, 50, 1000, 50000};
public:
	constexpr auto upgrade_updater(bool P)
		TO( [&,sign = P?1:-1]<size_t I>(unsigned long delta) {
			if constexpr (I == 5) return;
			else {
				value += sign * delta * (line_values[I+1] - line_values[I]);
			}
		} )

	// P = le joueur QUI BLOQUE. Les fenêtres adverses devenues mortes sortent
	// du compte de l'adversaire, ce qui profite à P.
	constexpr auto block_updater(bool P)
		TO( [&,sign = P?1:-1]<size_t I>(unsigned long delta) {
			if constexpr (I == 0) return;
			else {
				value += sign * delta * line_values[I];
			}
		} )

	constexpr std::strong_ordering operator<=>(this score_t const &lhs, score_t const &rhs)
		{ return lhs.value <=> rhs.value; }

	// Alpha-bêta compare des bornes numériques, un <=> ne suffit pas.
	// Convention : les updaters utilisent sign = P?1:-1, donc une valeur
	// POSITIVE favorise le joueur 1. C'est à l'appelant de la retourner pour
	// obtenir le point de vue du joueur au trait (voir EngineState::evaluate).
	constexpr long raw() const
		{ return value; }

	friend std::ostream &operator<<(std::ostream &o, score_t const &score)
		{ return o << "{" << score.value << "}"; }
};

struct Move {
	Pos pos;
};

// Règles lues à l'exécution plutôt que fixées à la compilation : le protocole
// en envoie une ligne, et l'interface web les laisse configurer. Un binaire
// par jeu de règles ne serait pas compatible avec ça.
//
// Même modèle que la branche main, y compris les règles asymétriques : le
// caractère d'overline a quatre états et certaines restrictions ne visent que
// noir (Renju). Les deux moteurs doivent lire la MÊME ligne de la même
// façon, sinon ils ne jouent pas au même jeu — et aucune comparaison entre
// eux ne veut plus rien dire.
struct Rules {
	// Le plateau logique. Le stockage reste dimensionné pour 19x19 — les
	// BitBoard sont des types paramétrés à la compilation — mais un plateau
	// plus petit est joué correctement : les cases hors limites ne sont
	// jamais candidates, et les fenêtres qui en débordent sortent du
	// comptage (Gomoku::restrictToBoard).
	pos_t size = SIZE;

	bool capture = false;
	bool captureUnperfect = false;

	struct PlayerRules {
		bool overlineWins = true;
		bool overlineForbidden = false;
		bool threeThree = false;
		bool fourFour = false;
	} players[2] = {};

	Rules() = default;

	// Six caractères : taille, capture, capture imparfaite, overline,
	// double-trois, double-quatre.
	explicit Rules(std::string const &str) {
			if (str.size() != 6)
				throw std::runtime_error("Invalid rules");
			if      (str[0] == '5') size = 15;
			else if (str[0] == '9') size = 19;
			else throw std::runtime_error("Invalid grid size");

			capture          = (str[1] == '1');
			captureUnperfect = (str[2] == '1');

			// Overline à quatre états sur un seul caractère : '1' gagne pour
			// les deux, '0' légal mais ne gagne pas, 'f' interdit aux deux,
			// 'b' interdit à noir seulement et gagnant pour blanc (Renju).
			const char overline = str[3];
			players[0].overlineWins      = (overline == '1');
			players[0].overlineForbidden = (overline == 'f' || overline == 'b');
			players[1].overlineWins      = (overline == '1' || overline == 'b');
			players[1].overlineForbidden = (overline == 'f');

			// 'b' ne s'applique qu'à noir (players[0]).
			players[0].threeThree = (str[4] == '1' || str[4] == 'b');
			players[1].threeThree = (str[4] == '1');
			players[0].fourFour   = (str[5] == '1' || str[5] == 'b');
			players[1].fourFour   = (str[5] == '1');
		}

	// Vrai si `p` a au moins une forme interdite : permet à isLegalMove de
	// sortir immédiatement dans le cas courant, sans rien calculer.
	bool restricted(bool p) const {
			return players[p].threeThree || players[p].fourFour
				|| players[p].overlineForbidden;
		}
};

// Classification d'un alignement passant par une case. Reprise de main, où
// elle a été auditée et corrigée : les alignements y étaient comptés deux
// fois (huit directions au lieu de quatre axes), et overline/O4/C4 étaient
// confondus après le retrait du flanquement.
enum class ThreatType : std::uint8_t { None, O3, C4, FourFour, O4, Five, Overline };

struct Threat {
	ThreatType type = ThreatType::None;
	Pos start{};
	Pos end{};
	Dir dir{};
};

// L'ensemble des fenêtres de cinq qui tiennent sur un plateau n x n, dans
// l'orientation AX.
//
// CONVENTION, vérifiée expérimentalement et non déduite : make_line(pos)
// couvre les indices pos..pos+4, donc une pierre en `pos` compte dans les
// fenêtres d'indices pos..pos+4 — autrement dit la fenêtre d'indice `i`
// couvre les cases i-4a..i, et elle est repérée par sa DERNIÈRE case.
//
// Construit case par case plutôt que par décalage du plateau plein. Un
// `shift` de ±4 donne le bon ensemble sur les lignes et les colonnes, mais
// pas sur les diagonales : la représentation diagonale enroule les lignes, et
// le décalage fabrique des fenêtres qui traversent un bord. Mesuré : la case
// (0,0), dont l'antidiagonale ne fait qu'une case de long, se voyait
// attribuer cinq fenêtres. Ici le critère est explicite — une fenêtre est
// valide si ses DEUX extrémités sont sur le plateau, les cases intermédiaires
// suivant puisque l'axe est une droite. Voir tests/edge_test.cpp.
// La direction que traite REELLEMENT la disposition d'indice AX.
//
// Le tableau AXES et les BitBoard ne s'accordent pas sur les diagonales : la
// disposition d'indice 2 traite (1,-1) et celle d'indice 3 traite (1,1), soit
// l'inverse de AXES[2] et AXES[3]. C'est la transformation diagonale qui le
// veut — pour AX=2 elle envoie (x,y) sur (x, y+x), et ce sont les cases de
// l'ANTIdiagonale qui s'y retrouvent alignées sur une même ligne de bits.
//
// Vérifié expérimentalement, pas déduit : deux pierres adjacentes selon
// (1,1) font apparaître une fenêtre à deux pierres sur l'axe 3, pas l'axe 2.
// Confondre les deux fait compter les fenêtres diagonales dans la mauvaise
// direction, ce qui ne se voit qu'au bord — au milieu du plateau les deux
// diagonales se ressemblent trop.
template<size_t AX>
constexpr Dir LAYOUT_AXIS = AXES[AX < 2 ? AX : 5 - AX];

template<size_t AX>
constexpr BitBoard<AX> makeLines(pos_t n) {
	BitBoard<AX> lines{};
	for (pos_t y = 0; y < n; y++)
		for (pos_t x = 0; x < n; x++) {
			const pos_t sx = x - LAYOUT_AXIS<AX>.x * 4;
			const pos_t sy = y - LAYOUT_AXIS<AX>.y * 4;
			if (unsigned(sx) < unsigned(n) && unsigned(sy) < unsigned(n))
				lines += BitBoard<AX>(Pos{x, y});
		}
	return lines;
}

class Gomoku {
public:
	Gomoku() = default;
	explicit Gomoku(Rules rules): _rules(rules) {
			if (_rules.size < SIZE)
				restrictToBoard();
		}

	// Une case est-elle sur le plateau LOGIQUE ? Pos::valid() ne connaît que
	// la grille de stockage (19x19) ; sur un plateau 15x15, les quatre
	// dernières lignes et colonnes existent en mémoire mais pas dans le jeu.
	bool onBoard(Pos pos) const {
			return unsigned(pos.x) < unsigned(_rules.size)
			    && unsigned(pos.y) < unsigned(_rules.size);
		}
	Gomoku(Gomoku const &) = default;
	~Gomoku() {}

	// Un compteur plutôt qu'un vector : la recherche copie l'état à chaque
	// nœud (mesuré plus rapide que défaire un coup), et seul le NOMBRE de
	// coups était jamais lu. Le vector imposait une allocation tas par nœud ;
	// avec un compteur, Gomoku redevient un POD copiable par memcpy.
	unsigned turn() const
		{ return _turn; }

	Rules const &rules() const
		{ return _rules; }
	unsigned captures(bool p) const
		{ return _captures[p]; }
	bool player() const
		{ return turn() % 2; }
	score_t const &heuristic() const
		{ return _score; }

	// L'issue est décidée par play(), qui est de toute façon appelé pour
	// chaque coup : la relire ici coûte une comparaison au lieu de rebalayer
	// huit CountBoard à chaque nœud. Surtout, c'est la SEULE façon de gérer la
	// victoire différée — « ce cinq gagne-t-il ? » dépend de l'historique, pas
	// de la seule position.
	bool is_over() const
		{ return _resolved >= 0; }

	auto const &player_info(bool p) const
		{ return _info[p]; }

	// Qui a gagné, ou -1 si la partie continue. La recherche a besoin du QUI
	// et pas seulement du SI : sans lui elle ne distingue pas un mat gagnant
	// d'un mat perdant, et foncerait vers sa propre défaite en croyant gagner.
	int winner() const
		{ return _resolved; }

	// Variation de score qu'entraînerait le coup `pos` pour `P`, calculée SANS
	// jouer le coup ni copier l'état : ce sont exactement les deux termes de
	// place(), appliqués à un score temporaire. Définie hors-ligne car elle
	// utilise Line<AX>, dont le type de retour est déduit et n'est connu
	// qu'après sa définition plus bas dans la classe.
	//
	// C'est ce qui rend le tri des candidats abordable. Le faire en jouant
	// réellement chaque coup — une copie complète de l'état (3232 octets) et
	// quatre mises à jour de CountBoard, 21 à 26 fois par nœud — divisait le
	// débit par 2.05 : mesuré 81664 nœuds contre 167296 sans le tri, là où le
	// moteur scalaire de main en fait 125184.
	long moveDelta(Pos pos, bool P) const;

	// Nombre de pierres que `pos` capturerait pour `P` (0, 2, 4...). Sert au
	// tri des candidats : moveDelta ne chiffre que les alignements, or une
	// capture vaut par elle-même — elle rapproche des 10 pierres qui gagnent.
	unsigned wouldCapture(Pos pos, bool P) const;

	// Le coup `pos` créerait-il une menace de cinq pour `P` ? Autrement dit :
	// existe-t-il, parmi les fenêtres couvrant `pos`, une fenêtre où P aurait
	// 4 pierres et l'adversaire aucune ? C'est la définition d'un coup
	// forçant, et elle se lit directement sur les CountBoard — sans jouer le
	// coup ni parcourir d'alignement.
	bool wouldThreatenFive(Pos pos, bool P) const;

	// Le coup `pos` gagnerait-il immédiatement pour `P` (cinq, ou 10 pierres
	// capturées) ?
	bool wouldWin(Pos pos, bool P) const;

	// Menaces fortes, lues directement sur les CountBoard.
	//
	// Le score par fenêtres est LINÉAIRE en l'ouverture : un four ouvert
	// (.XXXX.) vaut 2 fenêtres vivantes, un four bloqué (OXXXX.) en vaut 1 —
	// soit 2000 contre 1000, alors que le premier gagne la partie et que le
	// second se pare d'un coup.
	//
	// Caractérisation exacte, et gratuite dans cette représentation : un four
	// est OUVERT si deux fenêtres vivantes CONSÉCUTIVES contiennent chacune 4
	// pierres. `.XXXX.` donne les départs s et s+1 ; `OXXXX.` n'en donne qu'un
	// (l'autre contient la pierre adverse) ; `XX.XX` aussi, et c'est correct —
	// le trou en est l'unique complétion. Même construction à 3 pierres pour
	// le three ouvert, y compris cassé : `.X.XX.` tombe sans code dédié.
	struct Threats {
		int open4 = 0;   // paires de fenêtres-à-4 vivantes consécutives
		int open3 = 0;   // paires de fenêtres-à-3 vivantes consécutives
		int axes  = 0;   // axes portant au moins une de ces menaces (fourche)
	};
	Threats threats(bool P) const;

	// Nombre de paires que `taker` pourrait capturer MAINTENANT.
	//
	// Terme purement statique, et c'est là tout son intérêt : une paire
	// exposée coûte un coup à l'adversaire, donc elle se paie souvent au-delà
	// de l'horizon de la recherche. Sans ce terme, une feuille où mes pierres
	// sont à prendre s'évalue comme une feuille saine.
	//
	// Motif de capture : VIDE, V, V, T dans une direction. Balayage bitboard
	// du plateau entier, trois décalages par direction — c'est le poste le
	// plus cher de l'évaluation (11% du débit) pour un gain mesuré nul :
	// voir W_CAPTURE dans engine_state.hpp.
	unsigned capturable(bool taker) const;

	// --- Légalité (double-trois, double-quatre, overline interdit) ---------
	//
	// Volontairement SCALAIRE : ne tourne qu'à la racine, une fois par coup
	// réellement joué. La revérifier à chaque nœud coûterait une détection de
	// menaces récursive par candidat, des centaines de milliers de fois — et
	// n'apporterait rien, puisque la recherche ne fait qu'estimer. Seul le
	// coup RENDU doit être légal.
	bool isLegalMove(Pos pos, bool player);
	std::vector<Threat> getThreats(Pos pos, bool player, int min);
	Pos runStart(Pos pos, Dir dir, bool player) const;
	Pos runEnd(Pos pos, Dir dir, bool player) const;

	// Un cinq est « imparfait » si l'une de ses pierres appartient à une paire
	// capturable : l'adversaire peut le casser au coup suivant. Avec la règle
	// captureUnperfect, un tel cinq ne gagne pas tout de suite — il gagne s'il
	// SURVIT au coup adverse.
	bool isUnperfect5(Pos start, Pos end, Dir dir, bool player) const;

	// Hachage de Zobrist, maintenu par place(). La table de transposition en
	// dépend : deux ordres de coups menant à la même position doivent donner
	// le même hachage, sinon la table ne sert à rien.
	// Le trait fait partie de la position : deux plateaux identiques avec des
	// joueurs différents au trait n'ont pas la même valeur.
	std::uint64_t hash() const {
			std::uint64_t h = _hash;
			if (player())  h ^= 0xD6E8FEB86659FD93ull;
			// Un cinq en attente fait partie de l'état : deux plateaux
			// identiques, l'un avec une victoire à confirmer et l'autre sans,
			// n'ont pas la même valeur. Sans ce bit, la table de transposition
			// les confondrait.
			if (_delayed) h ^= 0x2545F4914F6CDD1Dull;
			return h;
		}

//#if R_CAPTURE
//	unsigned score(unsigned player) const
//		{ return captures[player]; };
//#endif

	Stone stone(Pos pos) const
		{ return {player_info(0).stones[pos], player_info(1).stones[pos]}; }

	template<class F>
	auto with_move(this Gomoku copy, std::optional<Pos> move, F &&f) {
			if (move) 	copy.play(*move);
			else      	copy.pass();
			auto ret = f(copy);
			return ret;
		}

	void play(Pos);
	void pass();

private:
	// Retire du comptage toute fenêtre qui déborde du plateau logique. Sans
	// ça, une fenêtre de cinq débordant à droite ou en bas serait comptée
	// comme vivante alors qu'elle ne peut jamais être complétée : le moteur
	// surévaluerait les bords et s'y collerait.
	void restrictToBoard();

	void place(Pos, bool);
	void unplace(Pos, bool);
	bool threatensAt(Pos, bool, size_t) const;

	// Pose/retire une pierre SANS toucher au score ni au hachage : la
	// détection de menaces n'a besoin que de la géométrie, et place() ferait
	// payer quatre mises à jour de CountBoard pour rien.
	void rawPlace (Pos pos, bool p) { _info[p].stones += pos; }
	void rawRemove(Pos pos, bool p) { _info[p].stones -= pos; }

	Threat threatAt(Pos pos, Dir dir, bool player, int min);
	Threat threatOf(Pos pos, Dir dir, bool player, int min);

	template<size_t AX>
	static constexpr auto Line(Pos pos)
		{ return BitBoard<AX>::make_line(pos,5); }

	template<size_t AX>
	static constexpr BitBoard<AX> LinesStart = makeLines<AX>(SIZE);

	struct PlayerInfo {
		BitBoard<0> stones = {};

		std::tuple<CountBoard<0,6>, CountBoard<1,6>, CountBoard<2,6>, CountBoard<3,6>>
			lines = { LinesStart<0>, LinesStart<1>, LinesStart<2>, LinesStart<3> };
//#if R_CAPTURE && R_CAPTURE_UNPERFECT
//		BitBoard lines5[4] = {};
//#endif
//
//		//BitBoard closed[2][1][4];
//
//#if R_CAPTURE
//		unsigned captures = {0,0};
//		BitBoard vulnerable;
//#endif
	} _info[2] = {};

	unsigned _turn = 0;
	unsigned _captures[2] = {0, 0};

	// Issue de la partie, fixée une fois pour toutes par play(). -1 = en
	// cours ; une partie gagnée le reste.
	int _resolved = -1;

	// Cinq en attente de confirmation (captureUnperfect) : l'adversaire a un
	// coup pour le casser ; s'il n'y parvient pas, la victoire est acquise.
	bool _delayed = false;
	Pos  _dStart{}, _dEnd{};
	Dir  _dDir{};
	bool _dPlayer = false;
	Rules _rules = {};
	score_t _score = score_t();
	std::uint64_t _hash = 0;

	// Clés de Zobrist : une par (case, joueur), tirées une fois pour toutes.
	// Le XOR étant involutif, poser puis retirer une pierre rend le hachage
	// initial — ce qui rend le hachage indépendant de l'ordre des coups.
	static std::uint64_t const &zobristKey(Pos pos, bool player) {
			static std::uint64_t keys[SIZE*SIZE][2] = {};
			static bool init = [](){
				std::uint64_t s = 0x9E3779B97F4A7C15ull;
				for (auto &cell : keys)
					for (auto &k : cell) {
						s ^= s << 13; s ^= s >> 7; s ^= s << 17;
						k = s;
					}
				return true;
			}();
			(void)init;
			return keys[pos.y * SIZE + pos.x][player];
		}

	friend std::ostream &operator<<(std::ostream &o, Gomoku const &gomoku);
	friend BitBoard<0> candidates(Gomoku &state);
};
