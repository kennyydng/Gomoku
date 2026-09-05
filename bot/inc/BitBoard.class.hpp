
#pragma once

#include "vec.hpp"
#include "gomoku_types.hpp"

#include <bit>

using namespace sugar;

struct ediv_t { int quot; int rem; };

constexpr ediv_t ediv(pos_t const num, pos_t const div) {
		pos_t rem = num % div;
		rem += (rem < 0) * div;
		ediv_t ret = {(num - rem) / div, rem};
		contract_assert(ret.rem >= 0 );
		contract_assert(ret.rem < div);
		contract_assert(ret.quot * div + ret.rem == num);
		return ret;
	}

struct word {
	using value_type = uint64_t;

	constexpr word() = default;
	constexpr word(value_type value): value(value) {}

private:
	value_type value;

public:

	friend constexpr int countr_zero(word let self)
		{ return std::countr_zero(self.value); }
	friend constexpr int popcount(word let self)
		{ return std::popcount(self.value); }

	explicit constexpr operator bool() const
		{ return value; }

	constexpr word operator~() const
		{ return ~value; }

	friend constexpr word operator&(word lhs, word rhs)
		{ return lhs.value & rhs.value; }
	friend constexpr word operator|(word lhs, word rhs)
		{ return lhs.value | rhs.value; }
	friend constexpr word operator-(word lhs, word rhs)
		{ return lhs.value - rhs.value; }
	friend constexpr word operator^(word lhs, word rhs)
		{ return lhs.value ^ rhs.value; }

	constexpr word operator<<(int shift) const
		{ return value << shift; }
	constexpr word operator>>(int shift) const
		{ return value >> shift; }

	constexpr word &operator&=(word rhs)
		{ return value &= rhs.value, *this; }
	constexpr word &operator|=(word rhs)
		{ return value |= rhs.value, *this; }
	constexpr word &operator-=(word rhs)
		{ return value -= rhs.value, *this; }
	constexpr word &operator^=(word rhs)
		{ return value ^= rhs.value, *this; }
};

using vec::operator~;

template<size_t AX=0>
class BitBoard : public auto_range {
	static constexpr Pos AXIS = AXES[AX];
	static constexpr bool FLIPPED = AX&1;
	static constexpr bool DIAGONAL = AX&2;

	static constexpr pos_t X = SIZE;
	static constexpr pos_t Y = SIZE;
	static constexpr size_t WORD_BITS = 8*sizeof(word);

	// Y_PER_WORD extra bits necessary for diagonal masks, fortunately still optimal for both 15 and 19 variants
	static constexpr size_t Y_PER_WORD = WORD_BITS / (Y+1);
	static constexpr size_t X_PER_WORD = Y_PER_WORD * X;

	static constexpr size_t WORD_COUNT = (Y_PER_WORD+Y-1) / Y_PER_WORD;
	static constexpr size_t LAST_WORD = WORD_COUNT-1;

	static constexpr size_t Y_LAST_WORD = Y - Y_PER_WORD * LAST_WORD;
	static constexpr size_t X_LAST_WORD = Y_LAST_WORD * X;

	static constexpr Dir transform(Dir dir) {
			if constexpr (FLIPPED ) dir = {dir.y, -dir.x};
			if constexpr (DIAGONAL) dir = {dir.x, dir.y+dir.x};
			return dir;
		}
	static constexpr Dir untransform(Dir dir) {
			if constexpr (DIAGONAL) dir = {dir.x, dir.y-dir.x};
			if constexpr (FLIPPED ) dir = {-dir.y, dir.x};
			return dir;
		}

	static constexpr Pos transform(Pos pos) {
			if constexpr (FLIPPED ) pos = {pos.y, X-1-pos.x};
			if constexpr (DIAGONAL) pos = {pos.x, pos.y+pos.x};
			return pos;
		}
	static constexpr Pos untransform(Pos pos) {
			if constexpr (DIAGONAL) pos = {pos.x, pos.y-pos.x};
			if constexpr (FLIPPED ) pos = {X-1-pos.y, pos.x};
			return pos;
		}

