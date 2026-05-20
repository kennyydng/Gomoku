"use client"

import { memo } from 'react'
import GomokuBoardSurface from './GomokuBoardSurface'
import { type GameMode, type GameStats, useGomokuGame } from './useGomokuGame'

export type { GameMode, GameStats } from './useGomokuGame'

interface GomokuBoardProps {
  mode: GameMode
  onStatsChange?: (stats: GameStats) => void
  onBotResponseTime?: (ms: number | null) => void
}

function GomokuBoard({ mode, onStatsChange, onBotResponseTime }: GomokuBoardProps) {
  const {
    board,
    currentPlayer,
    hoveredCell,
    isLocked,
    winner,
    historyLength,
    handleHumanMove,
    handleUndo,
    setHoveredCell,
    isDraw,
    resetGame,
  } = useGomokuGame({ mode, onStatsChange, onBotResponseTime })

  const winnerLabel =
    winner === null
      ? ''
      : mode === 'local'
        ? winner === 1
          ? 'Black wins!'
          : 'White wins!'
        : winner === 2
          ? 'Bot wins!'
          : 'Player wins!'

  return (
    <div className="w-full">
      <div className="relative mx-auto w-full max-w-[min(88vmin,76vh,720px)] sm:max-w-[min(84vmin,72vh,680px)]">
        {mode === 'training' ? (
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
            board={board}
            currentPlayer={currentPlayer}
            hoveredCell={hoveredCell}
            isLocked={isLocked}
            winner={winner}
            onCellHover={setHoveredCell}
            onCellClick={(row, col) => void handleHumanMove(row, col)}
          />

          {(winner !== null || isDraw) && (
            <div className="absolute inset-0 z-20 flex items-center justify-center">
              <div className="mx-4 w-full max-w-3xl rounded-3xl border border-amber-900/50 bg-gradient-to-br from-black/70 via-black/60 to-black/50 p-6 text-center shadow-[0_30px_80px_rgba(0,0,0,0.6)]">
                <div className="mb-4 text-4xl font-extrabold tracking-tight text-amber-100 sm:text-6xl">
                  {winner !== null ? (
                    <span className="inline-block bg-clip-text text-transparent bg-[linear-gradient(90deg,#ffd37a,#ffb86b,#ffd37a)]">{winnerLabel}</span>
                  ) : (
                    <span className="inline-block bg-clip-text text-transparent bg-[linear-gradient(90deg,#c0c6d6,#9aa0b8,#c0c6d6)]">Draw — no legal moves remain</span>
                  )}
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
