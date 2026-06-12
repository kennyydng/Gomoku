"use client"

import { useState } from 'react'
import Link from 'next/link'
import { MENU_CARD_THEME } from './constants/ui'
import type { Rules } from './game/Gomoku'

export default function Home() {
  const [selectedMode, setSelectedMode] = useState<'local' | 'ai' | 'training'>('local')
  const [rules, setRules] = useState<Rules>({
    capture: true,
    captureUnperfect: true,
    foulOverline: false,
    overline: false,
    threeThree: true,
    fourFour: false,
    flanking: false,
    pass: true,
  })

  const ruleLabels: Record<keyof Rules, string> = {
    pass: 'Pass',
    capture: 'Capture',
    captureUnperfect: 'Capture a line of 5',
    foulOverline: 'Foul overline',
    overline: 'Overline',
    threeThree: 'Double free-three',
    fourFour: 'Double free-four',
    flanking: 'Flanking',
  }

  const blackRules: Array<keyof Rules> = ['overline', 'threeThree', 'fourFour', 'flanking']

  const applyRenjiPreset = () => {
    setRules({
      capture: true,
      captureUnperfect: true,
      foulOverline: true,
      overline: 'black',
      threeThree: 'black',
      fourFour: 'black',
      flanking: false,
      pass: true,
    })
  }

  const getRulesQueryString = () => {
    return Object.entries(rules)
      .map(([key, value]) => {
        if (typeof value === 'string') {
          return `${key}=${value === 'black' ? 'b' : '0'}`
        }
        return `${key}=${value ? '1' : '0'}`
      })
      .join('&')
  }

  const toggleRule = (key: keyof Rules) => {
    setRules(prev => {
      const current = prev[key]
      if (typeof current === 'boolean') {
        return { ...prev, [key]: !current }
      }
      return prev
    })
  }

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

        <section className="grid gap-4 grid-cols-1">
          <Link
            href={`/game?mode=local&${getRulesQueryString()}`}
            onClick={() => setSelectedMode('local')}
            className={`${MENU_CARD_THEME.base} ${
              selectedMode === 'local'
                ? MENU_CARD_THEME.selected
                : MENU_CARD_THEME.idle
            }`}
          >
            <div className="text-lg font-semibold text-amber-50">Player vs Player</div>
            <p className="mt-2 text-sm text-stone-400">Local two-player match.</p>
          </Link>

          <Link
            href={`/game?mode=ai&${getRulesQueryString()}`}
            onClick={() => setSelectedMode('ai')}
            className={`${MENU_CARD_THEME.base} ${
              selectedMode === 'ai'
                ? MENU_CARD_THEME.selected
                : MENU_CARD_THEME.idle
            }`}
          >
            <div className="text-lg font-semibold text-amber-50">Player vs Bot</div>
            <p className="mt-2 text-sm text-stone-400">The bot responds after each player move. Good luck !</p>
          </Link>

          <Link
            href={`/game?mode=training&${getRulesQueryString()}`}
            onClick={() => setSelectedMode('training')}
            className={`${MENU_CARD_THEME.base} ${
              selectedMode === 'training'
                ? MENU_CARD_THEME.selected
                : MENU_CARD_THEME.idle
            }`}
          >
            <div className="text-lg font-semibold text-amber-50">Training Mode</div>
            <p className="mt-2 text-sm text-stone-400">Play against the bot with suggested moves. You can undo the last actions to review variations.</p>
          </Link>
        </section>

        <section className="w-full max-w-2xl space-y-4">
          <h2 className="text-lg font-semibold text-amber-100">Customize Game Rules</h2>
          <div className="flex flex-wrap gap-3">
            <button
              type="button"
              onClick={applyRenjiPreset}
              className="rounded-full border border-amber-400/20 bg-amber-300/10 px-4 py-2 text-xs font-semibold uppercase tracking-[0.25em] text-amber-100 transition hover:border-amber-400/40 hover:bg-amber-300/15"
            >
              Renji rules
            </button>
          </div>
          <div className="rounded-xl border border-stone-700/40 bg-black/20 p-4 space-y-3">
            {Object.entries(rules).map(([key, value]) => {
              const ruleKey = key as keyof Rules
              const isBlackRule = blackRules.includes(ruleKey)
              const label = ruleLabels[ruleKey]
              
              return (
                <div key={key} className="flex items-center justify-between gap-3">
                  <span className="text-sm text-stone-200">{label}</span>
                  {isBlackRule ? (
                    <select
                      value={typeof value === 'string' ? value : (value ? 'true' : 'false')}
                      onChange={(e) => {
                        if (e.target.value === 'black') {
                          setRules(prev => ({ ...prev, [key]: 'black' as any }))
                        } else {
                          setRules(prev => ({ ...prev, [key]: e.target.value === 'true' }))
                        }
                      }}
                      className="text-xs bg-stone-900 border border-stone-700 rounded px-2 py-1 text-stone-200"
                    >
                      <option value="false">Disabled</option>
                      <option value="true">Enabled</option>
                      <option value="black">Black only</option>
                    </select>
                  ) : (
                    <label className="flex items-center gap-2 cursor-pointer">
                      <input
                        type="checkbox"
                        checked={typeof value === 'boolean' ? value : value === 'black'}
                        onChange={() => toggleRule(key as keyof Rules)}
                        className="w-4 h-4 rounded border-stone-600 bg-stone-900 cursor-pointer"
                      />
                    </label>
                  )}
                </div>
              )
            })}
          </div>
        </section>

      </div>
    </main>
  )
}

