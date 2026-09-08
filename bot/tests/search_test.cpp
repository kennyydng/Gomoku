// Vérifie la couche de recherche (inc/search.hpp) indépendamment du moteur.
//
// La branche bitboard exige un compilateur avec la réflexion C++26 ; ce test
// se compile en C++23 standard contre un état factice, ce qui permet de
// valider la logique de recherche sur n'importe quelle machine :
//
//     g++ -std=c++23 -O2 -Wall -Wextra -I inc tests/search_test.cpp -o /tmp/t && /tmp/t
//
// La propriété testée est celle qui compte : alpha-bêta, PVS et la table de
// transposition sont des optimisations, ils doivent rendre EXACTEMENT la même
// valeur qu'un minimax complet. Toute divergence est un bug d'élagage — le
// genre qui ne plante pas et se traduit juste par un bot qui joue mal.

#include "search.hpp"

#include <cassert>
#include <cstdio>
#include <random>

// --- Arbre de jeu factice, déterministe et reproductible ------------------
//
// Chaque nœud est identifié par un chemin ; la valeur d'une feuille dérive
// d'un hachage de ce chemin. Pas de plateau, pas de règles : on teste
// l'algorithme, pas le jeu.
struct FakeState {
	using move_t = int;

	std::uint64_t path = 1469598103934665603ull;
	int ply = 0;
	int branch = 4;
	// `terminal` = PARTIE FINIE, au sens de la recherche : elle y renvoie 0
	// (nulle) ou un score de mat, pas l'évaluation de la position. La limite
	// de profondeur est un concept distinct, porté par `depth`. Confondre les
	// deux fausse toute comparaison avec le minimax de référence.
	bool over = false;

	bool player() const { return ply % 2; }
	bool terminal() const { return over; }
	int winner() const { return -1; }          // nulle si terminal

	// Score du point de vue du joueur au trait (convention négamax).
	int evaluate() const {
		int raw = (int)(path % 2001) - 1000;
		return player() ? -raw : raw;
	}

	std::uint64_t hash() const { return path; }

	std::vector<move_t> candidates() const {
		if (terminal()) return {};
		std::vector<move_t> v(branch);
		for (int i = 0; i < branch; i++) v[i] = i;
		return v;
	}

	FakeState after(move_t m) const {
		FakeState s = *this;
		s.path = (s.path ^ (std::uint64_t)(m + 1)) * 1099511628211ull;
		s.ply++;
		return s;
	}
};

// Minimax complet, sans aucun élagage : la référence.
static int plainMinimax(FakeState const &s, int depth) {
	if (s.terminal() || depth == 0)
		return s.evaluate();
	int best = -search::INF;
	for (int m : s.candidates())
		best = std::max(best, -plainMinimax(s.after(m), depth - 1));
	return best;
}

// La recherche doit CHOISIR un coup optimal : la valeur minimax du coup
// qu'elle rend doit égaler celle de la position. C'est la propriété qui
// compte — alpha-bêta, PVS et la table sont des optimisations, ils n'ont pas
// le droit de changer la décision.
//
// (Comparer une valeur rendue par la recherche à plainMinimax ne testerait
// rien si on recalculait cette valeur avec plainMinimax : il faut passer par
// le coup effectivement choisi.)
static bool picksOptimalMove(FakeState const &s, int depth) {
	search::Limits lim;
	lim.maxBranch = 64;              // pas de coupe : arbre complet
	lim.budget = lim.hardBudget = std::chrono::hours(1);
	lim.minDepth = lim.maxDepth = depth;
	search::Searcher<FakeState> searcher(lim);

	auto chosen = searcher.search(s);
	if (!chosen)
		return false;

	int best = plainMinimax(s, depth);
	int got  = -plainMinimax(s.after(*chosen), depth - 1);
	return got == best;
}

static int checks = 0, failures = 0;

static void expectEqual(const char *what, int got, int want) {
	checks++;
	if (got != want) {
		failures++;
		std::printf("  ECHEC %-46s obtenu %d, attendu %d\n", what, got, want);
	}
}

int main() {
	std::printf("Recherche : alpha-beta/PVS/TT doivent egaler le minimax complet\n");

	// 1. Équivalence sur des arbres de formes variées.
	for (int branch : {2, 3, 5}) {
		for (int depth : {2, 3, 4, 5}) {
			FakeState s;
			s.branch = branch;
			char label[96];
			std::snprintf(label, sizeof label,
				"branchement %d, profondeur %d", branch, depth);
			expectEqual(label, picksOptimalMove(s, depth) ? 1 : 0, 1);
		}
	}

	// 2. Sur des graines différentes, pour ne pas valider un arbre chanceux.
	std::mt19937_64 rng(12345);
	for (int i = 0; i < 40; i++) {
		FakeState s;
		s.path = rng();
		s.branch = 3;
		expectEqual("arbre aleatoire", picksOptimalMove(s, 4) ? 1 : 0, 1);
	}

	// 3. La recherche doit toujours proposer un coup légal quand il en existe,
	//    et aucun sur une position terminale.
	{
		FakeState s;
		search::Searcher<FakeState> searcher;
		auto m = searcher.search(s);
		checks++;
		if (!m || *m < 0 || *m >= s.branch) {
			failures++;
			std::printf("  ECHEC %-46s coup hors bornes\n", "coup legal rendu");
		}

		FakeState done;
		done.over = true;                      // partie deja finie
		search::Searcher<FakeState> s2;
		checks++;
		if (s2.search(done).has_value()) {
			failures++;
			std::printf("  ECHEC %-46s coup rendu sur position terminale\n", "position terminale");
		}
	}

	// 4. Le budget de temps doit être respecté, même avec un arbre trop grand
	//    pour être épuisé.
	{
		FakeState s;
		s.branch = 8;
		search::Limits lim;
		lim.budget = lim.hardBudget = std::chrono::milliseconds(50);
		lim.minDepth = 1;
		lim.maxDepth = 30;
		search::Searcher<FakeState> searcher(lim);

		auto t0 = std::chrono::steady_clock::now();
		searcher.search(s);
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now() - t0).count();
		checks++;
		if (ms > 250) {                        // large : on teste l'ordre de grandeur
			failures++;
			std::printf("  ECHEC %-46s %lldms pour un budget de 50ms\n",
				"budget de temps respecte", (long long)ms);
		}
		std::printf("  (budget 50ms -> %lldms reels, profondeur %d, %lu noeuds)\n",
			(long long)ms, searcher.stats().depth, searcher.stats().nodes);
	}

	std::printf("%d verifications, %d echec(s)\n", checks, failures);
	return failures != 0;
}
