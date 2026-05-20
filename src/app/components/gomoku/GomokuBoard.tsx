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
  } = useGomokuGame({ mode, onStatsChange, onBotResponseTime })

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

        <GomokuBoardSurface
          board={board}
          currentPlayer={currentPlayer}
          hoveredCell={hoveredCell}
          isLocked={isLocked}
          winner={winner}
          onCellHover={setHoveredCell}
          onCellClick={(row, col) => void handleHumanMove(row, col)}
        />
      </div>
    </div>
  )
}

export default memo(GomokuBoard)