	static constexpr Pos flatten(Pos pos) {
			if constexpr (DIAGONAL)
				return {pos.x, pos.y - Y*(pos.y >= Y)};
			else return pos;
		}
	static constexpr Pos unflatten(Pos pos) {
			if constexpr (DIAGONAL)
				return {pos.x, pos.y + Y*(pos.y < 0)};
			else return pos;
		}

	using words_type = vec::data<word,WORD_COUNT>;
	using index_type = sets::part_of<WORD_COUNT>;
	using all_words = sets::all_of<WORD_COUNT>;
	using one_word = sets::one_of<WORD_COUNT>;
	using one_bit = size_t;

	static constexpr /*words_type*/ auto
		Y_MASK = vec::func([](int i){
			return (1ull << (i == LAST_WORD ? X_LAST_WORD : X_PER_WORD)) - 1ull;
		}, sets::all_of<WORD_COUNT>() );

	struct location { one_word word; one_bit bit; };
	static constexpr location locate(Pos pos) {
			Pos tpos = flatten(transform(pos));
			return {
				one_word( (tpos.y / Y_PER_WORD) ),
				one_bit ( (tpos.y % Y_PER_WORD) * X + tpos.x )
			};
		}

	constexpr void update_index()
		{ update_index(index); }
	constexpr void update_index(vec::index_for<words_type> Take where) {
			index_type empty{};
			for (auto i : where) {
				contract_assert(index_type::domain.has(i));
				if (!words[i]) empty |= i;
			}
			index ^= empty;
		}

	words_type words{};
	index_type index{};
public:
	constexpr BitBoard() = default;
	constexpr BitBoard(BitBoard let) = default;
	constexpr BitBoard(Pos pos) {
			auto loc = locate(pos);
			index = loc.word;
			words[loc.word] = 1ull << loc.bit;
		}
	constexpr BitBoard(bool val): words(val ? Y_MASK : words_type{}), index(val) {}
	explicit constexpr BitBoard(vec::vec_like Take words):
		words(words),
		index(words.vindex())
		{ update_index(); }

	constexpr auto get_words() const
		{ return vec::reindex(words,index); }

	constexpr bool operator[](Pos pos) const {
			auto loc=locate(pos);
			return !!(words[loc.word] & (1ull << loc.bit));
		}

	constexpr auto operator+(BitBoard let rhs) const { return operator+( rhs.get_words() ); }
	constexpr auto operator-(BitBoard let rhs) const { return operator-( rhs.get_words() ); }
	constexpr auto operator&(BitBoard let rhs) const { return operator&( rhs.get_words() ); }

	constexpr auto operator+(vec::vec_like Take rhs) const { return get_words() | forward(rhs); }
	constexpr auto operator-(vec::vec_like Take rhs) const { return get_words() &~forward(rhs); }
	constexpr auto operator&(vec::vec_like Take rhs) const { return get_words() & forward(rhs); }

	constexpr BitBoard &operator+=(BitBoard let rhs) { return operator+=( rhs.get_words() ); }
	constexpr BitBoard &operator-=(BitBoard let rhs) { return operator-=( rhs.get_words() ); }
	constexpr BitBoard &operator&=(BitBoard let rhs) { return operator&=( rhs.get_words() ); }

	constexpr BitBoard &operator+=(vec::vec_like Take rhs) {
			words |= rhs;
			index |= rhs.vindex();
			return *this;
		}
	constexpr BitBoard &operator-=(vec::vec_like Take rhs) {
			words ^= rhs & words;
			update_index(index & rhs.vindex());
			return *this;
		}
	constexpr BitBoard &operator&=(vec::vec_like Take rhs) {
			words  = vec::homogen(word(0), index & ~rhs.vindex());
			index &= rhs.vindex();
			words &= rhs;
			update_index(index);
			return *this;
		}

