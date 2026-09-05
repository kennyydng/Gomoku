
#include "Gomoku.class.hpp"
#include <algorithm>
#include <chrono>
#include <vector>

// --- Génération de coups -----------------------------------------------
//
// Candidats limités aux cases vides à CANDIDATE_RADIUS d'une pierre existante
// (borne le facteur de branchement sur un plateau surtout vide), filtrés par
// légalité (isLegalMove, seul contrôle récursif/coûteux), triés par un score
// de motif non récursif : ce que le coup crée pour soi + ce qu'il bloque
// chez l'adversaire.

static constexpr int CANDIDATE_RADIUS = 1;
// Réduit de 8 à 6 puis à 5 : les branches en plus changeaient peu la
// qualité des coups mais faisaient exploser le temps en milieu de partie
// (voir MIN_DEPTH plus bas : le palier 10 exigé par le sujet a droit à un
// plafond de temps élargi, mais pas illimité — réduire le branchement est
// le principal levier pour l'atteindre sans exploser le budget moyen).
static constexpr size_t MAX_BRANCH = 5;
static constexpr double BLOCK_FACTOR = 0.8;

// Boîtes englobantes (élargies de CANDIDATE_RADIUS) des groupes de pierres
// jouées depuis le début de la recherche. Une seule box globale gaspille le
// scan dès que deux échanges se jouent dans des coins opposés du plateau
// (toute la zone vide entre les deux serait incluse) : on garde donc
// plusieurs rectangles disjoints, un par groupe de pierres proches, fusionnés
// dès qu'ils se recouvrent. Ne font que grandir/fusionner, jamais rétrécir :
// pas cher à maintenir, et un coup hors de toute box est par construction
// trop loin de toute pierre pour être un candidat valable. Restreindre le
// scan à ces boxes transforme generateCandidates() d'un O(taille²) en
// O(somme des aires), ce qui compte vu que ça tourne à chaque nœud.
struct Box { int8_t minX, maxX, minY, maxY; };
static std::vector<Box> searchBoxes;

static bool overlaps(const Box &a, const Box &b) {
	return a.minX <= b.maxX && b.minX <= a.maxX
	    && a.minY <= b.maxY && b.minY <= a.maxY;
}

static void expandBox(Pos p) {
	int8_t lo = 0, hi = (int8_t)current_board_size() - 1;
	Box merged {
		std::max(lo, (int8_t)(p.x - CANDIDATE_RADIUS)),
		std::min(hi, (int8_t)(p.x + CANDIDATE_RADIUS)),
		std::max(lo, (int8_t)(p.y - CANDIDATE_RADIUS)),
		std::min(hi, (int8_t)(p.y + CANDIDATE_RADIUS)),
	};

	for (size_t i = 0; i < searchBoxes.size();) {
		if (!overlaps(searchBoxes[i], merged)) {
			i++;
			continue;
		}
		merged.minX = std::min(merged.minX, searchBoxes[i].minX);
		merged.maxX = std::max(merged.maxX, searchBoxes[i].maxX);
		merged.minY = std::min(merged.minY, searchBoxes[i].minY);
		merged.maxY = std::max(merged.maxY, searchBoxes[i].maxY);
		searchBoxes.erase(searchBoxes.begin() + (long)i);
	}
	searchBoxes.push_back(merged);
}

// --- Score de motif ------------------------------------------------------
//
// Classe un alignement par sa longueur + son nombre d'extrémités ouvertes,
// via une marche directe (non récursive) avec runStart/runEnd. Sert à
// l'heuristique de feuille (evaluate(), plateau entier) et, bien plus
// souvent, au tri des coups candidats (une seule ligne par case) : pas de
// récursion, contrairement à la détection de menace dans isLegalMove().

