
#include "Gomoku.class.hpp"
#include <list>

//struct MinimaxResult {
//	float score;
//	std::list<Pos> line;
//};
//
//struct Candidates {
//	Gomoku &state;
//	Pos current = {255,255};
//	bool next() {
//		while ( try_next() ) {
//			if ( state.cell(current).empty() ) {
//				for (int n = 0; n < 8; n++) {
//					auto pos = current + Directions[n];
//					if ( pos.valid() && !state.cell(pos).empty() )
//						return true;
//				}
//			}
//		}
//		return false;
//	};
//private:
//	bool try_next() {
//		if (current.x == 255) {
//			current = {0,0};
//			return true;
//		} else {
//			++current;
//			return current.valid();
//		}
//	}
//};
//
//static float heuristic(Gomoku &state) {
//	(void)state;
//	return 0;
//}
//
//static unsigned int counter = 0;
//
//static MinimaxResult minimax(Gomoku &state, const unsigned int depth) {
//	counter++;
//	if (depth == 0 || counter > 1000000)
//		return { heuristic(state), {} };
//
//	Candidates candidates = {state};
//	MinimaxResult best = {-100000,{}};
//	while ( candidates.next() ) {
//		auto move = candidates.current;
//		state.with_move( move, [&](Gomoku &state){
//			auto counter = minimax(state, depth-1);
//			if (best.score < -counter.score) {
//				best.score = -counter.score;
//				std::swap(best.line,counter.line);
//				best.line.push_front(move);
//			}
//		});
//	}
//
//	return best;
//};

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

	//auto move = minimax(state, 4);
	//std::cerr << "Visited " << counter << " nodes" << std::endl;
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

