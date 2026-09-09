// captureUnperfect sous le jeu de règles MANDATORY du sujet.
//
// « A player who manages to line up five stones wins only if the opponent
//   cannot break this line by capturing a pair. »
//
// Les règles obligatoires, et rien d'autre : 19x19, capture, capture de fin de
// partie, cinq OU PLUS gagne, double-trois interdit. Le double-quatre n'est pas
// demandé par le sujet, donc désactivé — soit "911110".
//
//     npx tsx tests/unperfect_test.ts

import { Gomoku, type Position, type Rules } from '../src/app/game/Gomoku'

const MANDATORY: Rules = {
  capture: true, captureUnperfect: true, overline: 'win',
  threeThree: true, fourFour: false, grid: '19x19',
}

let checks = 0, fails = 0
const expect = (what: string, got: unknown, want: unknown) => {
  checks++
  if (got !== want) { fails++; console.log(`  ECHEC ${what}\n        attendu ${want}, obtenu ${got}`) }
}

function replay(moves: Position[]) {
  let g = new Gomoku(MANDATORY)
  for (const m of moves) {
    const r = g.resolveMove(m)
    if (!r) throw new Error(`coup refusé : ${m} (après ${g.moves.length} coups)`)
    const next = new Gomoku(g)
    next.applyResolvedMove(r)
    g = next
  }
  return g
}

console.log('captureUnperfect, règles mandatory 911110')

// A. Cinq parfait : aucune paire de la ligne n'est prenable, victoire immédiate.
const A: Position[] = [[5,5],[0,0],[6,5],[0,2],[7,5],[0,4],[8,5],[0,6],[9,5]]
expect('A. cinq parfait -> noir gagne immédiatement', replay(A).result, 0)

// B. La paire verticale (5,5)-(5,6) est prenable en (5,7), blanc étant en
//    (5,4). Le cinq horizontal (5,5)..(9,5) passe donc par une pierre prenable.
const B: Position[] = [[5,5],[5,4],[6,5],[0,2],[7,5],[0,4],[8,5],[0,6],[5,6],[0,8],[9,5]]
expect('B. cinq prenable -> pas encore gagné', replay(B).result, null)
expect('B. blanc capture en 5:7 -> toujours pas gagné', replay([...B, [5,7]]).result, null)
expect('B. blanc joue ailleurs -> noir gagne au coup suivant', replay([...B, [1,18]]).result, 0)

// C. « Sometimes 5 or more is okay » : cinq OU PLUS gagne. Un overline se crée
//    en un seul coup, en comblant un trou — ici noir tient (5,5)..(8,5) et
//    (10,5),(11,5), et joue (9,5) : sept d'un coup, sans avoir jamais eu cinq.
const C: Position[] = [
  [5,5],[0,0], [6,5],[0,2], [7,5],[0,4], [8,5],[0,6],
  [10,5],[0,8], [11,5],[0,10], [9,5],
]
expect('C. overline créé en un coup -> noir gagne', replay(C).result, 0)

// D. Le même overline quand la règle dit que l'overline ne gagne PAS : il ne
//    doit pas gagner par un cinq imaginaire non plus.
{
  let g = new Gomoku({...MANDATORY, overline: 'legal'})
  for (const m of C) {
    const r = g.resolveMove(m)
    if (!r) throw new Error(`coup refusé : ${m}`)
    const next = new Gomoku(g); next.applyResolvedMove(r); g = next
  }
  expect("D. overline='legal' -> la partie continue", g.result, null)
}

// E. Le piège du sursis : le coup accordé ne sert que s'il CASSE la ligne.
//    Noir a le même cinq prenable qu'en B, et blanc dispose en plus d'une
//    capture sans rapport, en (3,0). La prendre ne retire aucune pierre du
//    cinq : blanc perd quand même, sur son propre coup.
const E: Position[] = [
  [5,5],[5,4], [6,5],[0,0], [7,5],[0,2], [8,5],[0,4],
  [1,0],[0,6], [2,0],[0,8], [5,6],[0,10], [9,5],
]
expect('E. cinq prenable posé -> pas encore gagné', replay(E).result, null)
{
  const after = replay([...E, [3,0]])
  expect('E. blanc capture AILLEURS -> la ligne tient, noir gagne', after.result, 0)
  expect('E. la capture a bien eu lieu', after.score[1], 2)
}
expect('E. blanc casse la ligne en 5:7 -> sauvé', replay([...E, [5,7]]).result, null)

console.log(`${checks} vérifications, ${fails} échec(s)`)
process.exit(fails === 0 ? 0 : 1)