static constexpr int W_FIVE       = 100000;
static constexpr int W_OPEN_FOUR  = 15000;
static constexpr int W_FOUR       = 2000;
static constexpr int W_OPEN_THREE = 800;
static constexpr int W_THREE      = 150;
static constexpr int W_OPEN_TWO   = 40;
static constexpr int W_TWO        = 10;
static constexpr int W_CAPTURE    = 400;
static constexpr int W_FORK       = 5000;

static int patternWeight(int len, int openEnds) {
	if (len >= 5)
		return W_FIVE;
	if (len == 4)
		return openEnds == 2 ? W_OPEN_FOUR : openEnds == 1 ? W_FOUR : 0;
	if (len == 3)
		return openEnds == 2 ? W_OPEN_THREE : openEnds == 1 ? W_THREE : 0;
	if (len == 2)
		return openEnds == 2 ? W_OPEN_TWO : openEnds == 1 ? W_TWO : 0;
	return 0;
}

// Cases vides consécutives à partir de `from` (inclus) en avançant selon
// `dir`, plafonné à `cap` (inutile d'aller plus loin que ce qu'il faut pour
// juger si un five tient encore dans la direction).
static int emptyRun(Gomoku &state, Pos from, Pos dir, int cap) {
	int n = 0;
	Pos p = from;
	while (n < cap && p.valid() && state.stone(p).empty()) {
		n++;
		p = p + dir;
	}
	return n;
}

// Un bout nominalement "ouvert" (case voisine vide) ne veut rien dire si la
// place manque pour qu'un five y tienne un jour — ex: un three collé à un
// bord avec une seule case libre de chaque côté. On ferme les deux bouts
// dès que l'espace total (les deux côtés combinés, un five pouvant se
// terminer de l'un ou l'autre) ne peut plus atteindre 5.
static void gateBySpace(Gomoku &state, int len, Pos before, Pos after, Pos dir, bool &openBefore, bool &openAfter) {
	if (len >= 5)
		return;
	int need = 5 - len;
	int spaceBefore = openBefore ? emptyRun(state, before, dir * -1, need) : 0;
	int spaceAfter = openAfter ? emptyRun(state, after, dir, need) : 0;
	if (spaceBefore + spaceAfter < need) {
		openBefore = false;
		openAfter = false;
	}
}

// Motif cassé : deux groupes séparés par un seul trou, l'ensemble tenant dans
// une fenêtre de 5 cases. `XX.XX` est à un coup du five exactement comme un
// four contigu, mais le comptage par run (runEnd ne suit que du contigu) n'y
// voit que deux twos — quelques dizaines de points (40 à 80 selon que les
// bouts extérieurs soient bloqués ou libres) là où un four en vaut 2000. Le
// trou étant l'unique point de complétion, l'ensemble est classé comme un
// four *fermé*, pas ouvert.
//
// Renvoie un bonus qui s'ajoute au score des runs, sans les remplacer, et
// n'examine qu'une seule direction par appel. À l'appelant de ne pas
// compter deux fois : evaluate() balaie tous les runs et n'appelle donc que
// vers l'avant, localPatternScore n'examine que les runs passant par une
// case et appelle des deux côtés.
static int brokenBonus(Gomoku &state, Pos end, Pos dir, bool player, int len) {
	const Pos gap = end + dir;
	if (!gap.valid() || !state.stone(gap).empty())
		return 0;

	const Pos next = gap + dir;
	if (!next.valid())
		return 0;
	const Stone beyond = state.stone(next);
	if (beyond.empty() || beyond.player() != player)
		return 0;

	const int len2 = runLenOf(next, state.runEnd(next, dir, player), dir);

	// Au-delà de 5 cases d'envergure, les deux groupes ne peuvent plus
	// partager la même fenêtre de five : le trou ne les relie pas vraiment.
	if (len + 1 + len2 > 5)
		return 0;

	return patternWeight(len + len2, 1);
}

