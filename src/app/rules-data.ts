const HELP_BOARD_SIZE = 5

type Player = 1 | 2

function createHelpBoard(stones: Array<{ row: number; col: number; player: Player }>) {
  const board = Array.from({ length: HELP_BOARD_SIZE }, () => Array.from({ length: HELP_BOARD_SIZE }, () => 0))

  for (const stone of stones) {
    board[stone.row][stone.col] = stone.player
  }

  return board
}

export type Rule = {
  title: string
  category: string
  text: string
  showBoard?: boolean
  beforeBoard?: number[][]
  afterBoard?: number[][]
}

const RULES: Rule[] = [
  // Victory by alignment
  {
    title: 'Victory by alignment',
    category: 'Victory',
    text: 'Victory by alignment: Align 5 or more stones of your color continuously (alignments of 6 or more also count as a win).',
    showBoard: false,
  },

  // Capture rule
  {
    title: 'Capture rule',
    category: 'Victory',
    text: 'Mechanism: You capture a pair of opponent stones by flanking them on both sides with your stones. The two captured stones are removed from the board, freeing the intersections.',
    showBoard: true,
    beforeBoard: createHelpBoard([
      { row: 2, col: 0, player: 1 },
      { row: 2, col: 1, player: 2 },
      { row: 2, col: 2, player: 2 },
    ]),
    afterBoard: createHelpBoard([
      { row: 2, col: 0, player: 1 },
      { row: 2, col: 3, player: 1 },
    ]),
  },

  // Overline note
  {
    title: 'Overline (six or more)',
    category: 'Victory',
    text: 'Note: some variants treat an overline (six or more in a row) differently. In this ruleset, five or more counts as a win.',
    showBoard: false,
  },

  // Forbidden moves
  {
    title: 'Double-three (forbidden)',
    category: 'Forbidden',
    text: "A move that simultaneously creates two 'free-threes' is illegal, unless that same move captures a pair immediately.",
    showBoard: true,
    beforeBoard: createHelpBoard([
      { row: 2, col: 0, player: 1 },
      { row: 2, col: 1, player: 1 },
      { row: 1, col: 2, player: 1 },
      { row: 3, col: 2, player: 1 },
    ]),
    afterBoard: createHelpBoard([
      { row: 2, col: 0, player: 1 },
      { row: 2, col: 1, player: 1 },
      { row: 2, col: 2, player: 1 },
      { row: 1, col: 2, player: 1 },
      { row: 3, col: 2, player: 1 },
    ]),
  },
]

export default RULES