	friend constexpr int popcount(BitBoard let self)
		{ return sum(popcount( self.get_words() )); }

private:
	static constexpr auto
		line_mask(pos_t y) {
			auto x_start = 0;
			auto x_end   = X;
			if constexpr (DIAGONAL) {
				x_start = std::max(x_start,y+1-Y);
				x_end   = std::min(x_end  ,y+1  );
				y -= Y*(y >= Y);
			}
			auto start = (y % Y_PER_WORD) * X;
			return ((1ull << x_end) - (1ull << x_start)) << start;
		}

public:
	static constexpr auto make_line(Pos pos, int length) {
			auto loc = locate(pos);
			word line_word = (((1ull << length) - 1ull) << loc.bit) & line_mask(transform(pos).y);
			return vec::homogen{line_word, one_word(loc.word)};
		}

private:
	static constexpr auto
		shift_mask(Dir shift) { 
			// This is a mask of the begining bit of each line of a word
			static constexpr word XS = (natural_index<Y_PER_WORD>) >> []<size_t ...YI>
				{ return (0ull | ... | (1ull << (X*YI))); };
			static constexpr word XE = XS << X;

			if (FLIPPED) std::swap(shift.x, shift.y);

			// Due to contraints on shift.x, bits of x_start are always lesser then bits of x_end
			word x_start = XS << std::max(0, int(shift.x));
			word x_end   = XE >> std::max(0,-int(shift.x));
			word x_mask = (x_end - x_start);
			(void)x_mask;

			if constexpr (!DIAGONAL)
				return vec::func([x_mask](Take i) TO( x_mask & Y_MASK[i] ), all_words());

			if constexpr (DIAGONAL) {
				// This is a diagonal mask coresponding to the bondary between dual-lines of a word
				static constexpr word DS = (natural_index<Y_PER_WORD>) >> []<size_t ...YI>
					{ return (0ull | ... | (1ull << (X*YI+YI))); };
				// I would love to do this, but it requires 1 extra bit for the maximum value (1 << (X*(Y_PER_WORD+1)))
				// That bit matters as it may be left-shifted back into range later to provide the upper bond for d_mask
				// static constexpr DE = DS << X;

				static constexpr /*words_type*/ auto
					Y_MASK0 = vec::func([](Take i) TO( ((DS << (1+i*Y_PER_WORD)) - XS) & Y_MASK[i] ), sets::all_of<WORD_COUNT>() );
				static constexpr /*words_type*/ auto
					Y_MASK1 = vec::func([](Take i) TO( (XE - (DS << (1+i*Y_PER_WORD))) & Y_MASK[i] ), sets::all_of<WORD_COUNT>() );

				// Due to contraints on shift.x, bits of d_start are always lesser then bits of d_end
				// Additionally, contraints on Y_PER_WORD ensure that d_end fits inside the word
				word d_start = DS << std::max(0,            -int(shift.y) );
				word d_end   = DS << std::max(0,std::min(X,X-int(shift.y)));
				word d_mask = (d_end - d_start);

				return vec::func([x_mask,d_mask](auto i){
					int y = i*Y_PER_WORD;
					return x_mask &
						( ( (d_mask >>-(y+1-Y)) & Y_MASK0[i] )
						| ( (d_mask << (y+1  )) & Y_MASK1[i] )
						);
				}, all_words());
			}
		}