// Score de la pierre en `pos` (déjà posée) pour `player`, sommé sur ses 4
// lignes. Quasi O(1) : chaque marche est bornée par la longueur (courte) de
// l'alignement, sans récursion.
//
// Volontairement plus grossier qu'evaluate() : pas de gateBySpace ni de
// brokenBonus ici. Les deux fonctions n'ont pas le même métier — evaluate()
// fixe la *valeur* d'une feuille, où une imprécision se propage directement
// dans la décision ; localPatternScore ne fait que *classer* des coups pour
// choisir lesquels explorer, et la recherche corrige derrière. Or elle est
// appelée deux fois par case candidate à chaque nœud interne, contre une
// fois par feuille pour evaluate() : mesuré, generateCandidates pesait 382ms
// des 460ms du budget, evaluate() seulement 58ms. Retirer ces deux raffinements
// du tri (en les gardant dans evaluate) rend +54% de nœuds et un pli de
// profondeur, pour un score de self-play de 58.3% sur 24 parties — la
// finesse ne servait à rien là où elle coûtait le plus cher.
static int localPatternScore(Gomoku &state, Pos pos, bool player) {
	int total = 0;
	for (const Pos &dir : DIRECTIONS) {
		Pos start = state.runStart(pos, dir, player);
		Pos end = state.runEnd(pos, dir, player);
		int len = runLenOf(start, end, dir);
		Pos before = start - dir;
		Pos after = end + dir;
		bool openBefore = before.valid() && state.stone(before).empty();
		bool openAfter = after.valid() && state.stone(after).empty();
		total += patternWeight(len, (openBefore ? 1 : 0) + (openAfter ? 1 : 0));
	}
	return total;
}

// `checkLegality` active le seul contrôle récursif/coûteux (isLegalMove,
// pour double-trois/double-quatre/foul-overline). Impossible à se payer à
// chaque nœud (potentiellement des centaines de milliers pour atteindre la
// profondeur 10) : la légalité complète n'est garantie qu'à la racine (le
// coup réellement joué est donc toujours 100% légal), les nœuds internes
// explorent les candidats géométriquement plausibles sans la revérifier —
// seule la détection de victoire/nulle (applyMove), toujours exacte, coupe
// l'arbre en profondeur.
static std::vector<Pos> generateCandidates(Gomoku &state, bool checkLegality) {
	if (state.turn() == 0) {
		int8_t center = (int8_t)current_board_size() / 2;
		return {{center, center}};
	}

	const bool player = state.player();
	static std::vector<std::pair<int,Pos>> scored;
	scored.clear();

	// Les boxes sont disjointes par construction (expandBox fusionne tout
	// chevauchement), donc pas de risque de scanner deux fois la même case.
	for (const Box &box : searchBoxes) {
	for (int8_t y = box.minY; y <= box.maxY; y++) {
	for (int8_t x = box.minX; x <= box.maxX; x++) {
		Pos pos{x, y};
		if (!state.stone(pos).empty())
			continue;

		// CANDIDATE_RADIUS vaut 1, donc les 8 sous-directions couvrent
		// exactement le voisinage recherché.
		bool hasNeighbor = false;
		for (const Pos &d : SUBDIRECTIONS) {
			Pos near = pos + d;
			if (near.valid() && !state.stone(near).empty()) {
				hasNeighbor = true;
				break;
			}
		}
		if (!hasNeighbor)
			continue;
		if (checkLegality && !state.isLegalMove(pos, player))
			continue;

		state.rawPlace(pos, player);
		int ownScore = localPatternScore(state, pos, player);
		state.rawRemove(pos, player);

		state.rawPlace(pos, !player);
		int oppScore = localPatternScore(state, pos, !player);
		state.rawRemove(pos, !player);

		// Score de motif seul : un coup de capture isolé (ne prolonge/bloque
		// aucune ligne) note 0 et se fait éliminer par MAX_BRANCH dès que 6
		// coups "normaux" existent sur le plateau — invisible à toute la
		// recherche. wouldCapture couvre l'attaque (je capture) et la
		// défense (l'adversaire capturerait s'il jouait ici). Le poids
		// croît avec le nombre de paires déjà prises (dérivée du bonus
		// quadratique captures² d'evaluate()) : sinon la 9e paire pèse
		// autant que la 1ère ici, et un blocage vital de fin de partie se
		// fait éliminer par des motifs plus "voyants" ailleurs.
		if (state.captureRule()) {
			if (state.wouldCapture(pos, player))
				ownScore += W_CAPTURE * (2 * (int)state.score(player) + 1);
			if (state.wouldCapture(pos, !player))
				oppScore += W_CAPTURE * (2 * (int)state.score(!player) + 1);
		}

		scored.push_back({ownScore + (int)(BLOCK_FACTOR * oppScore), pos});
	}
	}
	}

	auto cut = scored.begin() + std::min(scored.size(), MAX_BRANCH);
	std::partial_sort(scored.begin(), cut, scored.end(), [](auto &a, auto &b){ return a.first > b.first; });

	std::vector<Pos> moves;
	moves.reserve((size_t)(cut - scored.begin()));
	for (auto it = scored.begin(); it != cut; ++it)
		moves.push_back(it->second);
	return moves;
}

