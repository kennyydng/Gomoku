// Accord entre les deux moteurs sur la seconde clause de l'endgame capture.
//
// Le moteur C++ et celui-ci lisent les mêmes règles ; un désaccord ferait
// diverger l'issue affichée par l'UI de celle que le bot calcule, et le
// symptôme serait une partie qui continue après une victoire, ou l'inverse.
// Ce test rejoue côté TypeScript exactement la position vérifiée par
// bot/tests/rules_test.cpp, et attend les mêmes verdicts.
//
//     npx tsx tests/rules_test.ts

import { Gomoku, type Position, type Rules } from '../src/app/game/Gomoku'

let checks = 0, failures = 0
const expect = (what: string, ok: boolean) => {
  checks++
  if (!ok) { failures++; console.log(`  ECHEC ${what}`) }
}

const RULES: Rules = {
  capture: true, captureUnperfect: true, overline: 'win',
  threeThree: false, fourFour: false, grid: '19x19',
}

// Quatre paires noires capturées par blanc, une cinquième prenable en (3,8),
// puis un cinq de noir en (5,12)..(9,12) dont aucune paire n'est prenable.
const OPENING: Position[] = [
  [1,0],[0,0], [2,0],[3,0],   [1,2],[0,2], [2,2],[3,2],
  [1,4],[0,4], [2,4],[3,4],   [1,6],[0,6], [2,6],[3,6],
  [1,8],[0,8], [2,8],[18,18],
  [5,12],[18,16], [6,12],[18,14], [7,12],[18,12], [8,12],[18,10],
  [9,12],
]

function replay(moves: Position[], rules: Rules = RULES) {
  let g = new Gomoku(rules)
  for (const m of moves) {
    const resolved = g.resolveMove(m)
    if (!resolved) throw new Error(`coup refuse: ${m}`)
    const next = new Gomoku(g)
    next.applyResolvedMove(resolved)
    g = next
  }
  return g
}

console.log('Accord des moteurs : endgame capture, clause des quatre paires')

const base = replay(OPENING)
expect('blanc a bien quatre paires', base.score[1] === 8)
expect('le cinq ne conclut pas quand blanc peut compter jusqu a dix',
  base.result === null)

const taken = replay([...OPENING, [3,8]])       // blanc prend la cinquième paire
expect('blanc gagne par capture', taken.result === 1)

const missed = replay([...OPENING, [18,8]])     // blanc joue ailleurs
expect('sinon le cinq de noir tient', missed.result === 0)

const off = replay(OPENING, {...RULES, captureUnperfect: false})
expect('regle desactivee : le cinq conclut tout de suite', off.result === 0)

console.log(`${checks} verifications, ${failures} echec(s)`)
process.exit(failures === 0 ? 0 : 1)
