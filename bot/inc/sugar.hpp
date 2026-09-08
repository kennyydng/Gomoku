
#pragma once

#include <meta>
#include <tuple>
#include <array>
#include <utility>

#include "terms.hpp"
#include "macros.hpp"

namespace sugar {
	using namespace std::meta;
	using namespace std::string_literals;

	template<auto N> requires (0 <= N)
	struct natural_index_t {
		using value_type = decltype(N);

		static constexpr value_type MIN = 0;
		static constexpr value_type MAX = N-1;

		class sentinel {};
		class iterator {
			value_type i;
		public:
			using difference_type = std::make_signed_t<value_type>;

			constexpr iterator(value_type i = 0): i(i) {}
			constexpr auto operator*()const { return i; }
			constexpr auto &operator++() TO(++i,*this)

			constexpr iterator
				operator+(difference_type n) const
				{ return iterator(i + n); }

			constexpr bool
				operator==(sentinel) const
				{ return i >= N; }
			friend constexpr difference_type
				operator-(sentinel, iterator const &rhs)
				{ return N - rhs.i; }

			constexpr bool
				operator==(this iterator const &lhs, iterator const &rhs)
				{ return lhs.i == rhs.i; }
			constexpr difference_type
				operator-(this Let lhs, iterator const &rhs)
				{ return lhs.i - rhs.i; }
		};

		using index_sequence = decltype(std::make_index_sequence<N>());
		consteval index_sequence sequence() const { return {}; }
		consteval operator index_sequence() const { return {}; }

		consteval bool operator==(natural_index_t){ return true; }

		static consteval auto begin() { return iterator(); }
		static consteval auto end()   { return sentinel(); }

		template<size_t I>
		static constexpr size_t get() { return I; }
		static consteval size_t size() { return N; }
	};

	template<auto N>
	constexpr auto natural_index = natural_index_t<N>{};

	template<typename Range> consteval auto index_of(Range const &range) { return natural_index<std::size(range)>; }
	template<typename Range> constexpr auto index_of_v = index_of(std::declval<Range>());

	template<typename Tuple> constexpr auto index_of_tuple_v = natural_index<std::tuple_size_v<Tuple>>;
	template<typename Tuple> consteval auto index_of_tuple(Tuple const&) { return index_of_tuple_v<Tuple>; }

	template<size_t N>
	constexpr auto make_array(Take f) {
			constexpr auto [...Ns] = natural_index<N>;
			return std::array{f(Ns)...};
		}
}

template<auto N>
struct std::tuple_size<sugar::natural_index_t<N>> : std::integral_constant<size_t,N> {};
template<size_t I, auto N>
struct std::tuple_element<I,sugar::natural_index_t<N>> { using type = size_t; };

//namespace sugar {
//	template<typename T, T ...V>
//	class type_sequence {
//		static constexpr size_t N = sizeof...(V);
//		static constexpr T values[N?N:1] = {V...};
//	public:
//		template<size_t I>
//		static consteval T get() { return values[I]; }
//		static consteval auto size() { return sizeof...(V); }
//
//		static consteval auto begin() { return values; }
//		static consteval auto end()   { return values + N; }
//	};
//	template<auto V0, decltype(V0) ...V>
//	constexpr auto type_sequence_v = type_sequence<decltype(V0),V0,V...>{};
//
//	template<info ...V>
//	using info_sequence = type_sequence<info,V...>;
//	template<info ...V>
//	constexpr auto info_sequence_v = info_sequence<V...>{};
//
//	template<Var Range>
//	constexpr auto range_sequence = []{
//			[[maybe_unused]] static constexpr auto [...I] = natural_index<std::size(Range)>;
//			return type_sequence<
//				typename std::decay_t<decltype(Range)>::value_type,
//				Range[I]...
//			>{};
//		}();
//}
//
//template<typename T, T ...V>
//struct std::tuple_size<sugar::type_sequence<T,V...>> : std::integral_constant<size_t,sizeof...(V)> {};
//template<size_t I, typename T, T ...V>
//struct std::tuple_element<I,sugar::type_sequence<T,V...>> { using type = T; };

namespace sugar {
	template<size_t I>
	constexpr auto get(Take tuple) TRY_AS( std::get<I>(forward(tuple)) )
	template<size_t I>
	constexpr auto get(Take tuple) TRY_AS( forward(tuple).template get<I>() )

	//static_assert(requires { make_term<"+">(); });

	namespace concepts {
		template<auto Range>
		consteval info requires_all_index(auto Req) {
				template for (constexpr auto N : Range)
					if constexpr (!requires {Req.template operator()<N>();})
						return reflect_constant(N);
				return reflect_constant(nullptr);
			};
		template<auto Range, auto Req>
		concept requires_all = requires {
				Req.template operator()<([: requires_all_index<Range>(Req) :])>();
			};

		template<auto N,typename T>
		concept _ValidTupleMember = std::same_as<decltype(N),nullptr_t> ||
			( std::same_as<decltype(N),size_t> && requires (T t) {
			  { get<N>(t) } -> std::convertible_to<std::tuple_element_t<N,T>>;
			} );

		template<typename T>
		concept tuple_like =
			requires { { std::tuple_size<std::decay_t<T>>::value } -> std::convertible_to<size_t>; } &&
			requires_all<index_of_tuple_v<std::decay_t<T>>,
				([]<auto I> requires _ValidTupleMember<I,std::decay_t<T>> {})
			>;

		template<typename I, typename S>
		concept iterator_and_sentinel =
			requires (I i, S s) {
				{ *i };
				{ ++i } -> std::same_as<I&>;
				{ i == s } -> std::convertible_to<bool>;
				{ s - i } -> std::integral;
			};