// --- Heuristique -----------------------------------------------------------

static bool openAt(Gomoku &state, Pos p) {
	return p.valid() && state.stone(p).empty();
}

static bool stoneOf(Gomoku &state, Pos p, bool player) {
	if (!p.valid())
		return false;
	Stone s = state.stone(p);
	return !s.empty() && s.player() == player;
}

// Un alignement contient-il `target` (de `start` vers `end` selon `dir`) ?
static bool lineContains(Pos start, Pos end, Pos dir, Pos target) {
	for (Pos p = start; ; p = p + dir) {
		if (p.x == target.x && p.y == target.y)
			return true;
		if (p.x == end.x && p.y == end.y)
			return false;
	}
}

// Part "dynamique" : sans elle, l'heuristique ne juge que le plateau final,
// jamais l'enchaînement des coups qui y a mené. Un alignement qui vient
// d'être touché par le dernier coup est une menace vivante, pas une forme
// figée depuis dix tours — on le majore d'une fraction de son propre poids
// (donc un four frais compte, un two frais quasi pas).
//
// Jamais validé en self-play : les matchs de tools/selfplay.py ont tourné
// sur des binaires où cette part était absente. Ce qu'on sait vraiment, et
// seulement d'une version antérieure à bonus fixe testée sur UNE position :
// coût dans le bruit de mesure (~3% de nœuds) et aucun changement du coup
// choisi. Conservé pour la complétude de l'heuristique, en connaissance de
// cause — pas parce qu'un gain a été prouvé.
static constexpr int RECENCY_DIVISOR = 8;

