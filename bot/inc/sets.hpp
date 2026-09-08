
#pragma once

#include <concepts>
#include <meta>

#include "sugar.hpp"
#include "macros.hpp"

namespace sets {
	template<class ...> struct domain_t;
	template<auto ...D> struct set;

	template<class D> concept is_domain = (has_template_arguments(decay(^^D)) && template_of(decay(^^D)) == ^^domain_t);
	template<class S> concept is_set = requires { { std::decay_t<S>::domain } -> is_domain; };

	template<is_set S> constexpr static auto domain_of = std::decay_t<S>::domain;
	template<is_set S> using domain_type = decltype(domain_of<S>);
	template<is_set S> using domain_value_type = domain_type<S>::value_type;
	template<is_set S> using domain_rank_type = domain_type<S>::rank_type;
	template<is_set S> using domain_set_type = [:domain_of<S>.set_type_m():];

	template<auto ...D> struct set {
		constexpr static auto domain = domain_t{D...};
		static_assert( std::same_as<set, domain_set_type<set>> );

		class none;
		class  all;
		class  opt;
		class  one;
		class part;
	};

	template<auto ...D> using  all_of = set<D...>::all;
	template<auto ...D> using part_of = set<D...>::part;
	template<auto ...D> using  one_of = set<D...>::one;
	template<auto ...D> using  opt_of = set<D...>::opt;
	template<auto ...D> using none_of = set<D...>::none;

	template<class S, auto ...D> concept set_of = domain_of<S> == domain_t{D...};
	template<class L, class R> concept compatible = domain_of<L> == domain_of<R>;

	template<class S> concept is_one = std::same_as<std::decay_t<S>, typename domain_set_type<S>::one>;
	template<class S> concept is_opt = std::same_as<std::decay_t<S>, typename domain_set_type<S>::opt>;

	//template<class,class> constexpr bool precedes = false;
	//template<auto ...D> constexpr bool precedes< all_of<D...>, part_of<D...>> = true;
	//template<auto ...D> constexpr bool precedes< all_of<D...>,  one_of<D...>> = true;
	//template<auto ...D> constexpr bool precedes< all_of<D...>,  opt_of<D...>> = true;
	//template<auto ...D> constexpr bool precedes< all_of<D...>, none_of<D...>> = true;
	//template<auto ...D> constexpr bool precedes<none_of<D...>, part_of<D...>> = true;
	//template<auto ...D> constexpr bool precedes<none_of<D...>,  one_of<D...>> = true;
	//template<auto ...D> constexpr bool precedes<none_of<D...>,  opt_of<D...>> = true;
	//template<auto ...D> constexpr bool precedes<part_of<D...>,  one_of<D...>> = true;
	//template<auto ...D> constexpr bool precedes<part_of<D...>,  opt_of<D...>> = true;
	//template<auto ...D> constexpr bool precedes< one_of<D...>,  opt_of<D...>> = true;

	constexpr auto operator|(Take l, compatible<decltype(l)> Take r) TRY_AS( forward(r).operator|( forward(l) ) )
	constexpr auto operator&(Take l, compatible<decltype(l)> Take r) TRY_AS( forward(r).operator&( forward(l) ) )
	constexpr auto operator^(Take l, compatible<decltype(l)> Take r) TRY_AS( forward(r).operator^( forward(l) ) )

	template<auto ...D> class set<D...>::all :
		public set, public sugar::auto_range
	{
		using rank_type = domain_rank_type<set>;

	public:
		consteval none operator~() const { return {}; };
		consteval auto operator|(this all, set_of<D...> Take    ) TO( all{} )
		constexpr auto operator^(this all, set_of<D...> Take rhs) TO( ~rhs )
		constexpr auto operator&(this all, set_of<D...> Take rhs) AS( rhs )

		consteval auto iterator   () const TO( rank_type() )

		constexpr auto dereference() const TO( [](rank_type let r) TO( set::one{ domain.nth_value(r) } ) )
		constexpr auto increment  () const TO( [](rank_type var r) { ++r; } )
		constexpr auto sentinel   () const TO( [](rank_type let r) TO( domain.size() == r ) )
		constexpr auto distance   () const TO( [](rank_type let r) TO( domain.size() -  r ) )
	};

