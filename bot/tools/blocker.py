#!/usr/bin/env python3
"""Combien de tours au bot contre un joueur qui ne fait QUE bloquer ?

Le barème note « AI victory in under 20 turns ». turns.py mesure ça contre un
adversaire aléatoire, qui ne représente pas un correcteur attentif. Celui-ci
joue la défense la plus têtue possible : il bloque toujours l'alignement le
plus avancé du bot, et capture quand la capture casse une menace.

    ./blocker.py ./Gomoku 6        # 6 parties, règles mandatory
"""
import re, subprocess, sys, statistics

BIN   = sys.argv[1] if len(sys.argv) > 1 else "./Gomoku"
GAMES = int(sys.argv[2]) if len(sys.argv) > 2 else 6
RULES = sys.argv[3] if len(sys.argv) > 3 else "911110"
N, MAXPLY = 19, 150
AXES = [(1,0),(0,1),(1,1),(1,-1)]
OPENINGS = [(9,9),(8,8),(10,9),(9,7),(7,10),(11,8),(8,11),(10,11),(6,9),(9,12)]

def blank(): return [[None]*N for _ in range(N)]

def place(b, x, y, p):
    """Pose la pierre et applique les captures — même motif que le moteur."""
    b[y][x] = p
    taken = 0
    for dx in (-1,0,1):
        for dy in (-1,0,1):
            if dx == dy == 0: continue
            ps = [(x+dx*i, y+dy*i) for i in (1,2,3)]
            if any(not (0 <= a < N and 0 <= c < N) for a,c in ps): continue
            (x1,y1),(x2,y2),(x3,y3) = ps
            if b[y1][x1] == 1-p and b[y2][x2] == 1-p and b[y3][x3] == p:
                b[y1][x1] = b[y2][x2] = None
                taken += 2
    return taken

def run_len(b, x, y, dx, dy, p):
    n = 0
    while 0 <= x < N and 0 <= y < N and b[y][x] == p:
        n += 1; x += dx; y += dy
    return n

def block_move(b, me, foe):
    """La case qui contient le mieux la menace la plus avancée de `foe`."""
    best, best_score = None, -1
    for y in range(N):
        for x in range(N):
            if b[y][x] is not None: continue
            score = 0
            for dx, dy in AXES:
                a = run_len(b, x+dx, y+dy, dx, dy, foe)
                c = run_len(b, x-dx, y-dy, -dx, -dy, foe)
                if a + c >= 1:
                    score = max(score, (a + c) * 10 + (1 if a and c else 0))
            # Capturer vaut mieux que bloquer à valeur égale : ça retire deux
            # pierres au lieu d'en freiner une.
            probe = [row[:] for row in b]
            if place(probe, x, y, me):
                score += 25
            if score > best_score:
                best, best_score = (x, y), score
    return best

def ask(history):
    stdin = RULES + "\n" + "".join(f"|{x}:{y}\n" for x, y in history)
    r = subprocess.run([BIN], input=stdin, capture_output=True, text=True)
    done = re.search(r"^Result: (win [01]|draw)$", r.stderr, re.M)
    if done: return None, done.group(1)
    m = re.findall(r"\|(\d+):(\d+)", r.stdout)
    return ((int(m[-1][0]), int(m[-1][1])), None) if m else (None, "?")

wins, lengths = [], []
for g in range(GAMES):
    b, hist = blank(), []
    # Ouvertures distinctes : les deux moteurs étant déterministes, un même
    # premier coup rejoue exactement la même partie. Sans cette variété, six
    # parties n'en mesurent qu'une ou deux.
    ox, oy = OPENINGS[g % len(OPENINGS)]
    place(b, ox, oy, 0); hist.append((ox, oy))
    outcome, plies = "?", 1
    while plies < MAXPLY:
        d = block_move(b, 1, 0)
        if d is None: outcome = "plateau plein"; break
        place(b, *d, 1); hist.append(d); plies += 1
        mv, done = ask(hist)
        if done: outcome = done; break
        place(b, *mv, 0); hist.append(mv); plies += 1
        mv2, done = ask(hist)          # l'issue après le coup du bot
        if done: outcome = done; break
    turns = -(-plies // 2)
    lengths.append(turns)
    if outcome == "win 0": wins.append(turns)
    print(f"  partie {g+1}: {outcome} en {plies} plis = {turns} tours")

print(f"\n  victoires du bot : {len(wins)}/{GAMES}")
if wins:
    print(f"  tours pour gagner : moyenne {statistics.mean(wins):.1f}, "
          f"min {min(wins)}, max {max(wins)}")
    print(f"  sous les 20 tours : {sum(1 for w in wins if w < 20)}/{len(wins)}")
