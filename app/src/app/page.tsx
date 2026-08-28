"use client"

import { useState } from 'react'
import Link from 'next/link'
import { MENU_CARD_THEME } from './constants/ui'
import type { Rules } from './game/Gomoku'
import HelpModal, { getRuleModals } from './components/rules/HelpModal'

function RuleInfo({ label, description }: { label: string; description: string }) {
  return (
    <span className="relative inline-flex shrink-0 items-center group">
      <button
        type="button"
        aria-label={`Help for ${label}`}
        className="flex h-5 w-5 items-center justify-center rounded-full border border-amber-400/30 bg-amber-300/10 text-[10px] font-bold text-amber-100 transition hover:border-amber-300/60 hover:bg-amber-300/20 focus:outline-none focus:ring-2 focus:ring-amber-300/40"
      >
        i
      </button>
      <span className="pointer-events-none absolute left-1/2 top-full z-20 mt-2 hidden w-64 -translate-x-1/2 rounded-xl border border-amber-900/35 bg-[#17110c] px-3 py-2 text-xs leading-5 text-stone-200 shadow-[0_18px_40px_rgba(0,0,0,0.45)] group-hover:block group-focus-within:block">
        {description}
      </span>
    </span>
  )
}

export default function Home() {
  const [selectedMode, setSelectedMode] = useState<'local' | 'ai' | 'training'>('local')
  const [showRules, setShowRules] = useState(false)
  const [rules, setRules] = useState<Rules>({
    capture: true,
    captureUnperfect: true,
    overline: 'win',
    threeThree: true,
    fourFour: false,
    grid: '19x19',
  })

  const presets = [
    {
      key: '42-mandatory',
      label: '42 Mandatory',
      rules: {
        capture: true,
        captureUnperfect: true,
        overline: 'win',
        threeThree: true,
        fourFour: false,
        grid: '19x19',
      } as Rules,
    },
    {
      key: 'classic',
      label: 'Classic',
      rules: {
        capture: false,
        captureUnperfect: false,
        overline: 'win',
        threeThree: false,
        fourFour: false,
        grid: '15x15',
      } as Rules,
    },
    {
      key: 'renju',
      label: 'Renju',
      rules: {
        capture: false,
        captureUnperfect: false,
        overline: 'forbiddenBlack',
        threeThree: 'black',
        fourFour: 'black',
        grid: '15x15',
      } as Rules,
    },
    {
      key: 'ninuki-renju',
      label: 'Ninuki-renju',
      rules: {
        // Double-trois interdit à noir (comme Renju), mais double-quatre
        // autorisé (contrairement à Renju — la capture change l'équilibre)
        // et overline qui ne gagne pour PERSONNE (contrairement à Renju où
        // blanc gagne avec).
        capture: true,
        captureUnperfect: true,
        overline: 'legal',
        threeThree: 'black',
        fourFour: false,
        grid: '19x19',
      } as Rules,
    },
    {
      key: 'pente',
      label: 'Pente',
      rules: {
        capture: true,
        captureUnperfect: false,
        overline: 'win',
        threeThree: false,
        fourFour: false,
        grid: '19x19',
      } as Rules,
    },
  ] as const

  const ruleLabels: Record<keyof Rules, string> = {
    capture: 'Capture',
    captureUnperfect: 'Capture a line of 5',
    overline: 'Overline',
    threeThree: 'Double free-three',
    fourFour: 'Double free-four',
    grid: 'Grid size',
  }

  const ruleDescriptions: Record<keyof Rules, string> = {
    capture: 'Enables capturing a pair of opponent stones when they are flanked by your own stones.',
    captureUnperfect: 'Sometimes prevents a line of 5 from winning outright if it can be immediately broken by a capture.',
    overline: 'Decides what happens for a line of 6+ stones: win, legal move but no win, or illegal move (for both players, or for Black only, like in Renju).',
    threeThree: 'Forbids moves that create two "free-three" threats at the same time.',
    fourFour: 'Forbids moves that create two "four" threats at the same time (always Black-only, like in Renju).',
    grid: 'Chooses the board size: 15x15 for faster games, 19x19 for a more open game.',
  }

  const blackRules: Array<keyof Rules> = ['threeThree', 'fourFour']

  // Cocher la case interdit-elle un coup, ou active-t-elle un mécanisme de jeu ?
  // Le nom seul ne le dit pas, d'où un label "No X" explicite pour les règles qui interdisent un coup
  // (ex: "No double free-three" coché = interdit de jouer ce coup).
  const ruleBadges: Partial<Record<keyof Rules, string>> = {
    captureUnperfect: 'No win on breakable five',
    threeThree: 'No double free-three',
    fourFour: 'No double free-four',
  }

  const applyPreset = (presetRules: Rules) => {
    setRules(presetRules)
  }

  const getRulesQueryString = () => {
    return Object.entries(rules)
      .map(([key, value]) => {
        if (key === 'grid' || key === 'overline') {
          return `${key}=${value}`
        }
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
          <div className="flex items-center justify-between">
            <h2 className="text-lg font-semibold text-amber-100">Customize Game Rules</h2>
            <button
              type="button"
              onClick={() => setShowRules(true)}
              className="rounded-full border border-amber-400/20 bg-amber-300/10 px-4 py-1.5 text-xs font-semibold uppercase tracking-[0.24em] text-amber-100 transition hover:border-amber-400/40 hover:bg-amber-300/15"
            >
              Rules help
            </button>
          </div>
          <div className="grid gap-4 sm:grid-cols-2 lg:grid-cols-3">
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
              const description = ruleDescriptions[ruleKey]
              
              return (
                <div key={key} className="flex items-center justify-between gap-3">
                  <div className="flex min-w-0 items-center gap-2">
                    <span className="text-sm text-stone-200">{ruleBadges[ruleKey] ?? label}</span>
                    <RuleInfo label={label} description={description} />
                  </div>
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
                  ) : ruleKey === 'overline' ? (
                    <select
                      value={value as Rules['overline']}
                      onChange={(e) =>
                        setRules(prev => ({ ...prev, overline: e.target.value as Rules['overline'] }))
                      }
                      className="text-xs bg-stone-900 border border-stone-700 rounded px-2 py-1 text-stone-200"
                    >
                      <option value="win">Win</option>
                      <option value="legal">Legal, no win</option>
                      <option value="forbidden">Forbidden</option>
                      <option value="forbiddenBlack">Forbidden (black only)</option>
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
                      {ruleKey !== 'fourFour' ? <option value="true">Enabled</option> : null}
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

      <HelpModal show={showRules} onClose={() => setShowRules(false)} rules={getRuleModals(rules)} />
    </main>
  )
}

