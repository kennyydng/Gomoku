#!/usr/bin/env python3
"""Lit la sortie de selfplay.py et en extrait ce qui départage vraiment.

Le score brut d'un match ment par construction. Les ouvertures sont jouées
par PAIRES à couleurs inversées : dans une paire où la même couleur gagne
deux fois, chaque moteur marque un point et rien n'a été départagé. Sur douze
parties, il peut ne rester qu'UNE paire décisive — on croit lire une
différence de force, on lit une partie.

Ce script compte donc les paires décisives (celles où le même moteur gagne
les deux parties) et donne la probabilité d'obtenir ce résultat par hasard,
à force égale : c'est un simple tirage à pile ou face sur ces paires-là.

    ./tools/pairs.py resultat.txt [autres.txt ...]
"""
import re
import sys
from math import comb


def read(path):
    """Rend la liste des parties : True = le challenger a gagné."""
    out = []
    for line in open(path):
        m = re.match(r"partie \d+/\d+ \(challenger=(noir|blanc)\): (.+)", line)
        if m:
            out.append(m.group(2).startswith("challenger"))
    return out


def binom_at_least(k, n):
    """P(au moins k succès sur n) à pile ou face — unilatéral."""
    if n == 0:
        return 1.0
    return sum(comb(n, i) for i in range(k, n + 1)) / 2 ** n


def report(path, games):
    if len(games) < 2:
        print(f"{path}: {len(games)} partie(s), rien à lire")
        return
    # selfplay.py joue l'ouverture i en noir puis en blanc : parties 2k et 2k+1.
    pairs = list(zip(games[0::2], games[1::2]))
    decisive = [a for a, b in pairs if a == b]
    won = sum(decisive)
    n = len(games)
    black = sum(games[0::2])
    white = sum(games[1::2])
    score = 100.0 * sum(games) / n

    print(f"{path}")
    print(f"  score brut        {sum(games)}/{n}  ({score:.1f}%)")
    print(f"  par couleur       noir {black}/{len(games[0::2])}, "
          f"blanc {white}/{len(games[1::2])}")
    print(f"  paires decisives  {won} gagnees sur {len(decisive)} "
          f"(les {len(pairs) - len(decisive)} autres se partagent)")
    if decisive:
        p = binom_at_least(won, len(decisive))
        verdict = ("difference etablie" if p < 0.05 else
                   "compatible avec l'egalite" if p > 0.2 else "indice, pas une preuve")
        print(f"  p (a force egale) {p:.3f}   -> {verdict}")
    else:
        print("  aucune paire ne departage : ce match ne mesure rien")
    print()


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 1
    for path in sys.argv[1:]:
        try:
            report(path, read(path))
        except OSError as e:
            print(f"{path}: {e}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
