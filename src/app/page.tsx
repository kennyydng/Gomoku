"use client"

import Link from 'next/link'
import { useState } from 'react'

export default function Home() {
  const [selectedMode, setSelectedMode] = useState<'local' | 'ai'>('local')

  return (
    <main className="min-h-screen bg-[radial-gradient(circle_at_top,_rgba(255,215,145,0.16),_transparent_28%),linear-gradient(180deg,_#1a120d_0%,_#0c0907_100%)] px-4 py-10 text-stone-100 sm:px-6 lg:px-8">
      <div className="mx-auto flex w-full max-w-4xl flex-col items-center gap-10">
        <header className="space-y-4 text-center">
          <p className="text-[11px] uppercase tracking-[0.52em] text-amber-200/70">42 School</p>
          <h1 className="bg-[linear-gradient(180deg,_#fff7e7_0%,_#f6c77d_55%,_#d08a3f_100%)] bg-clip-text text-5xl font-black tracking-[0.08em] text-transparent sm:text-7xl">
            Gomoku
          </h1>
          <p className="text-sm text-stone-300 sm:text-base">Ninuki-Gomoku - Board 19x19</p>
        </header>

        <section className="grid w-full gap-4 grid-cols-1">
          <Link
            href="/game?mode=local"
            onClick={() => setSelectedMode('local')}
            className={`rounded-2xl border p-6 text-left transition ${
              selectedMode === 'local'
                ? 'border-amber-300 bg-amber-200/10 shadow-[0_0_0_1px_rgba(252,211,77,0.16)]'
                : 'border-stone-700/40 bg-[#120d09] hover:border-amber-700/50'
            }`}
          >
            <div className="text-lg font-semibold text-amber-50">Player vs Player</div>
            <p className="mt-2 text-sm text-stone-400">Local two-player match.</p>
          </Link>

          <Link
            href="/game?mode=ai"
            onClick={() => setSelectedMode('ai')}
            className={`rounded-2xl border p-6 text-left transition ${
              selectedMode === 'ai'
                ? 'border-amber-300 bg-amber-200/10 shadow-[0_0_0_1px_rgba(252,211,77,0.16)]'
                : 'border-stone-700/40 bg-[#120d09] hover:border-amber-700/50'
            }`}
          >
            <div className="text-lg font-semibold text-amber-50">Player vs Bot</div>
            <p className="mt-2 text-sm text-stone-400">The bot responds after each player move.</p>
          </Link>
        </section>
      </div>
    </main>
  )
}
