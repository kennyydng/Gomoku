
#pragma once

#include "macros.hpp"

#include <meta>

template<size_t I, class T>
struct _ {
	using type = T;
	constexpr static size_t number = I;
	constexpr _(Take t): value(forward(t)) {}

	type value;
	constexpr operator type const &() const { return value; }
	constexpr operator type &() { return value; }
};

template<typename T>
concept numbered = requires(T t) {
		typename T::type; T::number;
		{ t.value } -> std::same_as<typename T::type &>;
	};
template<typename T, auto Req>
concept numbered_like = numbered<T> &&
	requires { Req.template operator()<typename T::type>(); };

template<numbered ...A>
struct pack : A... {
	constexpr pack(Take ...a): A(forward(a))... {}

	template<size_t I>
	constexpr auto &get(this auto &&self) TO( self.A...[I]::value )
};

template<numbered ...A>
struct std::tuple_size<pack<A...>> : std::integral_constant<size_t,sizeof...(A)> {};
template<size_t I, numbered ...A>
struct std::tuple_element<I,pack<A...>> { using type = std::decay_t<typename A...[I]::type>; };

consteval static std::vector<std::meta::info> repack_args_m(std::vector<std::meta::info> Ts) {
		for (size_t i = 0; i < Ts.size(); i++)
			Ts[i] = substitute(^^_, {std::meta::reflect_constant(i),Ts[i]});
		return Ts;
	};
template<std::meta::info Template, class ...Ts>
using repack_t = [: substitute(Template, repack_args_m({^^Ts...})) :];

template<typename ...T>
constexpr auto make_pack(T take...t) { return repack_t<^^pack, T...>(forward(t)...); }

template<typename T>
concept structural =
	 std::is_structural_v<T> && 
	!std::is_pointer_v<T> && 
	!std::is_lvalue_reference_v<T> && 
	!std::is_const_v<T> && 
	!std::is_volatile_v<T> && 
	!std::is_void_v<T>;

template<typename T>
static consteval auto make_static_object(T const &val)
-> decltype(auto) {
		T copy = val;
		return (T const (&)[1])*std::define_static_object(copy);
	}
template<typename T, size_t N>
static consteval auto make_static_array(T const (&val)[N])
-> decltype(auto) {
		static constexpr auto [...I] = std::make_index_sequence<N>();
		const T copy[N] = {val[I]...};
		return (T const (&)[N])*std::define_static_array(copy).data();
	}
template<size_t N>
static consteval auto make_static_string(char const (&val)[N])
-> decltype(auto) {
		static constexpr auto [...I] = std::make_index_sequence<N-1>();
		const char copy[N-1] = {val[I]...};
		return (char const (&)[N])*std::define_static_string(copy);
	}

template<size_t N, structural T> requires (N > 0)
struct static_array {
	using value_type 	 = T;
	using array_type 	 = T const (&)[N];
	using pointer    	 = T const [N];
	using reference  	 = T const &;
	array_type value;

	static constexpr size_t size() { return N; }

	consteval static_array( const static_array & ) = default;

	consteval static_array(reference obj) requires (N == 1)
		: static_array( nullptr, make_static_object(obj) ) {}
	consteval static_array(array_type arr)// requires (!std::same_as<T,char>)
		: static_array( nullptr, make_static_array(arr) ) {}
	consteval static_array(array_type str) requires (std::same_as<T,char>)
		: static_array( nullptr, make_static_string(str) ) {}

	consteval static_array(nullptr_t, pointer ptr)
		: value((array_type)*ptr) {}

	consteval operator reference() const requires (N == 1)
		{ return value[0]; }
	consteval operator array_type() const
		{ return value; }
};

template<static_array S>
static consteval void assert_static()
{ static_assert(&*S.value == &*make_static_array(S.value)); }


template<static_array S>
static consteval const char *operator""_static()
{ return S.value; }

template<structural T>
static_array(T const  &    ) -> static_array<1,T>;
template<size_t N, structural T>
static_array(T const (&)[N]) -> static_array<N,T>;

//template<const char *S>
//static constexpr size_t atom_length = []{size_t i = 0; while(S[i] != '\0') i++; return i;}();

struct functor;
template<std::derived_from<functor> F, class ...Ts>
struct term;

struct functor {
	constexpr auto operator()(this auto &&f, auto &&...ts)
		{ return term{forward(f), forward(ts)...}; }
};

template<size_t N, auto const *A>
struct atom : functor {
	using atom_type = std::decay_t<decltype(*A)>;
	using constant_type = static_array<N,atom_type>;

	static constexpr constant_type constant = {nullptr, (typename constant_type::array_type)*A};
	consteval { assert_static<constant>(); };

	consteval operator decltype(A)() const
		{ return A; }

	template<numbered ...Args>
	using term = pack<Args...>;
};

template<static_array S>
static constexpr auto operator""_atom() { return atom<S.size(), S.value>(); }

template<std::derived_from<functor> F, class ...Ts>
struct term : /*F,*/ repack_t<^^F::template term, Ts...> {
	static constexpr size_t arity = sizeof...(Ts);
	using functor = F;
	using args = repack_t<^^F::template term, Ts...>;

	constexpr term(F, Ts take ...ts): /*functor(forward(f)),*/ args({forward(ts)...}) {}
};

template<class F, class ...Ts>
term(F &&, Ts &&...) -> term<std::remove_reference_t<F>,Ts...>;

static_assert( requires { "test"_atom(1,2); } );
