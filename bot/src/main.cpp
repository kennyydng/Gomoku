
#include "Gomoku.class.hpp"
#include <list>

struct MinimaxResult {
	float score;
	std::list<Pos> line;
};

static void candidates(Gomoku &state, auto &&f) {
	Gomoku::forall([&](Pos pos){
		if (!state.stone(pos).empty())
			return;
		for (int n = 0; n < 8; n++) {
			auto around = pos + SUBDIRECTIONS[n];
			if (around.valid() && !state.stone(around).empty()) {
				std::cerr << pos << std::endl;
				state.with_move(pos, [&](Gomoku &state){
					std::cerr << state;
					f(state, pos);
				});
				break;
			}
		}
	});
}

static float heuristic(Gomoku &state) {
	(void)state;
	return 0;
}

static unsigned int count = 0;

static MinimaxResult minimax(Gomoku &state, const unsigned int depth) {
	if (depth == 0 || count > 1000000)
		return { heuristic(state), {} };
	count++;

	MinimaxResult best = {-100000,{}};
	candidates(state, [&](Gomoku &state, Pos move){
		auto counter = minimax(state, depth-1);
		if (best.score < -counter.score) {
			best.score = -counter.score;
			std::swap(best.line,counter.line);
			best.line.push_front(move);
		}
	});

	return best;
}

int main() {
	std::string rules;
	std::getline(std::cin, rules);

	Gomoku state(rules);

	Pos move;
	char c;
	while (std::cin >> c && c == '|') {
		std::cin >> move;
		std::cout << move;
		state.play(move);
	}

	std::cerr << state << std::endl;

	auto result = minimax(state, 4);
	std::cerr << "Visited " << count << " nodes" << std::endl;
}

/* 
* P is bot
* O is opponent
* Algorithm :
* if win1(P) => Play it
* if win1(O) {
*   find counters
*   if counters.any() {
*     
*   } else {
*     return ???
*   }
* } else {
*  for each X0 = win2(P), simulate(X0)
*   with simulate(X0) {
*     update win1(P) => find counters
*     update win2(P)
*   } 
* }
*/