		template<typename R>
		concept range =
			requires (R r) {
				{ std::begin(r) }; { std::end(r) };
				requires iterator_and_sentinel<decltype(std::begin(r)), decltype(std::end(r))>;
			};
	};

	template<typename F, typename Pack, size_t ...I>
	consteval auto can_apply_template(F &&, Pack &&, std::index_sequence<I...>) {
			return []<auto=0>
				requires requires(F f, Pack pck)
				{ forward(f).template operator()<get<I>(pck)...>(); }
			{};
		}

	template<typename F, typename Pack, size_t ...I>
	INLINE consteval auto can_apply(F &&, Pack &&, std::index_sequence<I...>) {
			return []<auto=0>
				requires requires(F f, Pack pck)
				{ forward(f)(get<I>(pck)...); }
			{};
		}

	INLINE constexpr auto auto_apply(Take f, concepts::tuple_like Take pck) -> decltype(auto)
		requires requires { can_apply_template(f,pck,index_of_tuple(pck).sequence())(); } {
			[[maybe_unused]] static constexpr auto [...I] = index_of_tuple(pck);
			return forward(f).template operator()<get<I>(forward(pck))...>();
		}
	INLINE constexpr auto auto_apply(Take f, concepts::tuple_like Take pck) -> decltype(auto)
		requires requires { can_apply(f,pck,index_of_tuple(pck).sequence())(); } {
			[[maybe_unused]] static constexpr auto [...I] = index_of_tuple(pck);
			return forward(f)(get<I>(forward(pck))...);
		}

	static_assert( auto_apply( [](){return true;}, std::tuple{} ) );
	static_assert( auto_apply( [](size_t){return true;}, std::tuple{1} ) );

	INLINE constexpr auto operator>>(concepts::tuple_like Take pck, Take f)
		AS( auto_apply(forward(f), forward(pck)) );

	INLINE constexpr auto apply_or_call(Take f, Take pck) TRY_AS( auto_apply(forward(f), forward(pck)) )
	INLINE constexpr auto apply_or_call(Take f, Take pck) TRY_AS( forward(f)(forward(pck)) )

	class auto_range {
		class sentinel {};
		template<typename Range>
		class iterator {
			using Iter = decltype( std::declval<Range>().iterator() );

			Range range;
			Iter iter;
		public:
			constexpr iterator(Range take range): range(range), iter(range.iterator()) {}
			constexpr iterator(iterator const &) = default;

			constexpr iterator &operator=(iterator let other) {
					contract_assert(&range == &other.range); // since Range is a reference, it can't be copied
					iter = other.iter;
					return *this;
				}

			INLINE constexpr auto operator*() const
				AS( apply_or_call(range.dereference(), iter) )
			INLINE constexpr iterator & operator++()
				{ (void)apply_or_call(range.increment(), iter); return *this; }
			INLINE constexpr bool operator==(sentinel) const
				TO( apply_or_call(range.sentinel(), iter) )
			friend constexpr size_t operator-(sentinel, iterator let rhs)
				TO( apply_or_call(rhs.range.distance(), rhs.iter) )
			friend constexpr size_t operator-(iterator let lhs, iterator let rhs)
				TO( (sentinel{} - rhs) - (sentinel{} - lhs) )
		};

	public:
		constexpr auto begin(this Let self)
			{ return iterator<decltype(self)>{self}; }
		constexpr auto end(this Let)
			{ return sentinel(); }
	};

	constexpr struct range_product : public functor {
		template<typename R> using I = decltype( std::begin(std::declval<R>().value) );
		template<numbered ...R>
		class term : R..., public auto_range {
			static_assert(( concepts::range<typename R::type> && ... ));
			constexpr static class{} evaluate{};

			template<size_t I>
			constexpr bool _increment(Var i) const {
					++i;
					if constexpr (I+1 < sizeof...(R)) {
						if (i == std::end(R...[I]::value)) {
							i = std::begin(R...[I]::value);
							return false;
						}
					} return true;
				}
		public:
			constexpr term(R take ...r): R(r)... {}

			constexpr auto iterator   () const TO( make_pack( std::begin(R::value)... ) )

			constexpr auto dereference() const TO( [ ](I<R> let ...is) TO( make_pack( *is... ) ) )
			constexpr auto increment  () const TO( [&](I<R> var ...is) { ( _increment<R::number>(is) || ... ); } )
			constexpr auto sentinel   () const TO( [&](I<R> let ...is) AS( (is == std::end(R::value)) || ... ) )
			constexpr auto distance   () const TO( [&](I<R> let ...is) AS( _distance<R::number>(is) + ... + evaluate ) )
		};
	} product{};

	template<concepts::range R, class M>
		requires requires (R r, M m) { apply_or_call(m, *std::begin(r)); }
	struct map : auto_range {
		R r; M m;
		using It = decltype(std::begin(r));

		constexpr map(R take r, M take m): r(r), m(m) {}

		constexpr auto iterator   () const AS( std::begin(r) )

		constexpr auto dereference() const TO( [&](It let i) AS( apply_or_call(m, *i) ) )
		constexpr auto increment  () const TO( [ ](It var i) { ++i; } )
		constexpr auto sentinel   () const TO( [&](It let i) TO( std::end(r) == i ) )
		constexpr auto distance   () const TO( [&](It let i) AS( std::end(r) - i ) )
	};
	template<class R, class M>
	map(R take r, M take m) -> map<R,M>;

	constexpr auto operator|(Take r, Take m) TRY_TO( map{forward(r), forward(m)} )
}
