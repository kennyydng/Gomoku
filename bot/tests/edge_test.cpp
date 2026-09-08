// Symetrie du plateau : le comptage par fenetres traite-t-il les bords pareil ?
//
// Le plateau est symetrique par miroir horizontal, vertical et par transposee.
// Le score, qui n'est qu'une somme sur les fenetres, doit l'etre aussi. Une
// asymetrie signifie que certaines fenetres sont comptees et d'autres non
// selon l'endroit — et c'est invisible autrement : aucune assertion ne saute,
// le bot joue simplement mal d'un cote du plateau.
//
// Ce test a ete ecrit apres coup, sur un bug reel : LinesStart marquait les
// indices 0..SIZE-5 alors que la fenetre d'indice i couvre i-4..i. Quatre
// fenetres fantomes debordant a gauche etaient comptees, et les quatre
// dernieres de chaque ligne etaient invisibles. Un cinq aligne contre le bord
// droit valait 93 points au lieu de 52000.

#include "engine_state.hpp"

#include <cstdio>
#include <vector>

static int checks = 0, failures = 0;

static void expect(const char *what, bool ok, const char *detail = "") {
	checks++;
	if (!ok) {
		failures++;
		std::printf("  ECHEC %-50s %s\n", what, detail);
	}
}

// Joue la sequence telle quelle et rend le score interne.
static long scoreOf(std::vector<Pos> const &moves) {
	Gomoku g{Rules{}};
	for (Pos p : moves)
		g.play(p);
	return g.heuristic().raw();
}

static std::vector<Pos> mapAll(std::vector<Pos> const &moves, Pos (*f)(Pos)) {
	std::vector<Pos> out;
	out.reserve(moves.size());
	for (Pos p : moves)
		out.push_back(f(p));
	return out;
}

static Pos mirrorX(Pos p) { return {SIZE - 1 - p.x, p.y}; }
static Pos mirrorY(Pos p) { return {p.x, SIZE - 1 - p.y}; }
static Pos transpose(Pos p) { return {p.y, p.x}; }

int main() {
	std::printf("Symetrie du comptage par fenetres\n");

	// Une position quelconque, volontairement collee au bord gauche/haut :
	// c'est la que l'asymetrie se voit.
	const std::vector<Pos> pos = {
		{1,1},{5,7}, {2,1},{6,7}, {3,1},{7,7}, {1,2},{8,8}, {2,3},{9,9},
	};

	const long ref = scoreOf(pos);
	expect("miroir horizontal : meme score",
		scoreOf(mapAll(pos, mirrorX)) == ref,
		"les bords gauche et droit ne sont pas comptes pareil");
	expect("miroir vertical : meme score",
		scoreOf(mapAll(pos, mirrorY)) == ref,
		"les bords haut et bas ne sont pas comptes pareil");
	expect("transposee : meme score",
		scoreOf(mapAll(pos, transpose)) == ref,
		"les axes horizontal et vertical ne sont pas comptes pareil");

	// Et le cas qui avait revele le bug : un cinq contre chaque bord doit
	// etre vu par le comptage, pas seulement par la detection de fin de
	// partie.
	auto fiveAt = [](std::vector<Pos> mine) {
		Gomoku g{Rules{}};
		const Pos filler[] = {{9,9},{9,11},{11,9},{11,11}};
		for (std::size_t i = 0; i < mine.size(); i++) {
			g.play(mine[i]);
			if (i + 1 < mine.size())
				g.play(filler[i]);
		}
		int n = 0;
		template for (constexpr size_t AX : index_of(AXES))
			n += popcount(std::get<AX>(g.player_info(0).lines).of(5));
		return n;
	};

	expect("cinq au bord gauche : compte",
		fiveAt({{0,3},{1,3},{2,3},{3,3},{4,3}}) == 1);
	expect("cinq au bord droit : compte",
		fiveAt({{14,3},{15,3},{16,3},{17,3},{18,3}}) == 1,
		"les dernieres fenetres de la ligne sont invisibles");
	expect("cinq au bord haut : compte",
		fiveAt({{3,0},{3,1},{3,2},{3,3},{3,4}}) == 1);
	expect("cinq au bord bas : compte",
		fiveAt({{3,14},{3,15},{3,16},{3,17},{3,18}}) == 1,
		"les dernieres fenetres de la colonne sont invisibles");

	// L'invariant qui a attrape le bug, et qui ne depend d'aucune formule :
	// chaque fenetre couvre exactement cinq cases, donc la somme sur toutes
	// les cases du nombre de fenetres qui les contiennent vaut cinq fois le
	// nombre de fenetres. Une incoherence signale des fenetres fantomes ou
	// manquantes, sans qu'on ait besoin de savoir lesquelles.
	{
		Gomoku const empty{Rules{}};
		template for (constexpr size_t AX : index_of(AXES)) {
			const int windows = popcount(std::get<AX>(empty.player_info(0).lines).of(0));
			int covered = 0;
			for (pos_t y = 0; y < SIZE; y++)
				for (pos_t x = 0; x < SIZE; x++) {
					Gomoku g{Rules{}};
					g.play(Pos{x, y});
					covered += popcount(std::get<AX>(g.player_info(0).lines).of(1));
				}
			char label[80];
			std::snprintf(label, sizeof label,
				"axe %zu : couverture coherente (%d fenetres)", AX, windows);
			expect(label, covered == 5 * windows,
				"des fenetres fantomes sont comptees, ou il en manque");
		}
	}

	std::printf("%d verifications, %d echec(s)\n", checks, failures);
	return failures != 0;
}
