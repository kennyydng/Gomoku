// Diagnostic : la recherche annonce-t-elle des victoires qu'elle n'a pas ?
//
// Observé : depuis une position à 4 pierres, la recherche coupe à la
// profondeur 5 sur un score de mat après 387 nœuds. Trois causes possibles,
// aux conséquences très différentes :
//
//   A. Élagage des candidats — avec maxBranch=5, l'adversaire n'a que 5
//      défenses envisagées. La victoire serait réelle DANS L'ARBRE EXPLORÉ
//      mais pas sur le plateau. Élargir doit alors la faire disparaître.
//   B. Fuite via la table de transposition — un score de mat rangé à une
//      profondeur, réutilisé à une autre. Désactiver la table doit alors la
//      faire disparaître.
//   C. Victoire réellement forcée — elle survit aux deux.
//
// Le test joue ensuite la ligne annoncée en laissant l'adversaire se défendre
// avec une recherche LARGE : c'est le seul verdict qui compte.

#include "engine_state.hpp"

#include <cstdio>

static EngineState play(std::initializer_list<Pos> moves) {
	EngineState s;
	for (Pos p : moves)
		s = s.after(p);
	return s;
}

struct Probe {
	std::optional<Pos> move;
	int score;
	int depth;
	unsigned long nodes;
};

static Probe probe(EngineState const &s, std::size_t branch, bool tt, int maxDepth = 10) {
	search::Limits lim;
	lim.maxBranch = branch;
	lim.useTT = tt;
	lim.minDepth = 1;
	lim.maxDepth = maxDepth;
	lim.budget = lim.hardBudget = std::chrono::seconds(20);
	search::Searcher<EngineState> se(lim);
	auto m = se.search(s);
	return {m, se.stats().score, se.stats().depth, se.stats().nodes};
}

static const char *verdict(int score) {
	if (score >= search::MATE)  return "MAT ANNONCE";
	if (score <= -search::MATE) return "mat subi";
	return "position ordinaire";
}

int main() {
	// La position exacte où le comportement a été observé.
	EngineState s = play({ {9,9}, {9,10}, {10,8}, {8,9} });
	std::printf("Position a 4 pierres, joueur %d au trait\n\n", (int)s.player());

	std::printf("%-34s %12s %7s %9s  %s\n",
		"configuration", "score", "prof.", "noeuds", "verdict");

	struct Cfg { const char *name; std::size_t branch; bool tt; };
	Cfg cfgs[] = {
		{"defaut (branch 5, table on)",      5,  true },
		{"table desactivee",                 5,  false},
		{"branch 10",                       10,  true },
		{"branch 20",                       20,  true },
		{"branch 40 (quasi exhaustif)",     40,  true },
		{"branch 40, table desactivee",     40,  false},
	};
	for (Cfg const &c : cfgs) {
		Probe p = probe(s, c.branch, c.tt);
		std::printf("%-34s %12d %7d %9lu  %s\n",
			c.name, p.score, p.depth, p.nodes, verdict(p.score));
	}

	// Verdict pratique : jouer la ligne annoncee, l'adversaire se defendant
	// avec une recherche large. Si la victoire etait un artefact d'elagage,
	// elle ne survit pas.
	std::printf("\nDeroulement de la ligne annoncee (defense large, branch 40) :\n");
	EngineState cur = s;
	for (int ply = 0; ply < 10; ply++) {
		if (cur.terminal()) {
			std::printf("  -> partie finie, vainqueur = joueur %d\n", cur.winner());
			break;
		}
		bool attacker = (cur.player() == s.player());
		Probe p = probe(cur, attacker ? 5 : 40, true, attacker ? 8 : 6);
		if (!p.move) {
			std::printf("  -> plus de coup jouable\n");
			break;
		}
		std::printf("  ply %d : joueur %d joue %d:%d (score %d)\n",
			ply, (int)cur.player(), p.move->x, p.move->y, p.score);
		cur = cur.after(*p.move);
	}
	if (!cur.terminal())
		std::printf("  -> aucune victoire apres 10 plis (normal si aucun mat n'a ete\n"
		            "     annonce ci-dessus ; suspect si un score >= MATE apparait)\n");

	return 0;
}
