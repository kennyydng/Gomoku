
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
// (voir MIN_DEPTH plus bas, qui impose la profondeur 10 quel que soit le
// temps que ça prend — réduire le branchement est le seul levier pour
// respecter aussi le budget moyen sans risquer de rater cette profondeur).
static constexpr size_t MAX_BRANCH = 5;
static constexpr double BLOCK_FACTOR = 0.8;

// Boîte englobante (élargie de CANDIDATE_RADIUS) de toutes les pierres
// jouées depuis le début de la recherche. Ne fait que grandir : pas cher à
// maintenir, et un coup hors de la boîte est par construction trop loin de
// toute pierre pour être un candidat valable. Restreindre le scan à cette
// boîte transforme generateCandidates() d'un O(taille²) en O(aire boîte),
// ce qui compte vu que ça tourne à chaque nœud.
struct Box { int8_t minX, maxX, minY, maxY; };
static Box searchBox;

static void expandBox(Pos p) {
	int8_t lo = 0, hi = (int8_t)current_board_size() - 1;
	searchBox.minX = std::max(lo, (int8_t)std::min<int>(searchBox.minX, p.x - CANDIDATE_RADIUS));
	searchBox.maxX = std::min(hi, (int8_t)std::max<int>(searchBox.maxX, p.x + CANDIDATE_RADIUS));
	searchBox.minY = std::max(lo, (int8_t)std::min<int>(searchBox.minY, p.y - CANDIDATE_RADIUS));
	searchBox.maxY = std::min(hi, (int8_t)std::max<int>(searchBox.maxY, p.y + CANDIDATE_RADIUS));
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

// Score de la pierre en `pos` (déjà posée) pour `player`, sommé sur ses 4
// lignes. Quasi O(1) : chaque marche est bornée par la longueur (courte) de
// l'alignement, sans récursion.
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

	for (int8_t y = searchBox.minY; y <= searchBox.maxY; y++) {
	for (int8_t x = searchBox.minX; x <= searchBox.maxX; x++) {
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
		// défense (l'adversaire capturerait s'il jouait ici).
		if (state.captureRule()) {
			if (state.wouldCapture(pos, player))
				ownScore += W_CAPTURE;
			if (state.wouldCapture(pos, !player))
				oppScore += W_CAPTURE;
		}

		scored.push_back({ownScore + (int)(BLOCK_FACTOR * oppScore), pos});
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

// Parcourt chaque alignement une fois (compté depuis sa case de départ) et
// le note par longueur + extrémités ouvertes. Assez rapide pour tourner à
// chaque feuille : O(taille du plateau), sans récursion.
static int evaluate(Gomoku &state) {
	int score[2] = {0, 0};

	Gomoku::forall([&](Pos pos){
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
			bool openBefore = before.valid() && state.stone(before).empty();
			bool openAfter = after.valid() && state.stone(after).empty();
			score[player] += patternWeight(len, (openBefore ? 1 : 0) + (openAfter ? 1 : 0));
		}
	});

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
	for (Pos m : moves) {
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
// profondeurs jusqu'à MIN_DEPTH ont un plafond large au lieu du budget
// normal, pour qu'une position dense n'écourte jamais la recherche avant le
// palier 10 ; au-delà, le vrai budget de 460ms s'applique. HARD_BUDGET est
// un filet de sécurité contre une position pathologique, pas un objectif.
static constexpr int MIN_DEPTH = 10;
static constexpr auto HARD_BUDGET = std::chrono::milliseconds(1500);

int main() {
	std::string rules;
	std::getline(std::cin, rules);

	Gomoku state(rules);

	int8_t hi = (int8_t)current_board_size() - 1;
	searchBox = {hi, 0, hi, 0};

	Pos move;
	char c;
	while (std::cin >> c && c == '|') {
		std::cin >> move;
		std::cout << move;
		state.applyMove(move);
		expandBox(move);
	}

	std::cerr << state << std::endl;

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
