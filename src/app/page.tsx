"use client"

import { useCallback, useEffect, useState } from 'react'

import GomokuBoard, { type GameMode, type GameStats } from './components/GomokuBoard'

function Timer() {
  const [elapsed, setElapsed] = useState(0)

  useEffect(() => {
    const startedAt = performance.now()
    let frameId = 0

    const tick = () => {
      setElapsed(Math.floor(performance.now() - startedAt))
      frameId = window.requestAnimationFrame(tick)
    }

    frameId = window.requestAnimationFrame(tick)

    return () => window.cancelAnimationFrame(frameId)
  }, [])

  return <span className="font-mono text-2xl font-semibold tracking-[0.18em] text-amber-50">{elapsed.toLocaleString('fr-FR')} ms</span>
}

export default function Home() {
  const [mode, setMode] = useState<GameMode>('local')
  const [stats, setStats] = useState<GameStats>({
    capturesBlack: 0,
    capturesWhite: 0,
    currentPlayer: 1,
    isLocked: false,
    winner: null,
  })

  const handleStatsChange = useCallback((nextStats: GameStats) => {
    setStats(nextStats)
  }, [])

  return (
    <main className="min-h-screen bg-[radial-gradient(circle_at_top,_rgba(255,231,186,0.14),_transparent_28%),linear-gradient(180deg,_#16100c_0%,_#0d0907_100%)] px-4 py-8 text-stone-100 sm:px-6 lg:px-8">
      <div className="mx-auto flex w-full max-w-7xl flex-col gap-6">
        <section className="rounded-[2rem] border border-amber-900/40 bg-[#1b140f]/90 p-6 shadow-[0_20px_80px_rgba(0,0,0,0.35)] backdrop-blur-sm sm:p-8">
          <div className="flex flex-col gap-6 lg:flex-row lg:items-end lg:justify-between">
            <div className="space-y-3">
              <p className="text-xs uppercase tracking-[0.45em] text-amber-200/70">École 42</p>
              <h1 className="text-4xl font-semibold tracking-tight text-amber-50 sm:text-5xl">Gomoku</h1>
              <p className="max-w-2xl text-sm leading-6 text-stone-300 sm:text-base">
                Plateau 19×19, capture en cours, et architecture prête pour brancher l&apos;IA ou le moteur Prolog plus tard.
              </p>
            </div>

            <div className="rounded-2xl border border-amber-700/40 bg-amber-950/40 px-5 py-4 text-right">
              <p className="text-[11px] uppercase tracking-[0.35em] text-amber-200/60">Chronomètre</p>
              <Timer />
            </div>
          </div>

          <div className="mt-6 grid gap-4 lg:grid-cols-[1.1fr_0.9fr]">
            <div className="rounded-2xl border border-amber-900/40 bg-[#261b13] p-4">
              <p className="mb-3 text-sm font-medium uppercase tracking-[0.3em] text-amber-200/70">Mode de jeu</p>
              <div className="grid gap-3 sm:grid-cols-2">
                <button
                  type="button"
                  onClick={() => setMode('local')}
                  className={`rounded-2xl border px-4 py-4 text-left transition ${
                    mode === 'local'
                      ? 'border-amber-300 bg-amber-200/15 text-amber-50 shadow-[0_0_0_1px_rgba(252,211,77,0.2)]'
                      : 'border-amber-950/60 bg-[#18110c] text-stone-300 hover:border-amber-700/60 hover:bg-[#1f160f]'
                  }`}
                >
                  <div className="text-sm font-semibold">Humain vs Humain</div>
                  <div className="mt-1 text-xs text-stone-400">Partie locale à deux sur la même machine.</div>
                </button>
                <button
                  type="button"
                  onClick={() => setMode('ai')}
                  className={`rounded-2xl border px-4 py-4 text-left transition ${
                    mode === 'ai'
                      ? 'border-amber-300 bg-amber-200/15 text-amber-50 shadow-[0_0_0_1px_rgba(252,211,77,0.2)]'
                      : 'border-amber-950/60 bg-[#18110c] text-stone-300 hover:border-amber-700/60 hover:bg-[#1f160f]'
                  }`}
                >
                  <div className="text-sm font-semibold">Humain vs IA</div>
                  <div className="mt-1 text-xs text-stone-400">Le coup humain déclenche un POST vers /api/bot.</div>
                </button>
              </div>
            </div>

            <div className="grid gap-3 rounded-2xl border border-amber-900/40 bg-[#261b13] p-4 sm:grid-cols-2 lg:grid-cols-1 xl:grid-cols-2">
              <div className="rounded-2xl border border-stone-700/40 bg-[#120d09] p-4">
                <p className="text-[11px] uppercase tracking-[0.3em] text-stone-400">Captures noir</p>
                <p className="mt-2 text-3xl font-semibold text-stone-100">{stats.capturesBlack}</p>
              </div>
              <div className="rounded-2xl border border-stone-700/40 bg-[#120d09] p-4">
                <p className="text-[11px] uppercase tracking-[0.3em] text-stone-400">Captures blanc</p>
                <p className="mt-2 text-3xl font-semibold text-stone-100">{stats.capturesWhite}</p>
              </div>
            </div>
          </div>
        </section>

        <section className="grid gap-6 xl:grid-cols-[1fr_auto] xl:items-start">
          <div className="rounded-[2rem] border border-amber-900/40 bg-[#211913]/95 p-4 shadow-[0_18px_70px_rgba(0,0,0,0.3)] sm:p-6">
            <GomokuBoard mode={mode} onStatsChange={handleStatsChange} />
          </div>

          <aside className="w-full max-w-xl rounded-[2rem] border border-amber-900/40 bg-[#1a130e]/95 p-5 text-sm text-stone-300 xl:w-80">
            <p className="text-xs uppercase tracking-[0.35em] text-amber-200/60">État courant</p>
            <div className="mt-4 space-y-3">
              <div className="rounded-2xl border border-stone-700/40 bg-[#120d09] p-4">
                <div className="text-xs uppercase tracking-[0.25em] text-stone-500">Joueur actif</div>
                <div className="mt-2 text-lg font-semibold text-amber-50">{stats.currentPlayer === 1 ? 'Noir' : 'Blanc'}</div>
              </div>
              <div className="rounded-2xl border border-stone-700/40 bg-[#120d09] p-4">
                <div className="text-xs uppercase tracking-[0.25em] text-stone-500">Mode</div>
                <div className="mt-2 text-lg font-semibold text-amber-50">
                  {mode === 'local' ? 'Humain vs Humain' : 'Humain vs IA'}
                </div>
              </div>
              <div className="rounded-2xl border border-stone-700/40 bg-[#120d09] p-4">
                <div className="text-xs uppercase tracking-[0.25em] text-stone-500">Verrouillage</div>
                <div className="mt-2 text-lg font-semibold text-amber-50">{stats.isLocked ? 'IA en réflexion' : 'Libre'}</div>
              </div>
            </div>
          </aside>
        </section>
      </div>
    </main>
  )
}
