
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

	//o << gomoku.player_info(gomoku.player()).stones;
	for (Pos const pos : Pos::all()) {
		o << gomoku.stone(pos) << (pos.x == (SIZE-1) ? "\n" : "─");
	}
//	for (int i = 0; i < 4; i++) {
//		for (Pos const pos : Pos::all())
//			o << (gomoku._info[0].lines[i].of(0)[pos] == 0 ? "0" : "1" ) << (pos.x == 18 ? "\n" : "─");
//			//o << (gomoku.LineStart[i][pos] == 0 ? "0" : "1" ) << (pos.x == 18 ? "\n" : "─");
//		o << std::endl;
//	}
	return o;
}

void Gomoku::place(Pos pos, bool P) {
	contract_assert(pos.valid());
	contract_assert(stone(pos).empty());
	auto &p0 = _info[ P];
	auto &p1 = _info[!P];
	p0.stones += pos;
	template for (constexpr size_t AX : index_of(AXES)) {
		auto const line = Line<AX>(pos);
		auto &p0lines = std::get<AX>(p0.lines);
		auto &p1lines = std::get<AX>(p1.lines);
		p0lines.score( compute(p1lines.of(0) & line), _score.upgrade_updater(P) );
		p1lines.score( compute(p0lines.of(0) & line), _score.block_updater(!P)  );
		p0lines += line;
	}
}

//void Gomoku::take(Pos pos, bool P) {
//	contract_assert(pos.valid());
//	contract_assert(stone(pos).player() == P);
//	auto &p0 = _info[ P];
//	auto &p1 = _info[!P];
//	p0.stones -= pos;
//	template for (constexpr size_t AX : index_of(AXES)) {
//		auto &l0 = std::get<AX>(p0.lines);
//		auto &l1 = std::get<AX>(p1.lines);
//		auto line = Line<AX>(pos);
//		l0 -= line;
//		score_t l0delta = l0.score(l1.of(0) & line);
//		score_t l1delta = l1.score(l0.of(0) & line);
//		if (P) 	{ _score += l0delta; _score -= (l0delta << 1); _score -= l1delta; }
//		else   	{ _score -= l0delta; _score += (l0delta << 1); _score += l1delta; }
//	}
//}

void Gomoku::pass() {
	//std::cerr << std::string(turn(), ' ') << "Pass" << std::endl;
	_moves.push_back({});
}

void Gomoku::play(Pos pos) {
	const bool player = this->player();

	//std::cerr << std::string(turn(), ' ') << "Do move " << pos << std::endl;
	place(pos, player);
	//std::cerr << std::string(turn()+1, ' ') << "Score " << _score << std::endl;

//#if R_CAPTURE
//	uint16_t cap_mask = 0b111'1'1'111; // Top 8 bits are free in case of self-capture rules
//	if (pos.x < 3 ) cap_mask &= (uint16_t)0b110'1'0'110;
//	if (pos.x > 15) cap_mask &= (uint16_t)0b011'0'1'011;
//	if (pos.y < 3 ) cap_mask &= (uint16_t)0b111'1'1'000;
//	if (pos.y > 15) cap_mask &= (uint16_t)0b000'1'1'111;
//
//	for (int n = 0; n < 8; n++) {
//		if (cap_mask>>n & 1) {
//			const Pos dir = DIRECTIONS[n];
//			const Pos pos0 = pos + dir;
//			const Pos pos1 = pos0 + dir;
//			const Pos pos2 = pos1 + dir;
//			if (
//				stones[!player][pos0] &&
//				stones[!player][pos1] &&
//				stones[ player][pos2]
//			) {
//				stones[!player] -= pos0;
//				stones[!player] -= pos1;
//				captures[player] += 2;
//			} else {
//				cap_mask &= ~(1<<n);
//			}
//		}
//	}
//	_moves.push_back({{pos,cap_mask}});
//#else
	_moves.push_back({{pos}});
//#endif
}

//void Gomoku::undo() {
//	//std::cerr << std::string(turn()-1, ' ') << "Undo move" << std::endl;
//	auto move = _moves.back();
//	_moves.pop_back();
//	if (!move)
//		return;
//
//	const bool player = this->player();
////#if R_CAPTURE
////	const auto [pos, cap_mask] = *move;
////
////	for (int n = 0; n < 8; n++) {
////		if ((cap_mask>>n) & 1) {
////			const Pos dir = DIRECTIONS[n];
////			const Pos pos0 = pos + dir;
////			const Pos pos1 = pos0 + dir;
////
////			stones[!player] += pos0;
////			stones[!player] += pos1;
////
////			captures[player] -= 2;
////		}
////	}
////#else
//	const auto pos = move->pos;
////#endif
//	take(pos, player);
//}
