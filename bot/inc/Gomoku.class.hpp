
#ifndef __GOMOKU_CLASS_HPP__
# define __GOMOKU_CLASS_HPP__

# include <iostream>
# include <cstdint>
# include <vector>
# include <cassert>
# include <random>

# include "utils.hpp"

# define SIZE 19

inline uint8_t &current_board_size() {
	static uint8_t size = SIZE;
	return size;
}

struct Rules {
	uint8_t boardSize;
	bool pass; // parsé pour garder l'alignement du protocole, jamais lu ici :
	           // le frontend résout pass/nulle avant d'appeler le bot.

	bool capture;
	bool captureUnperfect;

	struct {
		bool foulOverline;
		bool overline;

		bool threeThree;
		bool fourFour;
		bool flanking;
	} players[2];

	Rules(std::string str) {
			if (str.size() != 9)
				throw std::runtime_error("Invalid rules");

			if (str[0] == '5')
				boardSize = 15;
			else if (str[0] == '9')
				boardSize = 19;
			else
				throw std::runtime_error("Invalid grid size");
			current_board_size() = boardSize;

			pass                    	 = (str[1] == '1');
			capture                 	 = (str[2] == '1');
			captureUnperfect        	 = (str[3] == '1');
			players[0].foulOverline 	 = (str[4] == '1'); players[1].foulOverline 	 = (str[4] ==  '1' || str[4] ==  'b');
			players[0].overline     	 = (str[5] == '1'); players[1].overline     	 = (str[5] ==  '1' || str[5] ==  'b');
			players[0].threeThree   	 = (str[6] == '1'); players[1].threeThree   	 = (str[6] ==  '1' || str[6] ==  'b');
			players[0].fourFour     	 = (str[7] == '1'); players[1].fourFour     	 = (str[7] ==  '1' || str[7] ==  'b');
			players[0].flanking     	 = (str[8] == '1'); players[1].flanking     	 = (str[8] ==  '1' || str[8] ==  'b');
		}
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
		{ return content == NONE; };
	bool player() const
		{ return content == WHITE; };

	bool operator==( const Stone &rhs ) const = default;

	friend std::ostream &operator<<(std::ostream &os, const Stone &cell) {
			switch (cell.content) {
				case NONE: os << "┼"; break;
				case BLACK: os << "○"; break;
				case WHITE: os << "●"; break;
				default: os << "?";
			}
			return os;
		};
};

struct Pos {
	int8_t x;
	int8_t y;

	bool valid() const
		{ return uint8_t(x) < current_board_size() && uint8_t(y) < current_board_size(); };

	Pos operator+( this Pos lhs, Pos rhs )
		{ return {int8_t(lhs.x + rhs.x), int8_t(lhs.y + rhs.y)}; };
	Pos operator-( this Pos lhs, Pos rhs )
		{ return {int8_t(lhs.x - rhs.x), int8_t(lhs.y - rhs.y)}; };
	Pos operator*( this Pos lhs, int8_t rhs )
		{ return {int8_t(lhs.x * rhs  ), int8_t(lhs.y * rhs  )}; };

	friend std::istream &operator>>(std::istream &is, Pos &pos) {
			short x,y;
			is >> x >> Expect(":") >> y;
			pos = {(int8_t)x,(int8_t)y};
			return is;
		};
	friend std::ostream &operator<<(std::ostream &os, const Pos &pos)
		{ return os << (int)pos.x << ":" << (int)pos.y; };
};

constexpr Pos DIRECTIONS[4] = {
		{ 1, 0}, { 0, 1},
		{ 1, 1}, { 1,-1},
	};
constexpr Pos SUBDIRECTIONS[8] = {
		{-1,-1},{ 0,-1},{ 1,-1},
		{-1, 0},        { 1, 0},
		{-1, 1},{ 0, 1},{ 1, 1},
	};

inline int runLenOf(Pos start, Pos end, Pos dir)
	{ return dir.x ? (end.x - start.x) / dir.x + 1 : (end.y - start.y) / dir.y + 1; };
inline Pos stepPos(Pos p, Pos dir, int n)
	{ return p + dir * int8_t(n); };

// Hachage de Zobrist : une clé 64 bits par (case, couleur) + une pour le
// trait, XORées à chaque coup pour maintenir un hash de position incrémental
// (table de transposition). Seed fixe : runs reproductibles.
inline uint64_t zobristKey(Pos pos, bool player) {
	static uint64_t table[2][SIZE * SIZE];
	static bool initialized = false;
	if (!initialized) {
		std::mt19937_64 rng(0xC0FFEE);
		for (auto &colorTable : table)
			for (auto &key : colorTable)
				key = rng();
		initialized = true;
	}
	return table[player][pos.y * SIZE + pos.x];
}

