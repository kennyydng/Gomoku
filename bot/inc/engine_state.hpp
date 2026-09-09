#pragma once

// Adaptateur entre le moteur bitboard (Gomoku) et la recherche générique
// (search.hpp). Tout ce que la recherche exige d'un état de jeu est réuni ici,
// pour que ni l'un ni l'autre n'ait à connaître les détails de son voisin.
//
// Compilé et testé dans le conteneur Arch du projet (GCC 16.2) et sur la
// machine de correction (Fedora 44, GCC 16.1.1 du dépôt) : voir
// tests/engine_test.cpp. Il exige la réflexion C++26 via Gomoku.class.hpp,
// donc GCC 16 au minimum — mais pas un compilateur exotique pour autant, le
// GCC livré par une distribution récente suffit. search.hpp, lui, reste
// vérifiable en C++23 standard.

#include <algorithm>
#include <vector>

#include "Gomoku.class.hpp"
#include "search.hpp"

struct EngineState {
	using move_t = Pos;

	Gomoku game;

	EngineState() = default;
	explicit EngineState(Rules rules): game(rules) {}

	bool player()   const { return game.player(); }
	bool terminal() const { return game.is_over(); }
	int  winner()   const { return game.winner(); }
	std::uint64_t hash() const { return game.hash(); }

	// Poids d'une capture, calé sur l'échelle interne des alignements
	// (line_values : un five vaut 50000). Quadratique : passer de 8 à 10
	// pierres prises gagne la partie et doit peser bien plus que passer de 0
	// à 2. À 10 prises, le terme vaut 50000, soit exactement un five.
	//
	// Sans ce terme, le moteur détectait la victoire par capture mais ne la
	// cherchait jamais : il capturait par accident.
	static long captureScore(unsigned taken)
		{ return 500L * (long)taken * (long)taken; }

	// Poids des menaces fortes, calés sur l'échelle interne (five = 50000) en
	// reprenant les rapports de l'heuristique de main, qui elle a été validée
	// en self-play : four ouvert / five = 0.15, three ouvert / five = 0.008,
	// fourche / five = 0.05.
	//
	// Ces termes ne sont PAS incrémentaux : ils sont recalculés à chaque
	// feuille. C'est le compromis assumé — le comptage par fenêtres, lui,
	// reste incrémental, et ne sait pas exprimer une non-linéarité (un four
	// ouvert ne vaut pas deux fours bloqués).
	static constexpr long W_OPEN4 = 7500;
	static constexpr long W_OPEN3 = 400;
	static constexpr long W_FORK  = 2500;
	// Une paire exposée vaut la dérivée du terme quadratique de captures, à
	// l'échelle de main (400 pour un five de 100000, donc 200 pour 50000) :
	// menacer la 5e paire pèse bien plus que menacer la 1re.
	//
	// MESURÉ NEUTRE, et il faut le savoir avant de s'appuyer dessus. Le
	// balayage de capturable() coûte 11% du débit (76800 nœuds contre 85888
	// sans lui, à budget égal), et deux A/B de 40 parties ne montrent aucun
	// gain : 47.5% à largeur 5 (p = 0.81), 55.0% à largeur 6 (p = 0.34).
	// Conservé parce que le sujet demande explicitement la part « captures
	// potentielles » de l'heuristique, pas parce qu'un gain a été prouvé.
	static constexpr long W_CAPTURE = 200;

	long threatScore(bool p) const {
			Gomoku::Threats const t = game.threats(p);
			long v = W_OPEN4 * t.open4 + W_OPEN3 * t.open3
				+ (t.axes >= 2 ? W_FORK : 0);
			if (game.rules().capture) {
				long const pairs = game.captures(p) / 2;
				v += W_CAPTURE * (2 * pairs + 1) * game.capturable(p);
			}
			return v;
		}

	// Le score interne accumule avec sign = P?1:-1, donc une valeur positive
	// favorise le joueur 1. Le négamax attend le point de vue du joueur au
	// trait : on retourne le signe quand c'est au joueur 0 de jouer.
	//
	// Le sens du signe est vérifié par engine_test (une inversion ne
	// planterait pas : le bot jouerait simplement les pires coups possibles).
	int evaluate() const {
			long v = game.heuristic().raw();
			if (game.rules().capture)   // positif = avantage joueur 1
				v += captureScore(game.captures(1)) - captureScore(game.captures(0));
			v += threatScore(1) - threatScore(0);
			long signed_v = player() ? v : -v;
			// Bornage : le score doit rester loin de MATE pour ne pas être
			// confondu avec une victoire prouvée.
			constexpr long cap = search::MATE / 2;
			return (int)std::clamp(signed_v, -cap, cap);
		}

