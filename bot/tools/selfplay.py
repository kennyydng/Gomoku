#!/usr/bin/env python3
"""Fait jouer deux binaires du bot l'un contre l'autre et compte les victoires.

Le seul verdict qui compte pour une modif d'heuristique : est-ce que la
nouvelle version bat l'ancienne ? Les nœuds/seconde et la profondeur atteinte
ne disent rien sur la force de jeu.

    ./tools/selfplay.py ./Gomoku ./Gomoku_baseline -n 40

Pour fabriquer le binaire de référence à partir d'une version précédente, sans
toucher au working tree (les binaires Gomoku_* sont gitignorés) :

    git worktree add /tmp/gomoku-base <ref>
    make -C /tmp/gomoku-base/bot
    cp /tmp/gomoku-base/bot/Gomoku bot/Gomoku_baseline
    git worktree remove /tmp/gomoku-base

Attention à la taille d'échantillon : sur 12 parties, l'écart-type est de
~14 points de pourcentage. Un résultat à 50% ne prouve donc que l'absence de
*gros* écart — il faut plusieurs dizaines de parties pour trancher sur un
petit avantage.

Protocole d'un binaire (voir main.cpp) : on lui envoie la ligne de règles puis
l'historique complet (`|x:y` par coup) sur stdin. Il rend l'historique + son
coup sur stdout, et l'état de la partie sur stderr (`Result: win 0`, `draw`,
`ongoing`). Un process par coup — c'est déjà comme ça que l'API web l'appelle.
"""

import argparse
import random
import re
import subprocess
import sys

# Coups d'ouverture tirés au sort : les deux moteurs sont déterministes, sans
# ça les N parties seraient N copies de la même. On impose seulement le tout
# début, le reste est joué par les bots.
OPENING_PLIES = 4


def ask(binary, rules, history, timeout):
    """Envoie l'historique au binaire, retourne (coup, résultat)."""
    stdin = rules + "\n" + "".join(f"|{x}:{y}\n" for x, y in history)
    run = subprocess.run(
        [binary], input=stdin, capture_output=True, text=True, timeout=timeout
    )

    result = "ongoing"
    found = re.search(r"^Result: (win [01]|draw|ongoing)$", run.stderr, re.M)
    if found:
        result = found.group(1)
    if result != "ongoing":
        return None, result

    played = run.stdout.rsplit("|", 1)
    if len(played) != 2:
        raise RuntimeError(f"{binary}: pas de coup en sortie ({run.stdout!r})")
    x, y = played[1].strip().split(":")
    return (int(x), int(y)), result


def legal_openings(size, count, rng):
    """Quelques coups distincts près du centre, alternés entre les joueurs."""
    center = size // 2
    spread = 3
    seen = set()
    while len(seen) < count:
        pos = (
            rng.randint(center - spread, center + spread),
            rng.randint(center - spread, center + spread),
        )
        seen.add(pos)
    return list(seen)


def play(binary_black, binary_white, rules, opening, timeout, max_plies):
    """Joue une partie. Retourne 0 (noir gagne), 1 (blanc), ou None (nulle)."""
    history = list(opening)

    while len(history) < max_plies:
        turn = len(history) % 2
        binary = binary_black if turn == 0 else binary_white
        move, result = ask(binary, rules, history, timeout)

        if result.startswith("win"):
            return int(result.split()[1])
        if result == "draw":
            return None
        history.append(move)

    return None  # partie trop longue, comptée comme nulle


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("challenger", help="binaire à évaluer")
    parser.add_argument("baseline", help="binaire de référence")
    parser.add_argument("-n", "--games", type=int, default=10)
    parser.add_argument("-r", "--rules", default="911111")
    parser.add_argument("-s", "--size", type=int, default=19)
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--timeout", type=float, default=30.0)
    parser.add_argument("--max-plies", type=int, default=200)
    args = parser.parse_args()

    rng = random.Random(args.seed)
    wins = losses = draws = 0
    opening = None

    for game in range(args.games):
        # Chaque ouverture est jouée deux fois, couleurs inversées : sinon on
        # mesure surtout l'avantage du premier joueur et le hasard de
        # l'ouverture, pas la différence entre les deux versions.
        challenger_is_black = game % 2 == 0
        if challenger_is_black:
            opening = legal_openings(args.size, OPENING_PLIES, rng)
        black, white = (
            (args.challenger, args.baseline)
            if challenger_is_black
            else (args.baseline, args.challenger)
        )

        winner = play(black, white, args.rules, opening, args.timeout,
                      args.max_plies)

        if winner is None:
            draws += 1
            verdict = "nulle"
        elif (winner == 0) == challenger_is_black:
            wins += 1
            verdict = "challenger gagne"
        else:
            losses += 1
            verdict = "baseline gagne"

        color = "noir" if challenger_is_black else "blanc"
        print(f"partie {game + 1}/{args.games} (challenger={color}): {verdict}",
              flush=True)

    played = wins + losses + draws
    print(f"\nchallenger {args.challenger} vs baseline {args.baseline}")
    print(f"{wins}V {losses}D {draws}N sur {played} parties "
          f"({100.0 * (wins + 0.5 * draws) / played:.1f}% de score)")


if __name__ == "__main__":
    sys.exit(main())