inline uint64_t zobristSideKey() {
	static const uint64_t key = std::mt19937_64(0xFACADE)();
	return key;
}

enum class ThreatType : uint8_t { None, O3, C4, FourFour, O4, Five, Overline };

struct Threat {
	ThreatType type = ThreatType::None;
	Pos start{};
	Pos end{};
	Pos dir{};
};

enum class Result : uint8_t { Ongoing, Win, Draw };

struct Outcome {
	Result state = Result::Ongoing;
	bool winner = false;
};

class BitBoard {
	static constexpr size_t X = SIZE;
	static constexpr size_t Y = SIZE;

	using WORD = unsigned long long;
	static constexpr size_t WORD_BITS = 8*sizeof(WORD);
	static constexpr size_t LINES_PER_WORD = WORD_BITS / X;
	static_assert(LINES_PER_WORD >= 3);
	static constexpr size_t WORD_COUNT = (LINES_PER_WORD+Y-1) / LINES_PER_WORD;
	static_assert(WORD_COUNT <= 8);

	WORD words[WORD_COUNT] = {};

	constexpr WORD wordof(const Pos &pos) const
		{ return words[pos.y / LINES_PER_WORD]; };
	constexpr WORD &wordof(const Pos &pos)
		{ return words[pos.y / LINES_PER_WORD]; };

	constexpr size_t bitof(const Pos &pos) const
		{ return pos.x + (pos.y % LINES_PER_WORD) * X; };
public:
	bool operator[](Pos pos) const
		{
			assert(pos.valid());
			return 1ull & wordof(pos)>>bitof(pos);
		};
	BitBoard &operator+=(Pos pos)
		{ wordof(pos) |= 1ull<<bitof(pos); return *this; };
	BitBoard &operator-=(Pos pos)
		{ wordof(pos) &= ~(1ull<<bitof(pos)); return *this; };
};

struct Move {
	Pos pos;
	uint16_t captures;
	bool delayedWinBefore = false;
	Pos delayedLine[2] = {};
	Pos delayedDir = {};
	bool delayedPlayerBefore = false;
};

class Gomoku {
public:
	Gomoku(const Rules &rules)
		: rules(rules), moves(), captures(0,0), stones() {};
	~Gomoku() {};

	unsigned turn() const
		{ return moves.size(); };
	bool player() const
		{ return turn() % 2; };
	unsigned score(unsigned player) const
		{ return captures[player]; };
	bool captureRule() const
		{ return rules.capture; };
	uint64_t zobrist() const
		{ return hash; };

	static void forall(auto &&f) {
			for (int8_t y = 0; y < (int8_t)current_board_size(); y++)
				for (int8_t x = 0; x < (int8_t)current_board_size(); x++)
					f(Pos{x,y});
		}

	Stone stone( Pos pos ) const
		{ return {stones[0][pos], stones[1][pos]}; };

	void play(Pos move);
	void undo();

	// Menaces, légalité, victoire/nulle. threatAt/getThreats supposent que
	// `pos` contient déjà la pierre de `player`.
	void rawPlace(Pos pos, bool player)
		{ stones[player] += pos; };
	void rawRemove(Pos pos, bool player)
		{ stones[player] -= pos; };

	Pos runStart(Pos pos, Pos dir, bool player) const;
	Pos runEnd(Pos pos, Pos dir, bool player) const;

	Threat threatAt(Pos pos, Pos dir, bool player, int min = 3);
	Threat threatOf(Pos pos, Pos dir, bool player, int min);
	std::vector<Threat> getThreats(Pos pos, bool player, int min = 3);

	bool isUnperfect5(Pos start, Pos end, Pos dir, bool player);
	bool wouldCapture(Pos move, bool player);
	bool isLegalMove(Pos pos, bool player);

	Outcome applyMove(Pos pos);

private:

	const Rules rules;
	std::vector<Move> moves;
	unsigned captures[2];

	BitBoard stones[2];

	uint64_t hash = 0;

	bool delayedWin = false;
	Pos delayedStart{};
	Pos delayedEnd{};
	Pos delayedDir{};
	bool delayedPlayer = false;
};

std::ostream &operator<<(std::ostream &o, Gomoku const &gomoku);

#endif