	EngineState after(Pos m) const {
			EngineState next = *this;
			next.game.play(m);
			return next;
		}

	// Candidats : cases vides adjacentes à une pierre, triées par intérêt.
	//
	// Le tri n'est pas cosmétique. La recherche ne garde que les
	// `maxBranch` premiers, et un coup absent de cette liste n'est JAMAIS
	// exploré, quelle que soit la profondeur atteinte. Mesuré sur main :
	// passer la coupe de 5 à 4 fait chuter le score en self-play à 12.5%,
	// la monter à 6 le porte à 79.2%. C'est le paramètre le plus sensible
	// du moteur, et la qualité de ce tri conditionne ce qu'il vaut.
	//
	// Le critère de tri exploite le score incrémental : jouer le coup et lire
	// l'évaluation résultante donne exactement ce que l'heuristique en pense,
	// sans code de notation dupliqué. C'est possible ici précisément parce
	// que le moteur maintient son score par delta — sur main, il fallait une
	// fonction de notation séparée, qui pesait 83% du temps de recherche.
	std::vector<Pos> candidates() const {
			std::vector<Pos> cells = emptyNeighbours();

			std::vector<std::pair<int,Pos>> scored;
			scored.reserve(cells.size());
			const bool me = player();
			for (Pos p : cells) {
				// moveDelta rend la variation de score dans la convention
				// interne (positif = avantage joueur 1). On la ramène au point
				// de vue du joueur au trait, sans jouer le coup ni copier
				// l'état — c'est ce qui rend ce tri assez rapide pour tourner
				// à chaque nœud.
				//
				// ATTENTION AU SIGNE : la version précédente notait
				// `-after(p).evaluate()`, la négation étant indispensable
				// puisque after(p) rend le point de vue de l'ADVERSAIRE. Ici
				// le point de vue est explicite, et le piège disparaît avec la
				// copie d'état — mais le test d'ordre reste : il avait déjà
				// attrapé une inversion qui faisait rater un cinq immédiat.
				long d = game.moveDelta(p, me);
				// Gain marginal de la capture, dans la même échelle : la
				// dérivée du terme quadratique, donc la 5e paire pèse bien
				// plus que la 1re. Toujours dans la convention interne.
				if (unsigned taken = game.wouldCapture(p, me)) {
					unsigned had = game.captures(me);
					long gain = captureScore(had + taken) - captureScore(had);
					d += me ? gain : -gain;
				}
				scored.push_back({(int)std::clamp(me ? d : -d,
					-(long)search::MATE / 2, (long)search::MATE / 2), p});
			}
			// Tri décroissant : le meilleur d'abord, ce dont PVS dépend
			// entièrement pour que ses fenêtres nulles tiennent.
			std::sort(scored.begin(), scored.end(),
				[](auto const &a, auto const &b){ return a.first > b.first; });

			std::vector<Pos> out;
			out.reserve(scored.size());
			for (auto const &[s, p] : scored)
				out.push_back(p);
			return out;
		}

	// Public : le VCF (vcf.hpp) a besoin du même support de cases sans payer
	// le tri, qui coûterait un moveDelta par case à chacun de ses nœuds.
	// Le premier coup a besoin d'un cas particulier : sans pierre sur le
	// plateau, aucune case n'a de voisin et la liste serait vide, donc la
	// recherche ne rendrait aucun coup.
	std::vector<Pos> emptyNeighbours() const {
			std::vector<Pos> out;
			if (game.turn() == 0) {
				const pos_t half = game.rules().size / 2;
				out.push_back(Pos{half, half});
				return out;
			}
			for (Pos const pos : Pos::all()) {
				// Pos::all() balaie la grille de stockage ; sur un plateau
				// plus petit, les dernières lignes n'existent pas dans le jeu.
				if (!game.onBoard(pos) || !game.stone(pos).empty())
					continue;
				bool near = false;
				template for (constexpr auto dir : DIRECTIONS) {
					if (!near) {
						Pos a = pos + dir, b = pos - dir;
						if ((a.valid() && !game.stone(a).empty())
						 || (b.valid() && !game.stone(b).empty()))
							near = true;
					}
				}
				if (near)
					out.push_back(pos);
			}
			return out;
		}
};

static_assert(search::State<EngineState>,
	"EngineState doit satisfaire le concept attendu par la recherche");
