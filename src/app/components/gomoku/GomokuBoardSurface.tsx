"use client"

import { memo, useMemo } from 'react'
import { BOARD_RANGE, BOARD_SIZE, type Player } from './useGomokuGame'

const BOARD_LINE_COLOR = 'rgba(39, 25, 14, 0.72)'
const BOARD_MARK_COLOR = 'rgba(67, 46, 23, 0.9)'

function getStoneClass(player: Player) {
  return player === 1
    ? 'bg-[radial-gradient(circle_at_30%_30%,_#7c7c7c_0%,_#1e1e1e_52%,_#050505_100%)] shadow-[inset_0_2px_3px_rgba(255,255,255,0.16),0_12px_18px_rgba(0,0,0,0.45)]'
    : 'bg-[radial-gradient(circle_at_30%_30%,_#fffdf8_0%,_#ddd3c2_52%,_#a89475_100%)] shadow-[inset_0_2px_3px_rgba(255,255,255,0.4),0_12px_18px_rgba(0,0,0,0.3)]'
}

function getPreviewClass(player: Player) {
  return player === 1 ? 'bg-black/25' : 'bg-white/35'
}

interface GomokuBoardSurfaceProps {
  board: number[][]
  currentPlayer: Player
  hoveredCell: { row: number; col: number } | null
  isLocked: boolean
  winner: Player | null
  onCellHover: (cell: { row: number; col: number } | null) => void
  onCellClick: (row: number, col: number) => void
}

function GomokuBoardSurface({ board, currentPlayer, hoveredCell, isLocked, winner, onCellHover, onCellClick }: GomokuBoardSurfaceProps) {
  const boardLineOverlay = useMemo(() => {
    const lines = []

    for (let index = 0; index < BOARD_SIZE; index += 1) {
      const percent = (index / BOARD_RANGE) * 100

      lines.push(
        <line key={`v-${index}`} x1={`${percent}%`} y1="0%" x2={`${percent}%`} y2="100%" stroke={BOARD_LINE_COLOR} strokeWidth="1.2" />,
        <line key={`h-${index}`} x1="0%" y1={`${percent}%`} x2="100%" y2={`${percent}%`} stroke={BOARD_LINE_COLOR} strokeWidth="1.2" />,
      )
    }

    return lines
  }, [])

  const starPoints = useMemo(() => [3, 9, 15], [])

  return (
    <div className="relative aspect-square rounded-[1.8rem] border border-amber-800/45 bg-[linear-gradient(135deg,_#c79b63_0%,_#b8834a_44%,_#8e5b30_100%)] p-[clamp(10px,1.6vw,18px)] shadow-[inset_0_1px_0_rgba(255,255,255,0.18),0_24px_80px_rgba(0,0,0,0.35)]">
      <div className="relative h-full w-full rounded-[1.1rem] bg-[radial-gradient(circle_at_top,_rgba(255,245,220,0.18),_transparent_32%),linear-gradient(180deg,_rgba(255,235,197,0.12),_rgba(255,235,197,0.02))]">
        <svg aria-hidden="true" className="absolute inset-0 h-full w-full">
          <rect x="0" y="0" width="100%" height="100%" rx="16" fill="rgba(255, 235, 197, 0.08)" />
          {boardLineOverlay}
          {starPoints.map((index) => {
            const position = (index / BOARD_RANGE) * 100

            return [
              <circle key={`s-${index}-a`} cx={`${position}%`} cy={`${position}%`} r="1.6%" fill={BOARD_MARK_COLOR} opacity="0.95" />,
              <circle key={`s-${index}-b`} cx={`${position}%`} cy={`${100 - position}%`} r="1.6%" fill={BOARD_MARK_COLOR} opacity="0.95" />,
            ]
          })}
        </svg>

        <div className="absolute inset-0">
          {board.map((rowValues, row) =>
            rowValues.map((cell, col) => {
              const left = `${(col / BOARD_RANGE) * 100}%`
              const top = `${(row / BOARD_RANGE) * 100}%`
              const stoneSize = `clamp(16px, 2.3vmin, 28px)`
              const isHovered = hoveredCell?.row === row && hoveredCell?.col === col
              const canPreview = cell === 0 && !isLocked && winner === null

              return (
                <button
                  key={`${row}-${col}`}
                  type="button"
                  aria-label={`Intersection ${row + 1}, ${col + 1}`}
                  disabled={cell !== 0 || isLocked || winner !== null}
                  onPointerEnter={() => onCellHover({ row, col })}
                  onPointerLeave={() => {
                    if (hoveredCell?.row === row && hoveredCell?.col === col) {
                      onCellHover(null)
                    }
                  }}
                  onFocus={() => onCellHover({ row, col })}
                  onBlur={() => {
                    if (hoveredCell?.row === row && hoveredCell?.col === col) {
                      onCellHover(null)
                    }
                  }}
                  onClick={() => onCellClick(row, col)}
                  className="absolute -translate-x-1/2 -translate-y-1/2 rounded-full outline-none transition-transform disabled:cursor-not-allowed"
                  style={{
                    left,
                    top,
                    width: 'min(1.9vw, 20px)',
                    height: 'min(1.9vw, 20px)',
                  }}
                >
                  {cell !== 0 ? (
                    <span
                      className={`absolute inset-0 block rounded-full ${getStoneClass(cell as Player)}`}
                      style={{ width: stoneSize, height: stoneSize, left: '50%', top: '50%', transform: 'translate(-50%, -50%)' }}
                    />
                  ) : canPreview && isHovered ? (
                    <span
                      className={`absolute inset-0 block rounded-full opacity-75 blur-[0.2px] ${getPreviewClass(currentPlayer)}`}
                      style={{ width: stoneSize, height: stoneSize, left: '50%', top: '50%', transform: 'translate(-50%, -50%)' }}
                    />
                  ) : null}
                </button>
              )
            }),
          )}
        </div>
      </div>
    </div>
  )
}

export default memo(GomokuBoardSurface)