	static constexpr auto shl(auto x, pos_t s)
		{ return s >= 0 ? x << s : x >> -s; }

public:
	constexpr BitBoard shift(Dir amount) const {
			auto mask = shift_mask(amount);
			auto shift = transform(amount);

			BitBoard ret = ( shift.x >= 0
				? shift_y( get_words() <<  shift.x, shift.y)
				: shift_y( get_words() >> -shift.x, shift.y)
			);
			ret &= mask;
			ret.update_index();
			return ret;
		}

private:
	static constexpr BitBoard
		shift_y(auto words, pos_t amount) {
			auto index = words.vindex();
			BitBoard ret;

			auto y = ediv(amount, Y_PER_WORD);
			auto shift = X * y.rem;

			ret += vec::func([&](auto i) TO(
				words[ one_word(i - y.quot) ] << shift
			), shl(index, y.quot));
			ret += vec::func([&](auto i) TO(
				words[ one_word(i - y.quot - 1) ] >> (X_PER_WORD - shift)
			), shl(index, y.quot + 1));

			if constexpr (DIAGONAL) {
				y = ediv(amount + (amount < 0 ? Y : -Y), Y_PER_WORD);
				shift = X * y.rem;

				ret += vec::func([&](auto i) TO(
					words[ one_word(i - y.quot) ] << shift
				), shl(index, y.quot));
				ret += vec::func([&](auto i) TO(
					words[ one_word(i - y.quot - 1) ] >> (X_PER_WORD - shift)
				), shl(index, y.quot + 1));
			}
			ret.update_index();

			return ret;
		}

public:
	constexpr auto iterator   () const {
			auto i = std::begin(index);
			return make_pack( forward(i), i == std::end(index) ?  word() : words[*i] );
		}

	constexpr auto dereference() const TO( [ ](Let i, word let iword) {
			pos_t bit = countr_zero(iword);
			//pos_t y = bit / X;
			pos_t y = natural_index<pos_t(Y_PER_WORD - 1)> >> [&]<pos_t ...I>{
				return (0 + ... + pos_t(bit >= X+X*I));
			};
			return unflatten(untransform(Pos{
				bit - y*X,
				y + pos_t((*i)*Y_PER_WORD)
			}));
		} )
	constexpr auto increment  () const TO( [&](Var i, word var iword) {
			if (!(iword &= iword-1) && (++i != std::end(index)))
				iword = words[*i];
		} )
	constexpr auto sentinel   () const TO( [&](Let i, word let) TO( i == std::end(index) ))
	constexpr auto distance   () const TO( [&](Let i, word let iword) {
			size_t ret = popcount(iword);
			auto j = i;
			for (++j; j != std::end(index); ++j)
				ret += popcount(words[*j]);
			return ret;
		} )

	friend std::ostream &operator<<(std::ostream &os, BitBoard let bb) {
			for (int y = 0; y < Y; y++) {
				os << (bb.index.has(locate({0,y}).word) ? "+" : "-");
				for (int x = 0; x < X; x++)
					os << ' ' << (bb[{x,y}] ? 'X' : '.');
				os << '\n';
			}
			return os;
		}
};

template<size_t AX, size_t N>
class CountBoard {
	using digits_t = BitBoard<AX>[N];
	digits_t digits;

public:
	constexpr CountBoard(): digits() {}
	constexpr CountBoard(BitBoard<AX> let bb0): digits({bb0}) {}

	constexpr const BitBoard<AX> &of(size_t i) const
		{ return digits[i]; }

	constexpr void score(Take mask, Take updater) {
			template for (constexpr auto I : sugar::natural_index<N>)
				updater.template operator()<I>(
					sum(popcount(digits[I] & mask))
				);
		}

	constexpr void operator+=(Take mask) {
			contract_assert( !vec::any(digits[N-1] & mask) );
			for (size_t i = N-1; i > 0; i--) {
				digits[i] += (digits[i-1] & mask);
				digits[i-1] -= mask;
			}
		}

	constexpr void operator-=(Take mask) {
			contract_assert( !vec::any(digits[0] & mask) );
			for (size_t i = 0; i < N-1; i++) {
				digits[i] += (digits[i+1] & mask);
				digits[i+1] -= mask;
			}
		}
};
