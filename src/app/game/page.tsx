"use client"

import { useEffect, useState } from 'react'
import { useRouter, useSearchParams } from 'next/navigation'
import GomokuBoard, { type GameMode, type GameStats } from '../components/GomokuBoard'

function getTurnOrbClass(currentPlayer: 1 | 2) {
  return currentPlayer === 1
    ? 'bg-[radial-gradient(circle_at_30%_30%,_#8a8a8a_0%,_#222222_48%,_#050505_100%)] border-stone-700/60 shadow-[inset_0_2px_4px_rgba(255,255,255,0.16),0_8px_14px_rgba(0,0,0,0.5)]'
    : 'bg-[radial-gradient(circle_at_30%_30%,_#ffffff_0%,_#e8e0d2_48%,_#a89475_100%)] border-stone-300/70 shadow-[inset_0_2px_4px_rgba(255,255,255,0.45),0_8px_14px_rgba(0,0,0,0.35)]'
}

function getCaptureOrbClass(isFilled: boolean, tone: 'black' | 'white') {
  if (tone === 'black') {
    return isFilled
      ? 'bg-[radial-gradient(circle_at_30%_30%,_#8a8a8a_0%,_#232323_48%,_#050505_100%)] border-stone-700/60 shadow-[inset_0_2px_4px_rgba(255,255,255,0.16),0_8px_14px_rgba(0,0,0,0.5)] scale-110'
      : 'bg-[radial-gradient(circle_at_30%_30%,_rgba(122,122,122,0.42)_0%,_rgba(49,49,49,0.34)_48%,_rgba(15,15,15,0.3)_100%)] border-stone-500/20 shadow-[inset_0_1px_2px_rgba(255,255,255,0.08),0_4px_8px_rgba(0,0,0,0.16)] opacity-55'
  }

  return isFilled
    ? 'bg-[radial-gradient(circle_at_30%_30%,_#ffffff_0%,_#e8e0d2_48%,_#a89475_100%)] border-stone-300/70 shadow-[inset_0_2px_4px_rgba(255,255,255,0.45),0_8px_14px_rgba(0,0,0,0.35)] scale-110'
    : 'bg-[radial-gradient(circle_at_30%_30%,_rgba(255,255,255,0.42)_0%,_rgba(226,218,206,0.3)_48%,_rgba(168,148,117,0.24)_100%)] border-stone-300/20 shadow-[inset_0_1px_2px_rgba(255,255,255,0.16),0_4px_8px_rgba(0,0,0,0.12)] opacity-55'
}

function getGaugeLabel(tone: 'black' | 'white', mode: GameMode) {
  if (mode === 'ai') {
    return tone === 'black' ? 'Player' : 'Bot'
  }

  return 'Player'
}

export default function GamePage() {
  const router = useRouter()
  const searchParams = useSearchParams()
  const modeParam = searchParams?.get('mode') as GameMode | null
  const [botResponseMs, setBotResponseMs] = useState<number | null>(null)
  const [gameStats, setGameStats] = useState<GameStats>({
    capturesBlack: 0,
    capturesWhite: 0,
    currentPlayer: 1,
    isLocked: false,
    winner: null,
  })

  useEffect(() => {
    if (!modeParam) {
      // If no mode, return to home
      router.replace('/')
    }
  }, [modeParam, router])

  useEffect(() => {
    // mark in-progress so the home menu can't change mode mid-game
    try {
      sessionStorage.setItem('gomoku:inProgress', '1')
    } catch {}

    const onBeforeUnload = (e: BeforeUnloadEvent) => {
      e.preventDefault()
      e.returnValue = ''
    }

    window.addEventListener('beforeunload', onBeforeUnload)

    return () => {
      try {
        sessionStorage.removeItem('gomoku:inProgress')
      } catch {}
      window.removeEventListener('beforeunload', onBeforeUnload)
    }
  }, [])

  const handleStatsChange = (nextStats: GameStats) => {
    setGameStats(nextStats)
  }

  const handleBotResponse = (ms: number) => {
    setBotResponseMs(ms)
  }

  const handleQuit = () => {
    try {
      sessionStorage.removeItem('gomoku:inProgress')
    } catch {}
    router.push('/')
  }

  if (!modeParam) {
    return null
  }

  return (
    <main className="flex min-h-screen items-center justify-center bg-[radial-gradient(circle_at_top,_rgba(255,215,145,0.12),_transparent_28%),linear-gradient(180deg,_#18110c_0%,_#0c0907_100%)] px-4 py-6 text-stone-100 sm:px-6 lg:px-8">
      <div className="mx-auto flex w-full max-w-5xl flex-col items-center gap-5">
        <header className="relative w-full rounded-[2rem] border border-amber-900/35 bg-[linear-gradient(180deg,_rgba(29,20,13,0.98),_rgba(18,12,8,0.98))] px-6 py-5 text-center shadow-[0_24px_90px_rgba(0,0,0,0.35)] sm:px-8">
          <button
            type="button"
            onClick={handleQuit}
            aria-label="Quit to menu"
            className="absolute right-4 top-4 inline-flex h-11 w-11 items-center justify-center rounded-full border border-stone-700/50 bg-black/25 text-stone-100 transition hover:border-amber-500/50 hover:bg-black/40"
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
            <div className="rounded-full border border-amber-400/20 bg-amber-300/10 px-4 py-2 text-sm text-amber-100">
              <span className="mr-2 align-middle">Turn:</span>
              <span
                className={`inline-block h-4 w-4 rounded-full border align-middle transform-gpu ${getTurnOrbClass(gameStats.currentPlayer)}`}
              />
            </div>
            <div className="flex flex-col gap-3">
              <div className="flex items-center gap-2">
                <span className="w-14 shrink-0 text-right text-[11px] font-semibold uppercase tracking-[0.35em] text-stone-400">
                  {getGaugeLabel('black', modeParam)}
                </span>
                {Array.from({ length: 10 }).map((_, i) => (
                  <div
                    key={`black-${i}`}
                    className={`h-4 w-4 rounded-full border transition-all transform-gpu ${getCaptureOrbClass(
                      i < gameStats.capturesBlack,
                      'black',
                    )}`}
                  />
                ))}
              </div>
              <div className="flex items-center gap-2">
                <span className="w-14 shrink-0 text-right text-[11px] font-semibold uppercase tracking-[0.35em] text-stone-400">
                  {getGaugeLabel('white', modeParam)}
                </span>
                {Array.from({ length: 10 }).map((_, i) => (
                  <div
                    key={`white-${i}`}
                    className={`h-4 w-4 rounded-full border transition-all transform-gpu ${getCaptureOrbClass(
                      i < gameStats.capturesWhite,
                      'white',
                    )}`}
                  />
                ))}
              </div>
            </div>
            {modeParam === 'ai' ? (
              <div className="rounded-full border border-amber-400/20 bg-amber-300/10 px-4 py-2 text-sm text-amber-100">
                Bot response: {botResponseMs === null ? '...' : `${botResponseMs} ms`}
              </div>
            ) : null}
          </div>

          <section className="w-full rounded-[2rem] border border-amber-900/35 bg-[linear-gradient(180deg,_rgba(29,20,13,0.98),_rgba(18,12,8,0.98))] p-4 shadow-[0_24px_90px_rgba(0,0,0,0.35)] sm:p-6">
            <GomokuBoard mode={modeParam} onStatsChange={handleStatsChange} onBotResponseTime={handleBotResponse} />
          </section>
        </div>
      </div>
    </main>
  )
}