// Parcourt chaque alignement une fois (compté depuis sa case de départ) et
// le note par longueur + extrémités ouvertes. Assez rapide pour tourner à
// chaque feuille : O(taille du plateau), sans récursion.
static int evaluate(Gomoku &state) {
	int score[2] = {0, 0};
	int strongThreats[2] = {0, 0};
	const Pos last = state.lastMove();

	// Bornée aux search boxes plutôt qu'à Gomoku::forall (plateau entier) :
	// toute pierre réelle y est forcément contenue (expandBox est appelé à
	// chaque coup réellement joué), donc c'est strictement équivalent tout
	// en évitant de sonder les cases vides loin de toute pierre à chaque
	// feuille — la même optimisation que generateCandidates.
	auto visit = [&](Pos pos){
		Stone s = state.stone(pos);
		if (s.empty())
			return;
		bool player = s.player();

		for (const Pos &dir : DIRECTIONS) {
			Pos prev = pos - dir;
			if (prev.valid() && state.stone(prev) == s)
				continue; // pas le début de l'alignement

			Pos end = state.runEnd(pos, dir, player);
			int len = runLenOf(pos, end, dir);
			Pos before = pos - dir;
			Pos after = end + dir;

			// Capture potentielle, vue depuis la paire menacée plutôt qu'en
			// sondant chaque case vide du plateau. La règle de capture est
			// VIDE,C,C,!C : une paire capturable est donc forcément un
			// alignement de longueur *exactement* 2 (à 3 pierres, la 3e
			// occupe la place du captureur) avec un flanc vide et l'autre
			// tenu par l'adversaire. On parcourt déjà les alignements, donc
			// c'est gratuit — au lieu d'appeler wouldCapture() deux fois par
			// case vide, ce qui était le poste le plus cher de la fonction.
			if (state.captureRule() && len == 2) {
				const bool taker = !player;
				const bool emptyBeforeStoneAfter =
					openAt(state, before) && stoneOf(state, after, taker);
				const bool emptyAfterStoneBefore =
					openAt(state, after) && stoneOf(state, before, taker);
				if (emptyBeforeStoneAfter || emptyAfterStoneBefore)
					score[taker] += W_CAPTURE * (2 * (int)state.score(taker) + 1);
			}
			bool openBefore = before.valid() && state.stone(before).empty();
			bool openAfter = after.valid() && state.stone(after).empty();
			gateBySpace(state, len, before, after, dir, openBefore, openAfter);
			int w = patternWeight(len, (openBefore ? 1 : 0) + (openAfter ? 1 : 0));
			w += brokenBonus(state, end, dir, player, len);
			if (w > 0 && last.valid() && lineContains(pos, end, dir, last))
				w += w / RECENCY_DIVISOR;
			score[player] += w;
			if (w >= W_OPEN_THREE)
				strongThreats[player]++;
		}
	};
	for (const Box &box : searchBoxes)
		for (int8_t y = box.minY; y <= box.maxY; y++)
			for (int8_t x = box.minX; x <= box.maxX; x++)
				visit(Pos{x, y});

	// Fourche : deux menaces "three ouvert ou mieux" en même temps, dans des
	// directions différentes, ne peuvent pas être bloquées par un seul coup
	// adverse. Approximation volontaire (ne vérifie pas que les points de
	// blocage sont réellement distincts) : suffisant pour valoriser la
	// position sans repayer une détection combinatoire complète à chaque
	// feuille — à affiner si des faux positifs se voient en pratique.
	if (strongThreats[0] >= 2)
		score[0] += W_FORK;
	if (strongThreats[1] >= 2)
		score[1] += W_FORK;

	score[0] += W_CAPTURE * (int)(state.score(0) * state.score(0));
	score[1] += W_CAPTURE * (int)(state.score(1) * state.score(1));

	bool toMove = state.player();
	return score[toMove] - score[!toMove];
}

// --- Recherche : négamax + élagage alpha-bêta + iterative deepening -------

static constexpr int MATE_SCORE = 1'000'000;
static constexpr int INF_SCORE  = 2'000'000;

struct SearchAborted {};

static std::chrono::steady_clock::time_point deadline;
static unsigned long nodeCount = 0;

static void checkTime() {
	if (++nodeCount % 128 == 0 && std::chrono::steady_clock::now() >= deadline)
		throw SearchAborted{};
}

// Table de transposition : cache le résultat d'une position par son hash de
// Zobrist, pour éviter de ré-explorer un sous-arbre déjà vu via un autre
// ordre de coups. Taille fixe, remplacement systématique — suffisant à
// cette échelle, l'essentiel du gain vient d'éviter le travail dupliqué.
enum class TTFlag : uint8_t { Exact, Lower, Upper };

struct TTEntry {
	uint64_t key = 0;
	int depth = -1;
	int score = 0;
	TTFlag flag = TTFlag::Exact;
	Pos best{-1, -1};
};

