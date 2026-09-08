// Le score incrémental valorise-t-il correctement le BLOCAGE ?
//
// Soupçon né du diagnostic de mat : la défense évidente (boucher un four
// adverse) est classée au-delà du 20e candidat, ce qui n'a de sens que si
// l'heuristique ne récompense pas le blocage — voire le pénalise.
//
// Convention du moteur : _score accumule avec sign = P?1:-1, donc une valeur
// POSITIVE favorise le joueur 1 et négative le joueur 0.

#include "engine_state.hpp"

#include <cstdio>

static EngineState play(std::initializer_list<Pos> moves) {
	EngineState s;
	for (Pos p : moves)
		s = s.after(p);
	return s;
}

int main() {
	std::printf("Le score recompense-t-il le blocage ?\n");
	std::printf("(convention : valeur POSITIVE = avantage joueur 1)\n\n");

	// Joueur 1 construit un trois en colonne 0 ; joueur 0 a le trait.
	// Attention au piege de la version precedente de ce test : il faut
	// verifier QUI a le trait avant d'etiqueter un coup "blocage".
	EngineState base = play({ {5,5}, {0,0}, {9,9}, {0,1}, {13,13}, {0,2} });
	long v0 = base.game.heuristic().raw();
	std::printf("  joueur au trait : %d (doit etre 0)\n", (int)base.player());
	std::printf("  base : joueur 1 a un trois en colonne 0   raw = %ld\n\n", v0);

	long vBlock   = base.after(Pos{0,3}).game.heuristic().raw();   // bloque le trois
	long vNeutral = base.after(Pos{16,4}).game.heuristic().raw();  // coup isole

	std::printf("  joueur 0 bloque en 0:3     raw = %ld  (delta %+ld)\n",
		vBlock, vBlock - v0);
	std::printf("  joueur 0 joue isole 16:4   raw = %ld  (delta %+ld)\n",
		vNeutral, vNeutral - v0);

	// Negatif favorise le joueur 0 : bloquer doit donc rendre la valeur PLUS
	// NEGATIVE que jouer dans le vide.
	bool ok = vBlock < vNeutral;
	std::printf("\n  bloquer profite-t-il a celui qui bloque ? %s\n",
		ok ? "OUI" : "NON  <-- signe du terme de blocage inverse");

	// Et la defense doit remonter dans le tri des candidats, sans quoi la
	// recherche ne l'explore jamais (elle ne garde que les premiers).
	auto cands = base.candidates();
	int rank = -1;
	for (std::size_t i = 0; i < cands.size(); i++)
		if (cands[i] == Pos{0,3}) { rank = (int)i; break; }
	std::printf("  rang du blocage 0:3 parmi %zu candidats : %d\n",
		cands.size(), rank);
	std::printf("  -> %s\n", (rank >= 0 && rank < 5)
		? "dans les 5 explores : la defense est vue"
		: "HORS des 5 explores : la defense reste invisible");

	return ok ? 0 : 1;
}
