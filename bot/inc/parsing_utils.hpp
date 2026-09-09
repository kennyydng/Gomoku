
#pragma once

#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string_view>

template<size_t N>
class Expect {
	char _expected[N];
public:
	constexpr Expect(const char (&expected)[N+1]) {
			for (size_t i = 0; i < N; i++)
				_expected[i] = expected[i];
		};

	friend std::istream &operator>>(std::istream &is, const Expect &e) {
			// _expected et actual ne sont PAS terminés par un zéro : les
			// afficher comme des char* lisait au-delà du tableau. Ce chemin
			// est celui de toute entrée malformée, donc il doit être sûr.
			// gcount() borne l'affichage à ce qui a réellement été lu, la
			// lecture pouvant s'arrêter avant N sur une entrée tronquée.
			char actual[N] = {};
			is.read(actual, N);
			const std::size_t got = (std::size_t)is.gcount();
			if (got != N || memcmp(e._expected, actual, N) != 0) {
				std::cerr << "Expected: " << std::string_view(e._expected, N) << '\n'
				          << "Actual: "   << std::string_view(actual, got) << std::endl;
				throw std::runtime_error("Unexpected input");
			}
			return is;
		};
};

template<size_t N>
Expect(const char (&expected)[N]) -> Expect<N-1>;