static constexpr size_t TT_SIZE = 1u << 21;
static std::vector<TTEntry> tt(TT_SIZE);

static int negamax(Gomoku &state, int depth, int alpha, int beta) {
	checkTime();

	if (depth == 0)
		return evaluate(state);

	const uint64_t key = state.zobrist();
	TTEntry &slot = tt[key % TT_SIZE];
	const bool hit = slot.key == key;
	Pos ttMove{-1, -1};

	if (hit) {
		ttMove = slot.best;
		if (slot.depth >= depth) {
			if (slot.flag == TTFlag::Exact)
				return slot.score;
			if (slot.flag == TTFlag::Lower)
				alpha = std::max(alpha, slot.score);
			else
				beta = std::min(beta, slot.score);
			if (alpha >= beta)
				return slot.score;
		}
	}

	std::vector<Pos> moves = generateCandidates(state, false);
	if (moves.empty())
		return 0;

	if (ttMove.valid()) {
		auto it = std::find_if(moves.begin(), moves.end(), [&](Pos p){ return p.x == ttMove.x && p.y == ttMove.y; });
		if (it != moves.end())
			std::rotate(moves.begin(), it, it + 1);
	}

	const int origAlpha = alpha;
	int best = -INF_SCORE;
	Pos bestMove = moves.front();
	bool first = true;
	for (Pos m : moves) {
		expandBox(m);
		Outcome outcome = state.applyMove(m);
		int v;
		if (outcome.state == Result::Win) {
			v = MATE_SCORE + depth;
		} else if (outcome.state == Result::Draw) {
			v = 0;
		} else if (first) {
			v = -negamax(state, depth - 1, -beta, -alpha);
		} else {
			// PVS/negascout : le coup de la TT est essayé en premier (voir le
			// rotate ci-dessus), donc les suivants sont a priori pires que
			// `alpha` — une recherche à fenêtre nulle suffit à le confirmer,
			// bien moins chère qu'une fenêtre complète. Si l'hypothèse est
			// fausse (le coup est en fait meilleur), on ne re-cherche à
			// fenêtre complète que dans ce cas, plus rare.
			v = -negamax(state, depth - 1, -alpha - 1, -alpha);
			if (v > alpha && v < beta)
				v = -negamax(state, depth - 1, -beta, -alpha);
		}
		state.undo();
		first = false;

		if (v > best) {
			best = v;
			bestMove = m;
		}
		if (best > alpha)
			alpha = best;
		if (alpha >= beta)
			break;
	}

	TTFlag flag = best <= origAlpha ? TTFlag::Upper : best >= beta ? TTFlag::Lower : TTFlag::Exact;
	slot = {key, depth, best, flag, bestMove};

	return best;
}

// Rempli par rootSearch à chaque profondeur : (coup, score) de la racine,
// pour un aperçu du raisonnement de l'IA (soutenance / debug) sur stderr.
static std::vector<std::pair<Pos,int>> rootScores;

static std::pair<Pos,int> rootSearch(Gomoku &state, int depth) {
	std::vector<Pos> moves = generateCandidates(state, true);

	Pos bestMove = moves.front();
	int alpha = -INF_SCORE;
	const int beta = INF_SCORE;

	// Accumulé localement : n'écrase rootScores (dernière profondeur
	// complète) qu'une fois la boucle finie, pour ne pas perdre les
	// données d'une profondeur déjà aboutie si celle-ci est interrompue
	// par checkTime() en cours de route.
	std::vector<std::pair<Pos,int>> scores;
	for (Pos m : moves) {
		checkTime();
		expandBox(m);
		Outcome outcome = state.applyMove(m);
		int v;
		if (outcome.state == Result::Win)
			v = MATE_SCORE + depth;
		else if (outcome.state == Result::Draw)
			v = 0;
		else
			v = -negamax(state, depth - 1, -beta, -alpha);
		state.undo();

		scores.push_back({m, v});
		if (v > alpha) {
			alpha = v;
			bestMove = m;
		}
	}
	rootScores = std::move(scores);
	return {bestMove, alpha};
}

