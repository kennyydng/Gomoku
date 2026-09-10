#pragma once

// VCF — victoire par fours consécutifs (Victory by Continuous Fours).
//
// La recherche principale voit à une douzaine de plis. Un adversaire gagne
// contre elle en montant une séquence forcée qui se dénoue plus loin : chaque
// coup crée un quatre, il n'y a qu'une réponse, et la conclusion arrive
// au-delà de l'horizon. Le bot joue alors le coup qui paraît le meilleur
// juste avant de se faire piéger.
//
// Le VCF n'explore QUE les coups forçants. Le branchement tombe de 5 à 1-3,
// ce qui permet de descendre deux à trois fois plus profond pour une fraction
// du coût — c'est la profondeur qu'on ne peut pas acheter en élargissant.
//
// RÈGLE DE SÛRETÉ, la plus importante de ce fichier : l'ensemble des réponses
// adverses doit être un SUR-ensemble des défenses réelles, jamais un
// sous-ensemble. Rater une défense ferait annoncer une victoire forcée qui
// n'existe pas — le bot jouerait la séquence et perdrait. Rater une victoire
// n'est qu'une occasion manquée. Les deux erreurs ne se valent pas, et tout
// le fichier est écrit dans ce sens : quand le budget s'épuise, on conclut
// que la défense tient.
//
// Ce jeu de règles complique le VCF classique : un quatre n'y est pas forçant
// au sens habituel, puisque l'adversaire peut aussi CAPTURER une paire de
// l'alignement pour le casser. D'où la construction explicite de l'ensemble
// des réponses.

#include <optional>
#include <vector>

#include "engine_state.hpp"

namespace vcf {

struct Budget {
	int plies = 16;                  // profondeur maximale de la séquence
	unsigned long nodeCap = 120000;  // plafond dur : le budget de temps ne doit
	unsigned long nodes = 0;         // pas dépendre de la forme de la position
};

bool defends(EngineState const &state, Budget &b, int plies);

// L'attaquant ne joue que des coups forçants. `best` reçoit le premier coup
// de la séquence gagnante, quand il y en a une.
inline bool attacks(EngineState const &state, Budget &b, int plies,
                    Pos *best = nullptr) {
	if (plies <= 0 || ++b.nodes > b.nodeCap)
		return false;

	const bool me = state.player();
	const auto cells = state.immediateCandidates();

	// D'abord les gains immédiats : inutile de chercher une séquence quand un
	// seul coup suffit. wouldWin() lit les fenêtres, donc on CONFIRME en
	// jouant le coup — c'est là, et seulement là, que les règles de fin de
	// partie s'appliquent exactement.
	for (Pos p : cells) {
		if (!state.game.wouldWin(p, me))
			continue;
		EngineState next = state.after(p);
		if (next.terminal() && next.winner() == (int)me) {
			if (best) *best = p;
			return true;
		}
	}

	for (Pos p : cells) {
		if (!state.game.wouldThreatenFive(p, me))
			continue;
		EngineState next = state.after(p);
		if (next.terminal())
			continue;                    // déjà traité au-dessus
		if (!defends(next, b, plies - 1)) {
			if (best) *best = p;
			return true;
		}
	}
	return false;
}

// Le défenseur survit-il ? Retourne true dès qu'UNE réponse tient — et true
// aussi quand le budget s'épuise, parce que « je ne sais pas » doit compter
// comme « la défense tient » (voir la règle de sûreté en tête de fichier).
inline bool defends(EngineState const &state, Budget &b, int plies) {
	if (plies <= 0 || b.nodes > b.nodeCap)
		return true;

	const bool defender = state.player();
	const bool attacker = !defender;

	// L'ensemble des réponses, volontairement large :
	//   - la case qui compléterait le cinq adverse (le blocage classique) ;
	//   - un coup qui gagne pour le défenseur (il prend de vitesse) ;
	//   - TOUTE capture, même sans rapport apparent avec l'alignement : elle
	//     peut en retirer une pierre, et sous cette règle c'est une parade.
	std::vector<Pos> replies;
	for (Pos p : state.immediateCandidates()) {
		if (state.game.wouldWin(p, attacker)
		 || state.game.wouldWin(p, defender)
		 || state.game.wouldCapture(p, defender))
			replies.push_back(p);
	}
	if (replies.empty())
		return false;                    // aucune parade : la menace conclut

	for (Pos p : replies) {
		EngineState next = state.after(p);
		if (next.terminal()) {
			// Terminal ne veut pas dire « sauvé ». Sous captureUnperfect, un
			// cinq de l'attaquant reste cassable tant qu'une de ses paires est
			// prenable : la partie n'est pas finie quand il le pose, elle se
			// termine sur le coup du DÉFENSEUR, dès que celui-ci répond autre
			// chose qu'une capture. Rendre true ici comptait cette parade
			// perdante comme tenue, et abandonnait la séquence.
			if (next.winner() == (int)attacker)
				continue;
			return true;                 // le défenseur conclut, ou nulle
		}
		if (!attacks(next, b, plies - 1))
			return true;                 // l'attaquant ne va pas au bout
	}
	return false;
}

// Point d'entrée : rend le premier coup d'une victoire forcée, ou rien.
inline std::optional<Pos> find(EngineState const &state, Budget &b) {
	Pos best{};
	if (attacks(state, b, b.plies, &best))
		return best;
	return std::nullopt;
}

} // namespace vcf
