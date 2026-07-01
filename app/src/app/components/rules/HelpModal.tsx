"use client"

import { useEffect, useMemo, useState } from 'react'
import { BOARD_THEME, STONE_THEME } from '../../constants/game'

const HELP_BOARD_SIZE = 7

type Player = 1 | 2
type HelpStone = Player | 3 | 4 | 5

export function createHelpBoard(stones: Array<{ row: number; col: number; player: HelpStone }>) {
  const board = Array.from({ length: HELP_BOARD_SIZE }, () => Array.from({ length: HELP_BOARD_SIZE }, () => 0))

  for (const stone of stones) {
    board[stone.row][stone.col] = stone.player
  }

  return board
}

export type RuleModal = {
  title: string
  category: 'Capture' | 'Forbidden' | 'Free-three' | 'General'
  text: string
  showBoard?: boolean
  beforeBoard?: number[][]
  afterBoard?: number[][]
  beforeLabel?: string
  afterLabel?: string
}

export const RULE_MODALS: RuleModal[] = [
  {
    title: 'Victory',
    category: 'General',
    text: '• Align 5 or more stones of your color continuously to win.\n• If an alignment can be immediately broken by capture, it does not count as a win.\n• Capturing 10 opponent stones also wins the game.',
    showBoard: false,
  },
  {
    title: 'Pass move',
    category: 'General',
    text: 'When pass is enabled, a player with no legal move can pass their turn instead of being forced into a draw. In the current setup, pass is on.',
    showBoard: false,
  },
  {
    title: 'Capture RuleModals',
    category: 'Capture',
    text: 'You capture a pair of opponent stones by flanking them on both sides with your stones. The two captured stones are removed from the board, freeing the intersections.',
    showBoard: true,
    beforeBoard: createHelpBoard([
      { row: 3, col: 1, player: 1 },
      { row: 3, col: 2, player: 2 },
      { row: 3, col: 3, player: 2 },
      { row: 3, col: 4, player: 3 },
    ]),
    afterBoard: createHelpBoard([
      { row: 3, col: 1, player: 1 },
      { row: 3, col: 4, player: 1 },
    ]),
  },
  {
    title: 'Capture win',
    category: 'Capture',
    text: 'Capturing 10 opponent stones wins the game immediately.',
    showBoard: false,
  },
  {
    title: 'Unperfect five',
    category: 'Capture',
    text: 'With captureUnperfect enabled, a line of 5 that can be immediately broken by capture is not always an instant win. The win may be delayed until the position is safe.',
    showBoard: false,
  },
  {
    title: 'What is a free-three?',
    category: 'Free-three',
    text: 'A free-three is an alignement of three stones that, if not immediately blocked, allows for an indefendable alignment of four stones (that’s to say an alignment of four stones with two unobstructed extremities). Both are free-threes:',
    showBoard: true,
    beforeLabel: 'Case 1',
    beforeBoard: createHelpBoard([
      { row: 3, col: 2, player: 1 },
      { row: 3, col: 3, player: 1 },
      { row: 3, col: 4, player: 1 },
    ]),
    afterLabel: 'Case 2',
    afterBoard: createHelpBoard([
      { row: 3, col: 2, player: 1 },
      { row: 3, col: 4, player: 1 },
      { row: 3, col: 5, player: 1 },
    ]),
  },
  {
    title: 'Forbidden Moves',
    category: 'Forbidden',
    text: "A double-three is a move that introduces two simultaneous free-three alignments. But the move in a would be legal:\n• Case 1: If it captures a pair of opponent stones\n• Case 2: If one of the free-threes would be obstructed",
    showBoard: true,
    beforeLabel: 'Case 1',
    beforeBoard: createHelpBoard([
      { row: 2, col: 3, player: 1 },
      
      { row: 3, col: 0, player: 1 },
      { row: 3, col: 1, player: 2 },
      { row: 3, col: 2, player: 2 },
      { row: 3, col: 3, player: 3 },
      { row: 3, col: 4, player: 1 },
      { row: 3, col: 5, player: 1 },

      { row: 4, col: 3, player: 1 },
    ]),
    afterLabel: 'Case 2',
    afterBoard: createHelpBoard([
        { row: 1, col: 1, player: 1 },
        { row: 2, col: 2, player: 1 },
        { row: 4, col: 4, player: 5 },
        { row: 4, col: 3, player: 4 },
        { row: 4, col: 5, player: 1 },
        { row: 4, col: 6, player: 1 },
    ]),
  },
  {
    title: 'Overline',
    category: 'Forbidden',
    text: 'An overline is a line longer than 5 stones. Depending on the rule settings, it can be illegal for Black or for both players.',
    showBoard: false,
  },
  {
    title: 'Double-four',
    category: 'Forbidden',
    text: 'A double-four is a move that creates two separate four-threats at the same time. When fourFour is enabled, this move is forbidden.',
    showBoard: false,
  },
  {
    title: 'Flanking',
    category: 'Forbidden',
    text: 'When flanking is enabled, a five can be rejected if it is completely flanked by opponent stones. That prevents a surrounded five from counting as an immediate win.',
    showBoard: false,
  },
]


