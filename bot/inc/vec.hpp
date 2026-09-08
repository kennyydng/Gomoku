
#pragma once

#include <type_traits>

#include "sugar.hpp"
#include "sets.hpp"
#include "terms.hpp"

namespace vec {
	template<typename I, typename T>
	concept index_for_value =
		requires(I const i, T const t) {
			{ *std::begin(i) } -> std::convertible_to<T>;
			{ std::begin(i) == std::end(i) } -> std::same_as<bool>;
		};
	template<typename I, typename V>
	concept index_for = index_for_value<std::decay_t<I>, typename std::decay_t<V>::index_type>;

	template<typename V>
	concept vec_like_value = 
		requires(V const v, typename V::index_type const i) {
			{ v.vindex() } -> std::convertible_to<typename V::vindex_type>;
			{ v[i] } -> std::convertible_to<typename V::value_type>;
		} && index_for<typename V::vindex_type, V>;
	template<typename V>
	concept vec_like = vec_like_value<std::decay_t<V>>;

	template<vec_like Base, index_for<Base> Index>
	class reindex {
		Base _base;
		Index _idx;
	public:
		using vindex_type = Index;
		using index_type = sets::domain_set_type<Index>::one;
		using value_type = std::decay_t<Base>::value_type;

		constexpr reindex(Take base, Take idx) :
			_base(base), _idx(idx) {}

		template<index_for<Base> FromIndex>
		constexpr reindex(reindex<Base,FromIndex> &&from, Index idx) :
			_base(from._base), _idx(idx) {}

		INLINE constexpr auto operator[](this Take self, index_type i)
		-> decltype(auto) { return self._base[i]; }

		constexpr Index const &vindex() const
			{ return _idx; }
	};
	template<typename Base, typename Index>
	reindex(Base&&, Index&&) -> reindex<Base,std::decay_t<Index>>;

	template<typename Func, typename Index>
	class func {
		Func _f;
		Index _idx;
	public:
		using vindex_type = Index;
		using index_type = sets::domain_set_type<vindex_type>::one;
		static constexpr auto domain = sets::domain_of<vindex_type>;
		using value_type = std::invoke_result_t<Func,index_type>;

		constexpr func(Take f, Take idx) :
			_f(f), _idx(idx) {}

		constexpr Index const &vindex() const { return _idx; }
		constexpr auto operator[](this Take self, index_type i)
		-> decltype(auto) { return self._f(i); }

		static_assert(vec_like<func>);
	};
	template<typename Func, typename Index>
	func(Func &&, Index &&) -> func<std::decay_t<Func>,std::decay_t<Index>>;

	template<typename T, typename Index>
	class homogen {
		T _val = {};
		Index _idx;
	public:
		using vindex_type = Index;
		using index_type = sets::domain_set_type<vindex_type>::one;
		static constexpr auto domain = sets::domain_of<vindex_type>;
		using value_type = T;

		constexpr homogen(Take val, Take idx) :
			_val(val), _idx(idx) {}

		constexpr Index const &vindex() const { return _idx; }
		constexpr auto operator[](this Take self, index_type)
		-> decltype(auto) { return self._val; }

		static_assert(vec_like<homogen>);
	};
	template<typename T, typename Index>
	homogen(T &&, Index &&) -> homogen<std::decay_t<T>,std::decay_t<Index>>;

	template<typename T, size_t Count>
	class data {
		T _data[Count] = {};
	public:
		using vindex_type = sets::all_of<Count>;
		using index_type = sets::domain_set_type<vindex_type>::one;
		static constexpr auto domain = sets::domain_of<vindex_type>;
		using value_type = T;

		constexpr data() = default;
		constexpr data(vec_like Take v)
			{ for (auto i : v.vindex()) operator[](i) = v[i]; }

		INLINE constexpr T &operator[](index_type i) TO( _data[domain.rank_of(i)] )
		INLINE constexpr T operator[](index_type i) const TO( _data[domain.rank_of(i)] )

		constexpr static vindex_type vindex() { return {}; }

		constexpr void operator =(this auto &lhs, vec_like Take rhs) { for (auto i : rhs.vindex()) lhs[i]  = rhs[i]; }
		constexpr void operator|=(this auto &lhs, vec_like Take rhs) { for (auto i : rhs.vindex()) lhs[i] |= rhs[i]; }
		constexpr void operator&=(this auto &lhs, vec_like Take rhs) { for (auto i : rhs.vindex()) lhs[i] &= rhs[i]; }
		constexpr void operator^=(this auto &lhs, vec_like Take rhs) { for (auto i : rhs.vindex()) lhs[i] ^= rhs[i]; }

