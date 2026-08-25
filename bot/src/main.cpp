
#include "Gomoku.class.hpp"
#include <algorithm>
#include <chrono>
#include <vector>

// --- Move generation ---------------------------------------------------
//
// Candidates are restricted to empty cells within CANDIDATE_RADIUS of an
// existing stone (keeps the branching factor bounded on a mostly-empty
// 19x19/15x15 board), filtered for legality (isLegalMove, the only
// recursive/expensive check), and ranked by a cheap non-recursive pattern
// score: the shape the move creates for its own side (offense) plus the
// shape it denies the opponent (defense/blocking).

static constexpr int CANDIDATE_RADIUS = 1;
static constexpr size_t MAX_BRANCH = 8;
static constexpr double BLOCK_FACTOR = 0.8;

// Bounding box (inflated by CANDIDATE_RADIUS) around every stone that has
// ever been on the board during this search. Only ever grows: cheap to
// maintain, and search moves that fall outside it are still adjacent to a
// real stone by construction, so it never hides a legitimate candidate.
// Restricting the board scan to it turns generateCandidates() from an
// O(boardSize^2) scan into an O(boxArea) one, which matters since it runs
// at every node.
struct Box { int8_t minX, maxX, minY, maxY; };
static Box searchBox;

static void expandBox(Pos p) {
	int8_t lo = 0, hi = (int8_t)current_board_size() - 1;
	searchBox.minX = std::max(lo, (int8_t)std::min<int>(searchBox.minX, p.x - CANDIDATE_RADIUS));
	searchBox.maxX = std::min(hi, (int8_t)std::max<int>(searchBox.maxX, p.x + CANDIDATE_RADIUS));
	searchBox.minY = std::max(lo, (int8_t)std::min<int>(searchBox.minY, p.y - CANDIDATE_RADIUS));
	searchBox.maxY = std::min(hi, (int8_t)std::max<int>(searchBox.maxY, p.y + CANDIDATE_RADIUS));
}

// --- Pattern scoring ---------------------------------------------------
//
// Classifies a run of stones by length + how many of its two ends are open,
// via a direct (non-recursive) walk with runStart/runEnd. Used both as the
// leaf heuristic (evaluate(), full board) and, much more often, as a cheap
// per-candidate move-ordering score (single line through one cell): no
// recursion, unlike the legality-only threat detection in isLegalMove().

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

// Score of the single stone at `pos` (already placed) for `player`, summed
// over its 4 lines. O(1)-ish: each line walk is bounded by the (short) run
// length, no recursion.
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

// `checkLegality` gates the one recursive/expensive check here
// (isLegalMove, for double-three/four-four/foul-overline). It is only
// affordable at a handful of calls per game turn, not at every one of the
// (possibly hundreds of thousands of) internal search nodes needed to reach
// the required 10-ply depth within the time budget. So: full legality is
// always enforced at the root (the move the bot actually plays must be
// 100% legal), while deeper hypothetical nodes explore the geometrically
// plausible candidates without re-deriving that check — win/draw detection
// (applyMove) is unaffected and always exact, only the rare "this exact
// hypothetical stone would itself be a forbidden double-three" case is
// approximated away deep in the tree.
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

		bool hasNeighbor = false;
		for (int8_t dy = -CANDIDATE_RADIUS; dy <= CANDIDATE_RADIUS && !hasNeighbor; dy++) {
			for (int8_t dx = -CANDIDATE_RADIUS; dx <= CANDIDATE_RADIUS; dx++) {
				if (dx == 0 && dy == 0)
					continue;
				Pos near = pos + Pos{dx,dy};
				if (near.valid() && !state.stone(near).empty()) {
					hasNeighbor = true;
					break;
				}
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

// --- Heuristic --------------------------------------------------------------

// Scans every run of stones once (only counted from its start cell) and
// scores it by length + how many of its two ends are open. Cheap enough to
// run at every leaf: O(board size) per call, no recursion.
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
				continue; // not the start of this run

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

// --- Search: negamax with alpha-beta pruning + iterative deepening ---------

static constexpr int MATE_SCORE = 1'000'000;
static constexpr int INF_SCORE  = 2'000'000;

struct SearchAborted {};

static std::chrono::steady_clock::time_point deadline;
static unsigned long nodeCount = 0;

static void checkTime() {
	if (++nodeCount % 128 == 0 && std::chrono::steady_clock::now() >= deadline)
		throw SearchAborted{};
}

// Transposition table: caches the result of a position keyed by its Zobrist
// hash, so that when the same board is reached again through a different
// move order (common with alpha-beta, since move ordering is only a
// heuristic) the subtree is not re-explored from scratch. Fixed-size,
// always-replace — simple, and the dominant win (avoiding duplicate work)
// doesn't need a fancier replacement policy at this scale.
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

static std::pair<Pos,int> rootSearch(Gomoku &state, int depth) {
	std::vector<Pos> moves = generateCandidates(state, true);

	Pos bestMove = moves.front();
	int alpha = -INF_SCORE;
	const int beta = INF_SCORE;

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

		if (v > alpha) {
			alpha = v;
			bestMove = m;
		}
	}
	return {bestMove, alpha};
}

static constexpr int MAX_DEPTH = 14;
static constexpr auto TIME_BUDGET = std::chrono::milliseconds(460);

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
		for (int depth = 1; depth <= MAX_DEPTH && std::chrono::steady_clock::now() < deadline; depth++) {
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
		<< " | Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() << "ms"
		<< std::endl;

	if (bestMove.valid())
		std::cout << "|" << bestMove;
}