	template<auto ...D> class set<D...>::none :
		public set, public sugar::auto_range
	{
	public:
		consteval all_of<D...> operator~() const { return {}; };
		constexpr auto operator|(this none, set_of<D...> Take lhs) AS( lhs )
		constexpr auto operator^(this none, set_of<D...> Take lhs) AS( lhs )
		consteval auto operator&(this none, set_of<D...> Take    ) TO( none{} )

		consteval auto iterator   () const TO( nullptr )

		constexpr auto dereference() const TO( [](nullptr_t) pre(false) TO( set::one{ domain.nth_value(0) } ) )
		constexpr auto increment  () const TO( [](nullptr_t) {} )
		constexpr auto sentinel   () const TO( [](nullptr_t) TO( true ) )
		constexpr auto distance   () const TO( [](nullptr_t) TO( 0 ) )
	};

	template<auto ...D> class set<D...>::one :
		public set, public sugar::auto_range
	{
		using value_type = domain_value_type<set>;
		value_type value;
	public:

		constexpr one(one let) = default;
		//constexpr one(): value() { static_assert(domain.has(value_type())); }
		constexpr explicit one(value_type value): value( domain.checked(value) ) {}

		//constexpr bool operator==(one rhs) const { return value == rhs.value; }
		constexpr operator value_type() const TO( domain.assumed(value) )

		constexpr set::part operator~() const { return ~set::part(value); };

		consteval auto iterator   () const TO( true )

		constexpr auto dereference() const TO( [&](bool  ) TO( *this ) )
		constexpr auto increment  () const TO( [ ](bool var i) { i = false; } )
		constexpr auto sentinel   () const TO( [ ](bool i) TO( !i ) )
		constexpr auto distance   () const TO( [ ](bool i) TO(  i ) )
	};

	template<auto ...D> class set<D...>::opt :
		public set, public sugar::auto_range
	{
		using value_type = domain_value_type<set>;
		value_type value;
		bool has;
	public:

		constexpr explicit opt()
			: has(false) {}
		constexpr explicit opt(one value)
			: has(true), value(value) {}
		constexpr explicit opt(auto &&init, bool has)
			: has(has) { if (has) value = forward(init); }

		//constexpr bool operator==(opt rhs) const TO( has == rhs.has && (!has || value == rhs.value) )
		constexpr explicit operator bool() const TO( has )

		constexpr auto operator*() const { contract_assert(has); return one(domain.assumed(value)); }

		//constexpr set::part operator~() const { return ~(has ? set::part(value_type(*this)) : set::part()); };

		constexpr auto iterator   () const TO( has )

		constexpr auto dereference() const TO( [&](bool  ) TO( operator*() ) )
		constexpr auto increment  () const TO( [ ](bool var i) { i = false; } )
		constexpr auto sentinel   () const TO( [ ](bool i) TO( !i ) )
		constexpr auto distance   () const TO( [ ](bool i) TO(  i ) )
	};

