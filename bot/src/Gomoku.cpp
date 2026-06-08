
#include "Gomoku.class.hpp"
#include <cassert>
#include <iostream>

std::ostream &operator<<(std::ostream &o, Gomoku const &gomoku) {
	o << gomoku.turn() << " " <<
		Stone(Stone::WHITE) << gomoku.score(0) << "-" <<
		Stone(Stone::BLACK) << gomoku.score(1) << "\n";
	Gomoku::forall([&](Pos &&pos){
		o << gomoku.stone(pos) << (pos.x == 18 ? "\n" : "─");
	});
	return o;
}

void Gomoku::play(Pos move) {
	const bool player = this->player();
	uint16_t cap_mask = 0b111'1'1'111; // Top 8 bits are free in case of self-capture rules

	//std::cerr << "Do move " << move << std::endl;
	assert(move.valid() && stone(move).empty());

	if (move.x < 3 ) cap_mask &= (uint16_t)0b110'1'0'110;
	if (move.x > 15) cap_mask &= (uint16_t)0b011'0'1'011;
	if (move.y < 3 ) cap_mask &= (uint16_t)0b111'1'1'000;
	if (move.y > 15) cap_mask &= (uint16_t)0b000'1'1'111;

	stones[player] += move;
	for (int n = 0; n < 8; n++) {
		if (cap_mask>>n & 1) {
			const Pos dir = SUBDIRECTIONS[n];
			const Pos pos0 = move + dir;
			const Pos pos1 = pos0 + dir;
			const Pos pos2 = pos1 + dir;
			if (
				stones[!player][pos0] &&
				stones[!player][pos1] &&
				stones[ player][pos2]
			) {
				stones[!player] -= pos0;
				stones[!player] -= pos1;
				captures[player] += 2;
			} else {
				cap_mask &= ~(1<<n);
			}
		}
	}

	moves.push_back({move,cap_mask});
}

void Gomoku::undo() {
	const bool player = this->player();

	//std::cout << "Undo move " << move << std::endl;
	const auto [move, cap_mask] = moves.back();
	moves.pop_back();

	for (int n = 0; n < 8; n++) {
		if ((cap_mask>>n) & 1) {
			const Pos dir = SUBDIRECTIONS[n];
			const Pos pos0 = move + dir;
			const Pos pos1 = pos0 + dir;

			stones[!player] += pos0;
			stones[!player] += pos1;

			captures[!player] -= 2;
		}
	}
	stones[player] -= move;
};
