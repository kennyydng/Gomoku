"use client"

import { memo, useEffect, useState } from 'react'
import GomokuBoardSurface from './GomokuBoardSurface'
import { Gomoku } from '../../game/Gomoku'
import type { Rules, Player, Position } from '../../game/Gomoku'

type GameMode = 'local' | 'ai' | 'training'

interface GomokuBoardProps {
  mode: GameMode
  rules: Rules
  onUpdate?: (game: Gomoku) => void
  onBotResponseTime?: (ms: "pending" | number | null) => void
}

function resultString(mode: GameMode, result: Gomoku['result']) {
  if (result === null) return null
  if (result === 'draw') return 'Draw!'

  return [
    mode === 'local' ? "Black wins!" : "Player wins!",
    mode === 'local' ? "White wins!" : "Bot wins!",
  ][result]
}

function GomokuBoard({ mode, rules, onUpdate, onBotResponseTime }: GomokuBoardProps) {
  const [game, setGameState] = useState<Gomoku>(() => new Gomoku(rules))
  useEffect(() => { onUpdate?.(game) }, [onUpdate, game])

  const [hoveredCell, setHoveredCell] = useState<Position | null>(null)
  const [hintCell, setHintCell] = useState<Position | null>(null)
  const [history, setHistory] = useState<Gomoku[]>([])

  const resetGame = () => {
    setGameState(new Gomoku(rules))
    setHistory([])
    onBotResponseTime?.(null)
  }

  const aiPlayer: Player = 1

  const handleUndo = () => {
    if (history.length === 0) return
    const snapshot = history.at(-1)
    if (!snapshot) return
    setGameState(new Gomoku(snapshot))
    setHistory(history.slice(0,-1))
    onBotResponseTime?.(null)
  }

  const isHumanMove = mode === 'local' || game.player !== aiPlayer
  const isLocked = !isHumanMove || game.result !== null
  const shouldSuggestMove = mode === 'local' || mode === 'training'

  const handleHumanMove = (pos: Position) => {
    if (isLocked) return
    const move = game.resolveMove(pos)
    console.log(move)
    if (move) {
      setHistory([...history, game])
      const next = new Gomoku(game)
      next.applyResolvedMove(move)
      setGameState(next)
    }
  }

  const fetchBotMove = async (controller: AbortController) => {
    onBotResponseTime?.("pending")
    try {
      const TIMEOUT = 10000
      const timeoutId = window.setTimeout(() => controller.abort(), TIMEOUT)

      const response = await fetch('/api/bot', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ game }),
        signal: controller.signal,
      })

      window.clearTimeout(timeoutId)

      if (!response.ok) throw new Error('Bot response failed')

      const data: { move?: Position, time?: number } = await response.json()
      onBotResponseTime?.(data.time ?? null)
      return data.move
    } catch (e) {
      onBotResponseTime?.(null)
      if (!(e instanceof DOMException)) console.error(e)
    }
  }

  const handleAIMove = async (controller: AbortController) => {
    const pos = await fetchBotMove(controller)
    if (!pos) return
    const move = game.resolveMove(pos)
    if (!move) return
    setHistory([...history, game])
    const next = new Gomoku(game)
    next.applyResolvedMove(move)
    setGameState(next)
  }

  const handleAIHint = async (controller: AbortController) => {
    const pos = await fetchBotMove(controller)
    if (pos) setHintCell(pos)
  }

  useEffect(() => {
    if (game.result === null) {
      const controller = new AbortController()
      if (!isHumanMove) {
        handleAIMove(controller)
        return () => controller.abort()
      } else if (shouldSuggestMove) {
        handleAIHint(controller)
        return () => controller.abort()
      }
    }
  }, [game])

  const resultLabel = resultString(mode,game.result)

  return (
    <div className="w-full">
      <div className="relative mx-auto w-full max-w-[min(88vmin,76vh,720px)] sm:max-w-[min(84vmin,72vh,680px)]">
        {mode === 'training' || mode === 'local' ? (
          <div className="mb-3 flex justify-end">
            <button
              type="button"
              onClick={handleUndo}
              disabled={history.length === 0 || isLocked}
              className="rounded-full border border-amber-400/20 bg-amber-300/10 px-4 py-2 text-xs font-semibold uppercase tracking-[0.28em] text-amber-100 transition hover:border-amber-400/40 hover:bg-amber-300/15 disabled:cursor-not-allowed disabled:opacity-40"
            >
              Undo last move
            </button>
          </div>
        ) : null}

        <>
          <GomokuBoardSurface
            game={game}
            isLocked={isLocked}
            hintCell={hintCell}
            hoveredCell={hoveredCell}
            onCellHover={setHoveredCell}
            onCellClick={(pos) => void handleHumanMove(pos)}
          />

          {(game.result !== null) && (
            <div className="absolute inset-0 z-20 flex items-center justify-center">
              <div className="mx-4 w-full max-w-3xl rounded-3xl border border-amber-900/50 bg-gradient-to-br from-black/70 via-black/60 to-black/50 p-6 text-center shadow-[0_30px_80px_rgba(0,0,0,0.6)]">
                <div className="mb-4 text-4xl font-extrabold tracking-tight text-amber-100 sm:text-6xl">
                  <span className="inline-block bg-clip-text text-transparent bg-[linear-gradient(90deg,#ffd37a,#ffb86b,#ffd37a)]">{resultLabel}</span>
                </div>
                <p className="mb-6 text-sm text-stone-300">The board is locked — start a new game or review the position.</p>
                <div className="flex items-center justify-center gap-3">
                  <button
                    type="button"
                    onClick={resetGame}
                    className="rounded-full border border-amber-400/20 bg-amber-300/10 px-5 py-3 text-sm font-semibold uppercase tracking-[0.18em] text-amber-100 transition hover:scale-105"
                  >
                    New game
                  </button>
                  {mode === 'training' || mode === 'local' ? (
                    <button
                      type="button"
                      onClick={handleUndo}
                      className="rounded-full border border-amber-400/20 bg-amber-300/10 px-5 py-3 text-sm font-semibold uppercase tracking-[0.18em] text-amber-100 transition hover:scale-105 disabled:cursor-not-allowed disabled:opacity-40"
                    >
                      Undo last move
                    </button>
                  ) : null}
                </div>
              </div>
            </div>
          )}
        </>
      </div>
    </div>
  )
}

export default memo(GomokuBoard)
