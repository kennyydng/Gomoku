"use client"

import { useMemo, useState } from 'react'

const HELP_BOARD_SIZE = 5

type Player = 1 | 2

export function createHelpBoard(stones: Array<{ row: number; col: number; player: Player }>) {
  const board = Array.from({ length: HELP_BOARD_SIZE }, () => Array.from({ length: HELP_BOARD_SIZE }, () => 0))

  for (const stone of stones) {
    board[stone.row][stone.col] = stone.player
  }

  return board
}

export type Rule = {
  title: string
  category: 'Victory' | 'Capture' | 'Forbidden'
  text: string
  showBoard?: boolean
  beforeBoard?: number[][]
  afterBoard?: number[][]
}

export const RULES: Rule[] = [
  {
    title: 'Victory Rules',
    category: 'Victory',
    text: '• Align 5 or more stones of your color continuously to win.\n• If an alignment can be immediately broken by capture, it does not count as a win.\n• Capturing 10 opponent stones also wins the game.',
    showBoard: false,
  },
  {
    title: 'Capture Rules',
    category: 'Capture',
    text: 'You capture a pair of opponent stones by flanking them on both sides with your stones. The two captured stones are removed from the board, freeing the intersections.',
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
  {
    title: 'Forbidden Moves',
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


function HelpMiniBoard({ board }: { board: number[][] }) {
  return (
    <div
      className="grid aspect-square w-full max-w-[170px] border border-amber-900/35 bg-[linear-gradient(135deg,_#c79b63_0%,_#b8834a_44%,_#8e5b30_100%)] p-1.5 shadow-[0_12px_28px_rgba(0,0,0,0.28)]"
      style={{ gridTemplateColumns: 'repeat(5, minmax(0, 1fr))' }}
    >
      {board.map((row, rowIndex) =>
        row.map((cell, colIndex) => (
          <div key={`${rowIndex}-${colIndex}`} className="aspect-square flex items-center justify-center border border-[rgba(39,25,14,0.35)] bg-[rgba(255,235,197,0.05)]">
            {cell !== 0 ? (
              <span
                className={`h-[60%] w-[60%] rounded-full border ${
                  cell === 1
                    ? 'border-stone-700/60 bg-[radial-gradient(circle_at_30%_30%,_#8f8f8f_0%,_#232323_52%,_#050505_100%)] shadow-[inset_0_3px_5px_rgba(255,255,255,0.18),0_8px_12px_rgba(0,0,0,0.5)] ring-1 ring-black/15'
                    : 'border-stone-300/70 bg-[radial-gradient(circle_at_30%_30%,_#ffffff_0%,_#ece4d7_50%,_#a89475_100%)] shadow-[inset_0_3px_5px_rgba(255,255,255,0.5),0_8px_12px_rgba(0,0,0,0.32)] ring-1 ring-white/20'
                }`}
              />
            ) : null}
          </div>
        )),
      )}
    </div>
  )
}

export default function HelpModal({ show, onClose, rules }: { show: boolean; onClose: () => void; rules: Rule[] }) {
  const [activeCategory, setActiveCategory] = useState<Rule['category']>('Victory')
  const categories = useMemo(() => ['Victory', 'Capture', 'Forbidden'] as const, [])

  if (!show) return null

  const visibleRules = rules.filter((r) => r.category === activeCategory)

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
              {visibleRules.map((rule) => (
                <article key={rule.title} className="rounded-2xl border border-stone-700/40 bg-black/20 p-4">
                  <h3 className="text-sm font-semibold uppercase tracking-[0.35em] text-amber-200/70">{rule.title}</h3>
                  <p className="mt-2 text-sm leading-6 text-stone-200 whitespace-pre-line">{rule.text}</p>

                  {rule.showBoard !== false && rule.beforeBoard && rule.afterBoard ? (
                    <div className="mt-4 grid gap-3 sm:grid-cols-2">
                      <div>
                        <p className="mb-2 text-[10px] uppercase tracking-[0.35em] text-stone-500">Before</p>
                        <HelpMiniBoard board={rule.beforeBoard} />
                      </div>
                      <div>
                        <p className="mb-2 text-[10px] uppercase tracking-[0.35em] text-stone-500">After</p>
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
