// Captures : détection, comptage, victoire — et surtout intégrité du score
// incrémental après retrait de pierres.
//
// take() est l'inverse de place(). Une erreur y serait SILENCIEUSE : le score
// dériverait progressivement de la réalité sans que rien ne plante, et le bot
// jouerait de plus en plus mal au fil de la partie. Il faut donc un oracle.
//
// L'oracle : le score est une somme sur les fenêtres, donc une fonction pure
// de la position. Un plateau reconstruit de zéro avec les mêmes pierres doit
// donner exactement la même valeur qu'un plateau où des pierres ont été
// posées puis capturées.

#include "engine_state.hpp"

#include <cstdio>
#include <vector>

static int checks = 0, failures = 0;

static void expect(const char *what, bool ok, const char *detail = "") {
	checks++;
	if (!ok) {
		failures++;
		std::printf("  ECHEC %-46s %s\n", what, detail);
	}
}

static Rules withCapture() { Rules r; r.capture = true; return r; }

// Rejoue les pierres présentes sur `ref` dans un plateau neuf, sans capture,
// pour obtenir la valeur de référence de cette position.
static long rebuiltScore(Gomoku const &ref) {
	std::vector<Pos> byPlayer[2];
	for (Pos const p : Pos::all()) {
		Stone s = ref.stone(p);
		if (!s.empty())
			byPlayer[s.player()].push_back(p);
	}
	// Le plateau reconstruit doit recevoir les pierres en alternance pour que
	// `player()` reste cohérent ; on désactive la capture pour qu'aucune
	// pierre ne disparaisse pendant la reconstruction.
	Gomoku fresh{Rules{}};
	std::size_t i = 0, j = 0;
	while (i < byPlayer[0].size() || j < byPlayer[1].size()) {
		if (i < byPlayer[0].size()) fresh.play(byPlayer[0][i++]);
		else                        fresh.pass();
		if (j < byPlayer[1].size()) fresh.play(byPlayer[1][j++]);
		else                        fresh.pass();
	}
	return fresh.heuristic().raw();
}

int main() {
	std::printf("Captures : detection, comptage, et integrite du score\n");

	// 1. Motif de capture : joueur 0 en (5,5) et (8,5), joueur 1 en (6,5) et
	//    (7,5). Le coup du joueur 0 en (8,5) doit prendre la paire.
	{
		Gomoku g{withCapture()};
		g.play({5,5});          // j0
		g.play({6,5});          // j1
		g.play({15,15});        // j0 ailleurs
		g.play({7,5});          // j1  -> paire (6,5),(7,5)
		g.play({8,5});          // j0  -> capture
		expect("paire capturee : (6,5) videe", g.stone({6,5}).empty());
		expect("paire capturee : (7,5) videe", g.stone({7,5}).empty());
		expect("compteur de captures = 2", g.captures(0) == 2);
		expect("l'adversaire n'a rien capture", g.captures(1) == 0);
		expect("score coherent apres capture",
			g.heuristic().raw() == rebuiltScore(g),
			"le score incremental a derive de la position reelle");
	}

	// 2. Sans la règle, aucune capture ne doit avoir lieu.
	{
		Gomoku g{Rules{}};
		g.play({5,5}); g.play({6,5}); g.play({15,15}); g.play({7,5}); g.play({8,5});
		expect("regle desactivee : pas de capture", !g.stone({6,5}).empty());
		expect("regle desactivee : compteur a zero", g.captures(0) == 0);
	}

	// 3. Le score doit rester exact après PLUSIEURS captures successives,
	//    puisque c'est là qu'une dérive s'accumulerait.
	{
		Gomoku g{withCapture()};
		const Pos seq[] = {
			{5,5},{6,5},{15,15},{7,5},{8,5},      // capture 1 (horizontale)
			{5,7},{6,8},{15,16},{7,9},{8,10},     // capture 2 (diagonale)
			{9,5},{10,5},{16,15},{11,5},{12,5},   // capture 3 (horizontale)
		};
		for (Pos p : seq)
			g.play(p);
		expect("score exact apres captures multiples",
			g.heuristic().raw() == rebuiltScore(g),
			"derive du score incremental");
		std::printf("  (captures : joueur 0 = %u, joueur 1 = %u)\n",
			g.captures(0), g.captures(1));
	}

	// 4. Victoire par capture : 10 pierres prises, soit 5 paires.
	//    Les motifs sont espacés pour qu'aucun ne se gêne, et les coups
	//    "ailleurs" du joueur 0 sont dispersés : alignés, ils gagnaient la
	//    partie par cinq avant d'avoir pu capturer cinq paires.
	{
		Gomoku g{withCapture()};
		const pos_t rows[5] = {0, 3, 6, 9, 12};
		const Pos filler0[5] = { {17,0}, {15,4}, {17,8}, {15,12}, {17,16} };
		const Pos filler1[5] = { {0,17}, {4,15}, {8,17}, {12,15}, {16,17} };
		for (int k = 0; k < 5; k++) {
			pos_t y = rows[k];
			// SIX coups par iteration, pas cinq : un nombre impair inverserait
			// la parite a chaque tour et le role de captureur passerait a
			// l'adversaire une fois sur deux.
			g.play({5, y});        // j0 : ancre gauche
			g.play({6, y});        // j1 : premiere du duo
			g.play(filler0[k]);    // j0 : ailleurs, disperse
			g.play({7, y});        // j1 : duo complet
			g.play({8, y});        // j0 : capture (6,y) et (7,y)
			g.play(filler1[k]);    // j1 : ailleurs, retablit la parite
			if (g.captures(0) != (unsigned)(2 * (k + 1)))
				std::printf("  (capture %d ratee : compteur = %u)\n",
					k, g.captures(0));
		}
		expect("10 pierres capturees", g.captures(0) >= 10,
			"les motifs de capture se genent probablement");
		expect("partie gagnee par capture", g.is_over() && g.winner() == 0,
			"victoire par capture non detectee");
		expect("score toujours coherent", g.heuristic().raw() == rebuiltScore(g));
	}

	// 5. Le hachage doit suivre les captures : deux positions dont l'une a
	//    subi une capture ne sont pas la même.
	{
		Gomoku a{withCapture()};
		a.play({5,5}); a.play({6,5}); a.play({15,15}); a.play({7,5}); a.play({8,5});
		Gomoku b{withCapture()};
		b.play({5,5}); b.play({6,5}); b.play({15,15}); b.play({7,5}); b.play({0,0});
		expect("hachage distinct apres capture", a.hash() != b.hash());
	}

	// 6. Paires CAPTURABLES (menace, pas capture realisee). Terme statique :
	//    une paire exposee se paie souvent au-dela de l'horizon, donc la
	//    recherche seule ne la voit pas.
	{
		Gomoku g{withCapture()};
		g.play({5,5});          // j0 : ancre
		g.play({6,5});          // j1
		g.play({15,15});        // j0 ailleurs
		g.play({7,5});          // j1 : la paire (6,5),(7,5) est prenable en (8,5)
		expect("une paire adverse exposee est comptee", g.capturable(0) == 1);
		expect("aucune paire a soi n'est exposee", g.capturable(1) == 0);

		g.play({8,5});          // j0 capture : la menace disparait
		expect("la menace disparait une fois la paire prise",
			g.capturable(0) == 0);

		Gomoku h{Rules{}};
		h.play({5,5}); h.play({6,5}); h.play({15,15}); h.play({7,5});
		expect("regle desactivee : aucune menace de capture",
			h.capturable(0) == 0);
	}

	std::printf("%d verifications, %d echec(s)\n", checks, failures);
	return failures != 0;
}
