
#include <stack>
#include <contracts>

#include "Gomoku.class.hpp"

struct MinimaxResult {
	score_t score;
	std::stack<std::optional<Pos>> line;
};

BitBoard<0> candidates(Gomoku const &gomoku) {
	const auto &player   = gomoku.player_info( gomoku.player());
	const auto &opponent = gomoku.player_info(!gomoku.player());

	BitBoard<0> all{player.stones + opponent.stones};
	BitBoard<0> set{CENTER};
	template for (constexpr auto dir : DIRECTIONS) {
		set += all.shift(dir*1);
	}
	set -= all;

	return set;
}

static unsigned int count = 0;
template<bool PLAYER>
MinimaxResult minimax_alt(Gomoku const &gomoku, int depth) {
	const score_t score = gomoku.heuristic();

	//std::cerr << gomoku << std::endl;
	count++;
	if (depth == 0 || gomoku.is_over())
		return { score, {} };
	if (count > 1000000)
		return { score, {} };

	//std::cerr << gomoku << std::endl;

	auto evaluate_next = [&](std::optional<Pos> move){
		//[[assume(gomoku.player() == PLAYER)]];
		auto result = gomoku.with_move(move, [depth](Gomoku &gomoku){
			return minimax_alt<!PLAYER>(gomoku, depth-1);
		});
		result.line.push(move);
		return result;
	};

	MinimaxResult best = evaluate_next(std::nullopt);

	for (Pos const pos : candidates(gomoku)) {
		contract_assert(pos.valid());
		auto result = evaluate_next(pos);
		bool comp = PLAYER ? result.score > best.score : result.score < best.score;
		if (comp) {
			best.score = result.score;
			best.line = std::move(result.line);
			//std::cerr << std::size(best.line) << std::endl;
		}
	}

	//if (!best.line.top()) {
	//	std::cerr << gomoku << std::endl;
	//	std::cerr << candidates(gomoku) << std::endl;
	//}

	return best;
}

MinimaxResult minimax(Gomoku const &gomoku, int depth) {
	if (gomoku.player())
		return minimax_alt<true>(gomoku, depth);
	else
		return minimax_alt<false>(gomoku, depth);
}

int main() {
	Gomoku gomoku;

	char c;
	while (std::cin >> c && c == '|') {
		Pos move;
		std::cin >> move;
		//std::cerr << move << "|";
		gomoku.play(move);
	}

	std::cerr << std::endl;
	std::cerr << gomoku << std::endl;

	//template for (constexpr auto AX : index_of(AXES)) {
	//	std::cerr << "AX : " << AX << std::endl;
	//	Let lines = std::get<AX>(gomoku.player_info(false).lines);
	//	template for (constexpr auto L : sugar::natural_index<6>) {
	//		std::cerr << "Length : " << L << std::endl;
	//		std::cerr << lines.of(L) << std::endl;
	//	}
	//}

	std::cerr << "Running minimax" << std::endl;

	auto [score,line] = minimax(gomoku, 4);
	if (line.top())
		std::cout << *line.top();
	else
		std::cout << "/";

	std::cerr << "Visited " << count << " nodes" << std::endl;

	std::cerr << "Score : " << score << std::endl;

	std::cerr << "Line : ";
	while (!line.empty()) {
		if (line.top())
			std::cerr << *line.top() << " ";
		else
			std::cerr << "/ ";
		line.pop();
	}
	std::cerr << std::endl;
}

void handle_contract_violation( std::contracts::contract_violation const &cv ) noexcept {
	std::source_location loc = cv.location();
	std::cerr << "Contract violation in " << loc.file_name() << ":" << loc.line() << std::endl;
	std::cerr << "[" << loc.function_name() << "]" << std::endl;

	std::cerr << cv.comment() << std::endl;
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


// Visual tests for shifting
//	template for (constexpr auto AX : index_of(AXES)) {
//		std::cerr << "AX : " << AX << std::endl;
//		std::cerr << BitBoard<AX>(true);
//		template for (constexpr auto dir : AXES)
//			std::cerr << dir*2 << " | ";
//		std::cerr << std::endl;
//		for (auto y : natural_index<pos_t(19)>) {
//			template for (constexpr auto dir : AXES) {
//				static constexpr BitBoard<AX> bb = BitBoard<AX>(true).shift(dir*2);
//				for (auto x : natural_index<pos_t(19)>)
//					std::cerr << (bb[{x,y}] ? 'X' : '.') << ' ';
//				std::cerr << "| ";
//			}
//			std::cerr << std::endl;
//		}
//		template for (constexpr auto dir : AXES)
//			std::cerr << dir*-2 << " | ";
//		std::cerr << std::endl;
//		for (auto y : natural_index<pos_t(19)>) {
//			template for (constexpr auto dir : AXES) {
//				static constexpr BitBoard<AX> bb = BitBoard<AX>(true).shift(dir*-2);
//				for (auto x : natural_index<pos_t(19)>)
//					std::cerr << (bb[{x,y}] ? 'X' : '.') << ' ';
//				std::cerr << "| ";
//			}
//			std::cerr << std::endl;
//		}
//		std::cerr << std::endl;
//	}


