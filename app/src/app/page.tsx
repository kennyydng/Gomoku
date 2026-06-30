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
    swap2: false,
    grid: "19x19",
  })

  const presets = [
     {
      key: 'free-style',
      label: 'Free-style',
      rules: {
        capture: false,
        captureUnperfect: false,
        foulOverline: false,
        overline: false,
        threeThree: false,
        fourFour: false,
        flanking: false,
        pass: true,
        swap2: false,
        grid: "15x15",
      } as Rules,
    },
    {
      key: 'pente',
      label: 'Pente',
      rules: {
        capture: true,
        captureUnperfect: true,
        foulOverline: false,
        overline: false,
        threeThree: false,
        fourFour: false,
        flanking: false,
        pass: true,
        swap2: false,
        grid: "19x19",
      } as Rules,
    },
    {
      key: 'omok',
      label: 'Omok',
      rules: {
        capture: false,
        captureUnperfect: false,
        foulOverline: false,
        overline: true,
        threeThree: false,
        fourFour: false,
        flanking: false,
        pass: true,
        swap2: false,
        grid: "19x19",
      } as Rules,
    },
    {
      key: 'renju',
      label: 'Renju',
      rules: {
        capture: false,
        captureUnperfect: false,
        foulOverline: true,
        overline: 'black',
        threeThree: 'black',
        fourFour: 'black',
        flanking: false,
        pass: true,
        swap2: false,
        grid: "19x19",
      } as Rules,
    },
  ] as const

  const ruleLabels: Record<keyof Rules, string> = {
    pass: 'Pass',
    capture: 'Capture',
    captureUnperfect: 'Capture a line of 5',
    foulOverline: 'Foul overline',
    overline: 'Overline',
    threeThree: 'Double free-three',
    fourFour: 'Double free-four',
    flanking: 'Flanking',
    swap2: 'Swap2',
    grid: 'Grid size',
  }

  const blackRules: Array<keyof Rules> = ['overline', 'threeThree', 'fourFour', 'flanking']

  const applyPreset = (presetRules: Rules) => {
    setRules(presetRules)
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

  const getGameHref = (mode: 'local' | 'ai' | 'training') => {
    return `/game?mode=${mode}&${getRulesQueryString()}`
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
          <h1 className="bg-[linear-gradient(180deg,_#fff7e7_0%,_#f6c77d_55%,_#d08a3f_100%)] bg-clip-text text-5xl font-black tracking-[0.08em] text-transparent sm:text-7xl">
            Gomoku
          </h1>
        </header>

        <section className="grid gap-2 sm:grid-cols-3">
          <Link
            href={getGameHref('local')}
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
            href={getGameHref('ai')}
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
            href={getGameHref('training')}
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
          <div className="grid gap-4 sm:grid-cols-4">
            {presets.map((preset) => (
              <button
                key={preset.key}
                type="button"
                onClick={() => applyPreset(preset.rules)}
                className="rounded-2xl border border-amber-400/20 bg-amber-300/10 px-4 py-4 text-left transition hover:border-amber-400/40 hover:bg-amber-300/15"
              >
                <div className="text-sm font-semibold uppercase tracking-[0.24em] text-amber-100">{preset.label}</div>
              </button>
            ))}
          </div>

          <div className="rounded-xl border border-stone-700/40 bg-black/20 p-4 space-y-3">
            {Object.entries(rules).map(([key, value]) => {
              const ruleKey = key as keyof Rules
              const isBlackRule = blackRules.includes(ruleKey)
              const label = ruleLabels[ruleKey]
              
              return (
                <div key={key} className="flex items-center justify-between gap-3">
                  <span className="text-sm text-stone-200">{label}</span>
                  {ruleKey === 'grid' ? (
                    <select
                      value={value as string}
                      onChange={(e) =>
                        setRules(prev => ({ ...prev, grid: e.target.value as Rules['grid'] }))
                      }
                      className="text-xs bg-stone-900 border border-stone-700 rounded px-2 py-1 text-stone-200"
                    >
                      <option value="15x15">15x15</option>
                      <option value="19x19">19x19</option>
                    </select>
                  ) : isBlackRule ? (
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

