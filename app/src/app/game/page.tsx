"use client"

import { useEffect, useMemo, useState } from 'react'
import { useRouter, useSearchParams } from 'next/navigation'
import GomokuBoard from '../components/gomoku/GomokuBoard'
import { Gomoku } from './Gomoku'
import { getCaptureOrbClass, getTurnOrbClass } from '../constants/game'
import { GAME_PAGE_THEME } from '../constants/ui'
import HelpModal, { RULE_MODALS } from '../components/rules/HelpModal'

import type { Player } from './Gomoku'

type GameMode = 'local' | 'ai' | 'training'
import type { RuleModal } from '../components/rules/HelpModal'

function getGaugeLabel(tone: 'black' | 'white', mode: GameMode) {
  if (tone === 'black')
    return 'Black'
  if (mode === 'local')
    return 'White'
  return null
}

export default function GamePage() {
  const router = useRouter()
  const mode = useSearchParams()?.get('mode')
  const rules = {
    capture: true,
    captureUnperfect: true,
    foulOverline: false,
    overline: false,
    threeThree: true,
    fourFour: false,
    flanking: false,
    pass: true,
  }

  const [botResponseMs, setBotResponseMs] = useState<"pending" | number | null>(null)
  const [showRules, setShowRules] = useState(false)
  const [activeCategory, setActiveCategory] = useState<RuleModal['category']>('Victory')
  const [turn, setTurn] = useState<number>(0)
  const [score, setScore] = useState<[number,number]>([0,0])
  const [currentPlayer, setCurrentPlayer] = useState<Player>(0)

  const validMode = mode === 'local' || mode === 'ai' || mode === 'training'
  if (!validMode) {
    router.replace('/')
    return
  }

  const categories = useMemo(() => ['Victory', 'Capture', 'Forbidden'] as const, [])

  const handleGameUpdate = (game: Gomoku) => {
    if (turn !== game.moves.length)
      setTurn(game.moves.length)
    if (score[0] !== game.score[0] || score[1] !== game.score[1])
      setScore(game.score)
    if (currentPlayer !== game.player)
      setCurrentPlayer(game.player)
  }

  useEffect(() => {
    const prev = document.body.style.overflow
    if (showRules) {
      document.body.style.overflow = 'hidden'
    }

    return () => {
      document.body.style.overflow = prev
    }
  }, [showRules])

  const handleQuit = () => {
    router.push('/')
  }

  const visibleRules = RULE_MODALS.filter((modal) => modal.category === activeCategory)

  return (
    <main className="flex min-h-screen items-center justify-center bg-[radial-gradient(circle_at_top,_rgba(255,215,145,0.12),_transparent_28%),linear-gradient(180deg,_#18110c_0%,_#0c0907_100%)] px-4 py-6 text-stone-100 sm:px-6 lg:px-8">
      <div className="mx-auto flex w-full max-w-5xl flex-col items-center gap-5">
        <header className={GAME_PAGE_THEME.headerPanel}>
          <button
            type="button"
            onClick={() => setShowRules(true)}
            aria-label="Open rules help"
            className={`${GAME_PAGE_THEME.iconButton} absolute right-16 top-4`}
          >
            <svg viewBox="0 0 24 24" className="h-5 w-5" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
              <path d="M12 18h.01" />
              <path d="M9.09 9a3 3 0 1 1 5.82 1c0 2-3 2-3 4" />
            </svg>
          </button>
          <button
            type="button"
            onClick={handleQuit}
            aria-label="Quit to menu"
            className={`${GAME_PAGE_THEME.iconButton} absolute right-4 top-4`}
          >
            <svg viewBox="0 0 24 24" className="h-5 w-5" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
              <path d="M10 17l5-5-5-5" />
              <path d="M15 12H3" />
              <path d="M21 3v18" />
            </svg>
          </button>

          <p className="text-[11px] uppercase tracking-[0.5em] text-amber-200/65">42 School</p>
          <h1 className="bg-[linear-gradient(180deg,_#fff7e7_0%,_#f6c77d_55%,_#d08a3f_100%)] bg-clip-text text-4xl font-black tracking-[0.1em] text-transparent sm:text-6xl">
            Gomoku
          </h1>
          <p className="mt-2 text-sm text-stone-400">Ninuki-Gomoku - Board 19x19</p>
        </header>

        <div className="flex min-h-0 w-full flex-col items-center gap-3">
          <div className="flex w-full flex-col gap-3 sm:flex-row sm:flex-wrap sm:items-center sm:justify-center">
            <div className={GAME_PAGE_THEME.statusPill}>
              <span className="mr-2 align-middle">Tour {turn + 1}</span>
              <span className={`inline-block h-4 w-4 rounded-full border align-middle ${getTurnOrbClass(currentPlayer)}`} />
            </div>
            <div className="flex flex-col gap-3">
              <div className="flex items-center gap-2">
                <span className="w-14 shrink-0 text-right text-[11px] font-semibold uppercase tracking-[0.35em] text-stone-400">
                  {getGaugeLabel('white', mode)}
                </span>
                {Array.from({ length: 10 }).map((_, i) => (
                  <div
                    key={`white-${i}`}
                    className={`h-4 w-4 rounded-full border transition-all ${getCaptureOrbClass(i < score[0], 'white')}`}
                  />
                ))}
              </div>
              <div className="flex items-center gap-2">
                <span className="w-14 shrink-0 text-right text-[11px] font-semibold uppercase tracking-[0.35em] text-stone-400">
                  {getGaugeLabel('black', mode)}
                </span>
                {Array.from({ length: 10 }).map((_, i) => (
                  <div
                    key={`black-${i}`}
                    className={`h-4 w-4 rounded-full border transition-all ${getCaptureOrbClass(i < score[1], 'black')}`}
                  />
                ))}
              </div>
            </div>
            <div className={GAME_PAGE_THEME.statusPill}>
              Bot response: {
                botResponseMs === 'pending' ?
                  "Thinking..." :
                botResponseMs === null ?
                  '...' :
                `${botResponseMs} ms`
              }
            </div>
          </div>

          <section className={GAME_PAGE_THEME.sectionPanel}>
            <GomokuBoard rules={rules} mode={mode} onUpdate={handleGameUpdate} onBotResponseTime={setBotResponseMs} />
          </section>
        </div>

        <HelpModal show={showRules} onClose={() => setShowRules(false)} rules={RULE_MODALS} />
      </div>
    </main>
  )
}