function HelpMiniBoard({ board }: { board: number[][] }) {
  return (
    <div
      className={`grid aspect-square w-full max-w-[170px] border border-amber-900/35 ${BOARD_THEME.outerGradient} p-1.5 shadow-[0_12px_28px_rgba(0,0,0,0.28)]`}
      style={{ gridTemplateColumns: `repeat(${board.length}, minmax(0, 1fr))` }}
    >
      {board.map((row, rowIndex) =>
        row.map((cell, colIndex) => (
          <div key={`${rowIndex}-${colIndex}`} className="aspect-square flex items-center justify-center border border-[rgba(39,25,14,0.35)] bg-[rgba(255,235,197,0.05)]">
            {cell !== 0 ? (
              <span
                className={`h-[60%] w-[60%] rounded-full border ${
                  cell === 5
                    ? STONE_THEME.helpForbidden
                    : cell === 1 || cell === 3
                    ? STONE_THEME.black.help
                    : STONE_THEME.white.help
                }`}
                style={{ opacity: cell === 3 || cell === 4 ? 0.35 : cell === 5 ? 0.4 : 1 }}
              />
            ) : null}
          </div>
        )),
      )}
    </div>
  )
}

export default function HelpModal({ show, onClose, rules }: { show: boolean; onClose: () => void; rules: RuleModal[] }) {
  const [activeCategory, setActiveCategory] = useState<RuleModal['category']>('General')
  const categories = useMemo(() => Array.from(new Set(rules.map((rule) => rule.category))), [rules])

  useEffect(() => {
    if (categories.length === 0) {
      return
    }

    if (!categories.includes(activeCategory)) {
      setActiveCategory(categories[0])
    }
  }, [activeCategory, categories])

  if (!show) return null

  const visibleRuleModals = rules.filter((r) => r.category === activeCategory)

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/70 px-4 py-6 backdrop-blur-sm">
      <div className="flex max-h-[90vh] w-full max-w-5xl flex-col rounded-[2rem] border border-amber-900/35 bg-[linear-gradient(180deg,_rgba(29,20,13,0.99),_rgba(18,12,8,0.99))] p-5 shadow-[0_30px_120px_rgba(0,0,0,0.55)] sm:p-6">
        <div className="flex items-start justify-between gap-4">
          <div>
            <p className="text-[11px] uppercase tracking-[0.5em] text-amber-200/65">In-game help</p>
            <h2 className="mt-2 text-2xl font-black tracking-[0.08em] text-amber-100 sm:text-3xl">Rules summary</h2>
          </div>
          <button
            type="button"
            onClick={onClose}
            className="rounded-full border border-stone-700/50 bg-black/25 px-4 py-2 text-xs font-semibold uppercase tracking-[0.3em] text-stone-100 transition hover:border-amber-500/50 hover:bg-black/40"
          >
            Close
          </button>
        </div>

        <div className="mt-5 flex min-h-0 flex-1 gap-4 overflow-hidden">
          <aside className="hidden w-44 shrink-0 flex-col gap-2 md:flex">
            {categories.map((category) => (
              <button
                key={category}
                type="button"
                onClick={() => setActiveCategory(category)}
                className={`rounded-xl px-3 py-2 text-left text-sm transition ${
                  activeCategory === category ? 'bg-amber-500/15 text-amber-100' : 'text-stone-300 hover:bg-white/5'
                }`}
              >
                {category}
              </button>
            ))}
          </aside>

          <div className="flex min-h-0 flex-1 flex-col gap-3 overflow-y-auto pr-1">
            <div className="flex flex-wrap gap-2 md:hidden">
              {categories.map((category) => (
                <button
                  key={category}
                  type="button"
                  onClick={() => setActiveCategory(category)}
                  className={`rounded-full px-3 py-2 text-xs font-semibold uppercase tracking-[0.25em] transition ${
                    activeCategory === category ? 'bg-amber-500/15 text-amber-100' : 'border border-stone-700/40 text-stone-300'
                  }`}
                >
                  {category}
                </button>
              ))}
            </div>

            <div className="grid gap-3">
              {visibleRuleModals.map((rule) => (
                <article key={rule.title} className="rounded-2xl border border-stone-700/40 bg-black/20 p-4">
                  <h3 className="text-sm font-semibold uppercase tracking-[0.35em] text-amber-200/70">{rule.title}</h3>
                  <p className="mt-2 text-sm leading-6 text-stone-200 whitespace-pre-line">{rule.text}</p>

                  {rule.showBoard !== false && rule.beforeBoard && rule.afterBoard ? (
                    <div className="mt-4 grid gap-3 sm:grid-cols-2">
                      <div>
                        <p className="mb-2 text-[10px] uppercase tracking-[0.35em] text-stone-500">{rule.beforeLabel ?? 'Before'}</p>
                        <HelpMiniBoard board={rule.beforeBoard} />
                      </div>
                      <div>
                        <p className="mb-2 text-[10px] uppercase tracking-[0.35em] text-stone-500">{rule.afterLabel ?? 'After'}</p>
                        <HelpMiniBoard board={rule.afterBoard} />
                      </div>
                    </div>
                  ) : null}
                </article>
              ))}
            </div>
          </div>
        </div>

        <p className="mt-4 text-sm leading-6 text-stone-400">You can keep this open while playing, then close it and continue the game.</p>
      </div>
    </div>
  )
}
