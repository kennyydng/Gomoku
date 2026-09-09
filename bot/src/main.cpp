
#include <iostream>
#include <cstdlib>

#include "engine_state.hpp"
#include "vcf.hpp"

// Protocole (identique à celui de la branche main, pour que l'app web et les
// outils de mesure fonctionnent sans modification) :
//   - une ligne de règles
//   - puis l'historique, un `|x:y` par coup
// Sortie : l'historique rejoué puis `|x:y`, le coup choisi, sur stdout.
//          L'état de la partie et les diagnostics sur stderr.
//
// Format de la ligne de règles (identique à main) : 6 caractères — taille de
// grille, capture, capture de fin de partie, overline, double-trois,
// double-quatre. Lus par Rules, qui applique exactement les mêmes conventions
// que la branche main : les deux moteurs doivent jouer au même jeu.

// stdin est une frontière : rien de ce qui en vient n'est digne de confiance.
// Le sujet est catégorique — un programme qui plante ne vaut rien — et la
// validation est donc faite ICI, une fois par coup d'historique, plutôt que
// dans play(), appelée des centaines de milliers de fois par recherche avec
// des coups que le moteur a lui-même engendrés et qui sont valides par
// construction. Le corps est un function-try-block : Rules et les opérateurs
// de lecture lèvent, et une exception qui s'échappe de main() appelle
// std::terminate.
int main() try {
	std::string rulesLine;
	if (!std::getline(std::cin, rulesLine)) {
		std::cerr << "Entree vide : une ligne de regles est attendue" << std::endl;
		return 1;
	}

	EngineState state{Rules{rulesLine}};

	Pos move;
	char c;
	while (std::cin >> c && c == '|') {
		if (!(std::cin >> move)) {
			std::cerr << "Coup illisible dans l'historique" << std::endl;
			return 1;
		}
		// Pos::valid() borne aussi les négatifs : x et y sont comparés en
		// unsigned, donc -3 devient énorme et sort. Sans ce test, un `|99:99`
		// écrit hors des bitboards.
		if (!move.valid() || !state.game.onBoard(move)) {
			std::cerr << "Coup hors du plateau : " << move << std::endl;
			return 1;
		}
		if (!state.game.stone(move).empty()) {
			std::cerr << "Case deja occupee : " << move << std::endl;
			return 1;
		}
		std::cout << move;
		state = state.after(move);
	}

	std::cerr << state.game << std::endl;

	// L'état après rejeu de l'historique. Un pilote externe en a besoin après
	// CHAQUE coup : le moteur ne connaît que la position courante, donc
	// empiler un coup par-dessus une victoire la masquerait définitivement.
	int w = state.winner();
	if (w >= 0)
		std::cerr << "Result: win " << w << std::endl;
	else
		std::cerr << "Result: ongoing" << std::endl;

	// Mode "résultat seul" : permet d'interroger l'état sans payer une
	// recherche complète.
	if (w >= 0 || getenv("RESULT_ONLY"))
		return 0;

	auto start = std::chrono::steady_clock::now();

	// VCF avant la recherche principale : une victoire forcée par menaces est
	// exacte, il n'y a rien de mieux à chercher. Le coût est borné par un
	// plafond de nœuds, donc sans risque pour le budget de temps.
	vcf::Budget budget;
	// Le coup rendu par le VCF est joué directement, il doit donc être légal.
	// Le VCF ne filtre pas la légalité à l'intérieur de sa recherche (elle y
	// coûterait une détection de menaces par nœud) : on valide la sortie, et
	// on retombe sur la recherche normale si elle est illégale. On perd alors
	// une victoire forcée rare ; on ne joue jamais un coup interdit.
	auto forced = vcf::find(state, budget);
	if (forced && !state.game.isLegalMove(*forced, state.player()))
		forced.reset();
	if (forced) {
		std::cerr << "VCF: victoire forcee en " << budget.nodes
			<< " noeuds -> " << *forced << std::endl;
		std::cout << "|" << *forced;
		return 0;
	}
	std::cerr << "VCF: rien (" << budget.nodes << " noeuds)" << std::endl;

	search::Limits limits;               // 460ms, plafond 800ms, >= 10 plis
	search::Searcher<EngineState> searcher(limits);

	// Légalité : garantie sur le coup RÉELLEMENT joué, donc à la racine
	// seulement. La revérifier à chaque nœud coûterait une détection de
	// menaces récursive par candidat, des centaines de milliers de fois — et
	// n'apporterait rien, puisque la recherche ne fait qu'estimer.
	if (state.game.rules().restricted(state.player())) {
		std::vector<Pos> legal;
		for (Pos p : state.candidates())
			if (state.game.isLegalMove(p, state.player()))
				legal.push_back(p);
		if (!legal.empty())
			searcher.setRootMoves(std::move(legal));
	}

	auto best = searcher.search(state);
	auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now() - start).count();

	std::cerr << "Depth reached: " << searcher.stats().depth
		<< " | Nodes: " << searcher.stats().nodes
		<< " | Search time: " << elapsed << "ms"
		<< (searcher.stats().aborted ? " (interrompue)" : "")
		<< std::endl;

	if (best)
		std::cout << "|" << *best;
	else
		std::cerr << "Aucun coup jouable" << std::endl;
	return 0;
} catch (std::exception const &e) {
	std::cerr << "Erreur : " << e.what() << std::endl;
	return 1;
} catch (...) {
	std::cerr << "Erreur inconnue" << std::endl;
	return 1;
}
