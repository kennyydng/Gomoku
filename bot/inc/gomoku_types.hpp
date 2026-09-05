
#pragma once

#include <iostream>
#include <cstdint>

#include "sets.hpp"
#include "parsing_utils.hpp"

using pos_t = int;

constexpr pos_t SIZE = 19;
constexpr pos_t HALF = SIZE/2;

struct Dir {
	pos_t x = 0;
	pos_t y = 0;

	constexpr Dir operator-() const
		{ return {-x, -y}; }
	constexpr Dir operator*( this Dir lhs, int rhs )
		{ return {pos_t(lhs.x * rhs  ), pos_t(lhs.y * rhs  )}; }

	constexpr Dir operator+( this Dir lhs, Dir rhs )
		{ return {pos_t(lhs.x + rhs.x), pos_t(lhs.y + rhs.y)}; }
	constexpr Dir operator-( this Dir lhs, Dir rhs )
		{ return {pos_t(lhs.x - rhs.x), pos_t(lhs.y - rhs.y)}; }

	friend std::ostream &operator<<(std::ostream &os, const Dir &pos)
		{ return os << "{" << (int)pos.x << ":" << "}" << (int)pos.y; }
};

struct Pos {
	pos_t x = 0;
	pos_t y = 0;

	constexpr bool valid() const
		{ return unsigned(x) < SIZE && unsigned(y) < SIZE; }

	constexpr Pos operator+( this Pos lhs, Dir rhs )
		{ return {pos_t(lhs.x + rhs.x), pos_t(lhs.y + rhs.y)}; }
	constexpr Pos operator-( this Pos lhs, Dir rhs )
		{ return {pos_t(lhs.x - rhs.x), pos_t(lhs.y - rhs.y)}; }

	friend std::istream &operator>>(std::istream &is, Pos &pos) {
			short x,y;
			is >> x >> Expect(":") >> y;
			pos = {(pos_t)x,(pos_t)y};
			return is;
		};
	friend std::ostream &operator<<(std::ostream &os, const Pos &pos)
		{ return os << (int)pos.x << ":" << (int)pos.y; }

	static constexpr auto all() {
			constexpr auto coords = sets::all_of<pos_t(SIZE)>();
			return sugar::product(coords,coords) | [](pos_t x, pos_t y){
				return Pos{x,y};
			};
		}
};

constexpr Pos CENTER = {HALF, HALF};

constexpr Dir AXES[4] = {
		{ 1, 0}, { 0, 1},
		{ 1, 1}, { 1,-1},
	};
constexpr Dir DIRECTIONS[8] = {
		{-1,-1},{ 0,-1},{ 1,-1},
		{-1, 0},        { 1, 0},
		{-1, 1},{ 0, 1},{ 1, 1},
	};

class Stone {
public:
	enum type : uint8_t {
		NONE  = 0b00, ERROR = 0b11,
		BLACK = 0b01, WHITE = 0b10
	};
private:
	type content;
public:
	Stone() : content(NONE) {}
	Stone(type stone) : content(stone) {}
	Stone(bool p0, bool p1) : content((type)(p0 | p1 << 1)) {}

	bool empty() const
		{ return content == NONE; }
	bool player() const
		{ return content == WHITE; }

	bool operator==( const Stone &rhs ) const = default;

	friend std::ostream &operator<<(std::ostream &os, const Stone &cell) {
			switch (cell.content) {
				case NONE: os << "┼"; break;
				case BLACK: os << "○"; break;
				case WHITE: os << "●"; break;
				default: os << "?";
			}
			return os;
		}
};