		static_assert(vec_like<data>);
	};

	constexpr auto compute(vec_like Take v) requires sets::is_one<decltype(v.vindex())> {
			auto idx = v.vindex();
			return homogen(v[idx], idx);
		}

	constexpr auto compute(vec_like Take v) requires sets::is_opt<decltype(v.vindex())> {
			using value_type = std::decay_t<decltype(v)>::value_type;
			auto idx = v.vindex();
			return homogen(idx ? v[*idx] : value_type(), idx);
		}

	template<char const *S>
	struct op : public functor {
		constexpr bool operator==(char const *Str)
			{ return S == Str; }

		INLINE constexpr static auto args_vindex(vec_like Let v) {
				     if constexpr (S == "popcount"_static) return v.vindex();
				else if constexpr (S == "~"_static       ) return v.vindex().all;
			};
		INLINE constexpr static auto args_vindex(vec_like Let v0, vec_like Let v1) {
				     if constexpr (S == "&"_static ) return v0.vindex() & v1.vindex();
				else if constexpr (S == "|"_static ) return v0.vindex() | v1.vindex();
				else if constexpr (S == "^"_static ) return v0.vindex() | v1.vindex();
				else static_assert(false, "Operator not found");
			};
		INLINE constexpr static auto args_vindex(vec_like Let v0, int) -> decltype(auto) {
				     if constexpr (S == "<<"_static) return v0.vindex();
				else if constexpr (S == ">>"_static) return v0.vindex();
				else static_assert(false, "Operator not found");
			};

		INLINE constexpr static auto args_compute(Let v, Take i)
		-> decltype(auto) {
				     if constexpr (S == "popcount"_static) return popcount(v[i]);
				else if constexpr (S == "~"_static       ) return ~v[i];
				else static_assert(false, "Operator not found");
			}
		INLINE constexpr static auto args_compute(Let v0, Let v1, Take i)
		-> decltype(auto) {
				     if constexpr (S == "&"_static ) return v0[i] & v1[i];
				else if constexpr (S == "|"_static ) return v0[i] | v1[i];
				else if constexpr (S == "^"_static ) return v0[i] ^ v1[i];
				else if constexpr (S == "<<"_static) return v0[i] << v1;
				else if constexpr (S == ">>"_static) return v0[i] >> v1;
				else static_assert(false, "Operator not found");
			}

		template<numbered ...Arg>
		struct term {
			pack<Arg...> args;

			INLINE constexpr auto vindex() const
				AS( args_vindex(args.Arg::value...) )

			using vindex_type = std::decay_t< decltype(std::declval<term>().vindex()) >;
			using index_type = sets::domain_set_type<vindex_type>::one;
			static constexpr auto domain = sets::domain_of<vindex_type>;

			INLINE constexpr auto operator[](index_type i) const
				AS( args_compute(args.Arg::value..., i) )

			using value_type = std::decay_t< decltype(std::declval<term>()[std::declval<index_type>()]) >;

			static_assert(vec_like<term>);
		};
	};

	template<static_array Str>
	constexpr auto operator""_op()
		{ return op<Str.value>{}; }

	constexpr auto popcount(vec_like Take arg) TO( "popcount"_op( forward(arg) ) )
	constexpr auto operator~(vec_like Take rhs) TO( "~"_op( forward(rhs) ) )

	constexpr auto operator&(vec_like Take lhs, vec_like Take rhs) TO( "&"_op( forward(lhs) , forward(rhs) ) )
	constexpr auto operator|(vec_like Take lhs, vec_like Take rhs) TO( "|"_op( forward(lhs) , forward(rhs) ) )
	constexpr auto operator^(vec_like Take lhs, vec_like Take rhs) TO( "^"_op( forward(lhs) , forward(rhs) ) )

	constexpr auto operator<<(vec_like Take lhs, auto rhs) TO( "<<"_op( forward(lhs) , forward(rhs) ) )
	constexpr auto operator>>(vec_like Take lhs, auto rhs) TO( ">>"_op( forward(lhs) , forward(rhs) ) )

	constexpr auto sum(vec_like Take v) {
			using value_type = std::decay_t<decltype(v)>::value_type;
			value_type sum = 0;
			for (auto i : v.vindex()) sum += v[i];
			return sum;
		}

	constexpr bool any(vec_like Take v) {
			for (auto i : v.vindex())
				if (v[i]) return true;
			return false;
		}
}
