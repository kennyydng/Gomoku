
#ifndef __UTILS_HPP__
# define __UTILS_HPP__

# include <cstring>
# include <iostream>

template<size_t N>
class Expect {
	char _expected[N];
public:
	Expect(const char (&expected)[N+1]) {
			for (size_t i = 0; i < N; i++)
				_expected[i] = expected[i];
		};
	~Expect() {};

	friend std::istream &operator>>(std::istream &is, const Expect &e) {
			char actual[N];
			is.read(actual, N);
			if (memcmp(e._expected, actual, N) != 0) {
				std::cerr << "Expected: " << e._expected << std::endl;
				std::cerr << "Actual: " << actual << std::endl;
				throw std::runtime_error("Unexpected input");
			}
			return is;
		};
};

template<size_t N>
Expect(const char (&expected)[N]) -> Expect<N-1>;

#endif
