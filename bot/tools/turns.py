#!/usr/bin/env python3
"""Combien de tours faut-il au bot pour battre un joueur faible ?

Critère de la grille : "AI victory in under 20 turns". Le self-play ne peut
pas y répondre (deux moteurs forts jouent 40-55 plis), il faut un adversaire
volontairement mauvais : un coup au hasard collé à une pierre existante.

    ./turns.py ./Gomoku 10        # 10 parties, le bot commence
"""
import random
import re
import statistics
import subprocess
import sys

BINARY = sys.argv[1] if len(sys.argv) > 1 else "./Gomoku"
GAMES = int(sys.argv[2]) if len(sys.argv) > 2 else 10
RULES = sys.argv[3] if len(sys.argv) > 3 else "911111"
SIZE = 19
MAX_PLIES = 120


import os

def check_result(history):
    """État de la partie sans lancer de recherche (mode RESULT_ONLY)."""
    stdin = RULES + "\n" + "".join(f"|{x}:{y}\n" for x, y in history)
    env = dict(os.environ, RESULT_ONLY="1")
    run = subprocess.run([BINARY], input=stdin, capture_output=True,
                         text=True, env=env)
    found = re.search(r"^Result: (.+)$", run.stderr, re.M)
    return found.group(1) if found else "?"


def bot_move(history):
    stdin = RULES + "\n" + "".join(f"|{x}:{y}\n" for x, y in history)
    run = subprocess.run([BINARY], input=stdin, capture_output=True, text=True)
    found = re.search(r"^Result: (win [01]|draw)$", run.stderr, re.M)
    if found:
        return None, found.group(1)
    played = run.stdout.rsplit("|", 1)
    if len(played) != 2:
        return None, "erreur"
    x, y = played[1].strip().split(":")
    return (int(x), int(y)), "ongoing"


def weak_move(history, rng):
    played = set(history)
    near = {
        (x + dx, y + dy)
        for (x, y) in played
        for dx in (-1, 0, 1) for dy in (-1, 0, 1)
        if 0 <= x + dx < SIZE and 0 <= y + dy < SIZE
    }
    choices = sorted(near - played)
    return rng.choice(choices) if choices else (0, 0)


results = []
for g in range(GAMES):
    rng = random.Random(1000 + g)
    history = []                              # le bot ouvre => il est le joueur 0
    outcome = None
    while len(history) < MAX_PLIES:
        # Vérifier l'état APRÈS chaque coup : le moteur rapporte le résultat
        # du dernier coup rejoué, donc empiler un coup par-dessus une
        # victoire la masque définitivement.
        res = check_result(history) if history else "ongoing"
        if res != "ongoing":
            outcome = res
            break
        if len(history) % 2 == 1:             # index impair => adversaire faible
            history.append(weak_move(history, rng))
            continue
        mv, res = bot_move(history)
        if res != "ongoing":
            outcome = res
            break
        history.append(mv)

    plies = len(history)
    turns = (plies + 1) // 2                  # un "tour" = un coup de chaque
    won = outcome == "win 0"
    results.append((turns, won, outcome))
    print(f"  partie {g+1}: {turns} tours ({plies} plis) — "
          f"{'IA gagne' if won else outcome or 'non conclue'}", flush=True)

wins = [t for t, w, _ in results if w]
print(f"\nvictoires de l'IA : {len(wins)}/{len(results)}")
if wins:
    print(f"  tours pour gagner : moyenne {statistics.mean(wins):.1f}, "
          f"médiane {statistics.median(wins)}, min {min(wins)}, max {max(wins)}")
    print(f"  parties gagnées en moins de 20 tours : "
          f"{sum(1 for t in wins if t < 20)}/{len(wins)}")