static constexpr int MAX_DEPTH = 14;
static constexpr auto TIME_BUDGET = std::chrono::milliseconds(460);
// Le sujet exige *toujours* au moins 10 plis, pas "en moyenne". Les
// profondeurs jusqu'à MIN_DEPTH ont donc un plafond élargi (HARD_BUDGET) au
// lieu du budget normal, pour qu'une position dense n'écourte pas la
// recherche avant le palier 10 ; au-delà, le vrai budget de 460ms
// s'applique. Ce n'est pas une garantie absolue : une position assez
// pathologique peut encore être interrompue par HARD_BUDGET avant le palier
// — celui-ci borne le pire cas plutôt que de laisser filer le temps de
// réponse, arbitrage assumé pour rester proche des 500ms de moyenne visée.
static constexpr int MIN_DEPTH = 10;
static constexpr auto HARD_BUDGET = std::chrono::milliseconds(800);

int main() {
	std::string rules;
	std::getline(std::cin, rules);

	Gomoku state(rules);

	searchBoxes.clear();

	Pos move;
	char c;
	Outcome outcome{};
	while (std::cin >> c && c == '|') {
		std::cin >> move;
		std::cout << move;
		outcome = state.applyMove(move);
		expandBox(move);
	}

	std::cerr << state << std::endl;

	// État de la partie après rejeu de l'historique. Le moteur le calculait
	// déjà sans jamais l'exposer ; l'imprimer permet à un pilote externe
	// (tools/selfplay.py) de savoir quand la partie est finie, sans
	// réimplémenter les règles de son côté.
	if (outcome.state == Result::Win)
		std::cerr << "Result: win " << (int)outcome.winner << std::endl;
	else if (outcome.state == Result::Draw)
		std::cerr << "Result: draw" << std::endl;
	else
		std::cerr << "Result: ongoing" << std::endl;

	if (outcome.state != Result::Ongoing)
		return 0;

	auto start = std::chrono::steady_clock::now();
	deadline = start + TIME_BUDGET;

	std::vector<Pos> rootMoves = generateCandidates(state, true);
	Pos bestMove = rootMoves.empty() ? Pos{-1,-1} : rootMoves.front();
	int depthReached = 0;

	if (!rootMoves.empty()) {
		for (int depth = 1; depth <= MAX_DEPTH; depth++) {
			deadline = start + (depth <= MIN_DEPTH ? HARD_BUDGET : TIME_BUDGET);
			if (depth > MIN_DEPTH && std::chrono::steady_clock::now() >= deadline)
				break;
			try {
				auto [move, score] = rootSearch(state, depth);
				bestMove = move;
				depthReached = depth;
				if (score >= MATE_SCORE)
					break;
			} catch (SearchAborted &) {
				break;
			}
		}
	}

	std::cerr << "Depth reached: " << depthReached
		<< " | Nodes: " << nodeCount
		<< " | Search time: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() << "ms"
		<< std::endl;

	// Affiché en 1-indexé pour matcher les coordonnées montrées à l'écran
	// par l'UI (aria-label "Intersection x+1, y+1"), contrairement au
	// protocole stdin/stdout qui reste en 0-indexé.
	std::sort(rootScores.begin(), rootScores.end(), [](auto &a, auto &b){ return a.second > b.second; });
	std::cerr << "Candidates (move: score):";
	for (size_t i = 0; i < rootScores.size(); i++)
		std::cerr << " " << (int)rootScores[i].first.x + 1 << ":" << (int)rootScores[i].first.y + 1 << ":" << rootScores[i].second;
	std::cerr << " -> chosen " << (int)bestMove.x + 1 << ":" << (int)bestMove.y + 1 << std::endl;

	if (bestMove.valid())
		std::cout << "|" << bestMove;
}
