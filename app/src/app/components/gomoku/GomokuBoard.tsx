"use client"

import { memo } from 'react'
import { useEffect } from 'react'
import GomokuBoardSurface from './GomokuBoardSurface'
import { Gomoku, useGomokuGame } from './useGomokuGame'
import type { GameMode, Rules, Player, Position } from './useGomokuGame'

interface GomokuBoardProps {
  mode: GameMode
  rules: Rules
  onUpdate?: (game: Gomoku) => void
  onBotResponseTime?: (ms: number | null) => void
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
  const {
    game,
    resetGame,
    isLocked,
    historyLength,
    handleUndo,
    hintCell,
    hoveredCell,
    setHoveredCell,
    handleHumanMove,
  } = useGomokuGame({ mode, rules, onUpdate, onBotResponseTime })

  const resultLabel = resultString(mode,game.result)

  return (
    <div className="w-full">
      <div className="relative mx-auto w-full max-w-[min(88vmin,76vh,720px)] sm:max-w-[min(84vmin,72vh,680px)]">
        {mode === 'training' || mode === 'local' ? (
          <div className="mb-3 flex justify-end">
            <button
              type="button"
              onClick={handleUndo}
              disabled={historyLength === 0 || isLocked}
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
