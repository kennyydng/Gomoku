
#define forward(x) std::forward<decltype(x)>(x)
#define let const &
#define Let auto const &
#define var &
#define Var auto &
#define take &&
#define Take auto &&
//#define args [](...)
#define REQUIRES(CONCEPT, ...) []__VA_ARGS__ requires CONCEPT{}

#define TO(...) { return __VA_ARGS__; }
#define AS(...) -> decltype(auto) { return (__VA_ARGS__); }

#define TRY_TO(...) requires requires { __VA_ARGS__; } { return __VA_ARGS__; }
#define TRY_AS(...) -> decltype(auto) requires requires { (__VA_ARGS__); } { return (__VA_ARGS__); }

#define NOINLINE [[gnu::noinline]]
#define INLINE [[gnu::always_inline]]
