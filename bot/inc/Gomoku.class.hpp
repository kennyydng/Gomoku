
#pragma once

#include <iostream>
#include <cstdint>
#include <vector>

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

	constexpr auto block_updater(bool P)
		TO( [&,sign = P?1:-1]<size_t I>(unsigned long delta) {
			if constexpr (I == 0) return;
			else {
				value += sign * delta * line_values[I];
			}
		} )

	constexpr std::strong_ordering operator<=>(this score_t const &lhs, score_t const &rhs)
		{ return lhs.value <=> rhs.value; }

	friend std::ostream &operator<<(std::ostream &o, score_t const &score)
		{ return o << "{" << score.value << "}"; }
};

struct Move {
	Pos pos;
//#if R_CAPTURE
//	uint16_t captures;
//#endif
};

class Gomoku {
public:
	Gomoku() {_moves.reserve(SIZE*SIZE);}
	Gomoku(Gomoku const &) = default;
	~Gomoku() {}

	unsigned turn() const
		{ return _moves.size(); }
	bool player() const
		{ return turn() % 2; }
	score_t const &heuristic() const
		{ return _score; }

	NOINLINE bool is_over() const {
			template for (constexpr size_t AX : index_of(AXES)) {
				if (
					vec::any( std::get<AX>(_info[0].lines).of(5).get_words() ) ||
					vec::any( std::get<AX>(_info[1].lines).of(5).get_words() )
				) return true;
			}
			return false;
		}

	auto const &player_info(bool p) const
		{ return _info[p]; }

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
	//void undo();

private:
	void place(Pos, bool);
	//void take(Pos, bool);

	template<size_t AX>
	static constexpr auto Line(Pos pos)
		{ return BitBoard<AX>::make_line(pos,5); }

	template<size_t AX>
	static constexpr BitBoard<AX> LinesStart =
		BitBoard<AX>(true).shift(AXES[AX]*-4);

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

	std::vector<std::optional<Move>> _moves = {};
	score_t _score = score_t();

	friend std::ostream &operator<<(std::ostream &o, Gomoku const &gomoku);
	friend BitBoard<0> candidates(Gomoku &state);
};