	template<auto ...D> struct set<D...>::part :
		set, public sugar::auto_range
	{
		using value_type = domain_value_type<set>;
		static constexpr auto Count = domain.size();

	private:
		using bitset = uint64_t;
		static_assert( Count <= std::numeric_limits<bitset>::digits );
		constexpr static bitset mask = (bitset(1) << Count) - bitset(1);
		bitset _bits = 0;

		constexpr part(bitset bits,int) : _bits(bits) {}
	public:

		constexpr part() = default;
		constexpr explicit part(set::none) {}
		constexpr explicit part(set::all) : _bits(mask) {}
		constexpr explicit part(bool all) : _bits(all ? mask : 0) {}
		constexpr part(one e) { _bits = bitset(1) << domain.rank_of(e); }
		constexpr part(opt e) { if (e) _bits = bitset(1) << domain.rank_of(*e); }

		//	constexpr bool operator==(sentinel) const { return _bits; }
		//	constexpr iterator &operator++() { _bits &= _bits-1; return *this; }
		//	constexpr value_type operator*() const {
		//			value_type ret = std::countr_zero(_bits);
		//			[[assume(ret < Count)]];
		//			return domain.value_of(ret);
		//		}

		constexpr auto iterator   () const TO( *this )

		constexpr auto dereference() const TO( [](part let self) TO( self.first() ) )
		constexpr auto increment  () const TO( [](part var self) { self._bits &= self._bits-1; } )
		constexpr auto sentinel   () const TO( [](part let self) TO( !self ) )
		constexpr auto distance   () const TO( [](part let self) TO( self.size() ) )

		//constexpr static set::none none = {};
		//constexpr static set::all  all = {};

		friend std::ostream &operator<<(std::ostream &os, part self) {
				for (auto i : set::all()) os << (self.has(i) ? "1" : "0");
				return os;
			}

		constexpr operator bool() const TO( _bits )
		constexpr bool has(one e) const TO( _bits & bitset(1) << domain.rank_of(e) )

		constexpr bool operator==(part rhs) const TO( _bits == rhs._bits )

		constexpr auto first() const TO( set::one{ domain.nth_value(std::countr_zero(_bits)) } )
		constexpr auto size() const TO( std::popcount(_bits) )

		constexpr part operator~() const TO( {mask ^ _bits, 0} )
		constexpr part operator<<(int rhs) const TO( {mask & (_bits << rhs), 0} )
		constexpr part operator>>(int rhs) const TO( {mask & (_bits >> rhs), 0} )

		constexpr auto operator&(this part lhs, one rhs) TO( opt( rhs,        lhs.has( rhs)) )
		constexpr auto operator&(this part lhs, opt rhs) TO( opt(*rhs, rhs && lhs.has(*rhs)) )

		constexpr part operator&(this part lhs, part rhs) TO( {lhs._bits & rhs._bits, 0} )
		constexpr part operator|(this part lhs, part rhs) TO( {lhs._bits | rhs._bits, 0} )
		constexpr part operator^(this part lhs, part rhs) TO( {lhs._bits ^ rhs._bits, 0} )

		constexpr part &operator|=(part rhs)  { _bits |= rhs._bits; return *this; }
		constexpr part &operator|=(set::all)  { _bits = mask; return *this; }
		constexpr part &operator|=(set::none) { return *this; }

		constexpr part &operator^=(part rhs)  { _bits ^= rhs._bits; return *this; }
		constexpr part &operator^=(set::all)  { _bits ^= mask; return *this; }
		constexpr part &operator^=(set::none) { return *this; }

		constexpr part &operator&=(part rhs)  { _bits &= rhs._bits; return *this; }
		constexpr part &operator&=(set::all)  { return *this; }
		constexpr part &operator&=(set::none) { _bits = 0; return *this; }
	};

	template<std::integral U>
		class domain_t<U>
	{
		U count;
	public:
		using value_type = U;
		using rank_type = value_type;

		consteval domain_t(value_type count) : count(count)
			{ contract_assert(U() <= count); };

		consteval auto size() const { return count; }
		consteval bool operator==(domain_t const &) const = default;
		consteval auto set_type_m(this domain_t domain)
			{ return substitute(^^set, {std::meta::reflect_constant(domain.count)}); }

		constexpr auto has(value_type v) const requires std::unsigned_integral<U>
			{ return v < count; }
		constexpr auto has(value_type v) const requires (!std::unsigned_integral<U>)
			{ return U() <= v && v < count; }

		constexpr value_type checked(value_type v) const { contract_assert(has(v)); return v; }
		constexpr value_type assumed(value_type v) const { [[assume(has(v))]]; return v; }

		constexpr rank_type rank_of(value_type v) const TO( checked(v) )
		constexpr value_type nth_value(rank_type v) const TO( checked(v) )
	};
	template<std::integral U>
		domain_t(U) -> domain_t<U>;
}
