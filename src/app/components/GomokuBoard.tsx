"use client"

import { memo, useEffect, useMemo, useState } from 'react'

export type GameMode = 'local' | 'ai'
type Player = 1 | 2

export interface GameStats {
  capturesBlack: number
  capturesWhite: number
  currentPlayer: Player
  isLocked: boolean
  winner: Player | null
}

interface GomokuBoardProps {
  mode: GameMode
  onStatsChange?: (stats: GameStats) => void
  onBotResponseTime?: (ms: number) => void
}

const BOARD_SIZE = 19
const BOARD_RANGE = BOARD_SIZE - 1
const BOARD_LINE_COLOR = 'rgba(39, 25, 14, 0.72)'
const BOARD_MARK_COLOR = 'rgba(67, 46, 23, 0.9)'

function createEmptyBoard() {
  return Array.from({ length: BOARD_SIZE }, () => Array.from({ length: BOARD_SIZE }, () => 0))
}

function cloneBoard(board: number[][]) {
  return board.map((row) => [...row])
}

function getStoneClass(player: Player) {
  return player === 1
    ? 'bg-[radial-gradient(circle_at_30%_30%,_#7c7c7c_0%,_#1e1e1e_52%,_#050505_100%)] shadow-[inset_0_2px_3px_rgba(255,255,255,0.16),0_12px_18px_rgba(0,0,0,0.45)]'
    : 'bg-[radial-gradient(circle_at_30%_30%,_#fffdf8_0%,_#ddd3c2_52%,_#a89475_100%)] shadow-[inset_0_2px_3px_rgba(255,255,255,0.4),0_12px_18px_rgba(0,0,0,0.3)]'
}

function getPreviewClass(player: Player) {
  return player === 1 ? 'bg-black/25' : 'bg-white/35'
}

function resolveMove(board: number[][], row: number, col: number, player: Player) {
  if (board[row][col] !== 0) {
    return null
  }

  const nextBoard = cloneBoard(board)
  nextBoard[row][col] = player

  const capturedCells = countCaptures(nextBoard, row, col, player)

  for (const [capturedRow, capturedCol] of capturedCells) {
    nextBoard[capturedRow][capturedCol] = 0
  }

  return {
    board: nextBoard,
    capturedPairs: Math.floor(capturedCells.length / 2),
  }
}

function countCaptures(board: number[][], row: number, col: number, player: Player) {
  const opponent: Player = player === 1 ? 2 : 1
  const directions = [
    [0, 1],
    [1, 0],
    [1, 1],
    [1, -1],
  ] as const
  const capturedCells: Array<[number, number]> = []

  for (const [deltaRow, deltaCol] of directions) {
    const forwardRow1 = row + deltaRow
    const forwardCol1 = col + deltaCol
    const forwardRow2 = row + deltaRow * 2
    const forwardCol2 = col + deltaCol * 2
    const forwardRow3 = row + deltaRow * 3
    const forwardCol3 = col + deltaCol * 3

    if (
      forwardRow3 >= 0 &&
      forwardRow3 < BOARD_SIZE &&
      forwardCol3 >= 0 &&
      forwardCol3 < BOARD_SIZE &&
      board[forwardRow1][forwardCol1] === opponent &&
      board[forwardRow2][forwardCol2] === opponent &&
      board[forwardRow3][forwardCol3] === player
    ) {
      capturedCells.push([forwardRow1, forwardCol1], [forwardRow2, forwardCol2])
    }

    const backwardRow1 = row - deltaRow
    const backwardCol1 = col - deltaCol
    const backwardRow2 = row - deltaRow * 2
    const backwardCol2 = col - deltaCol * 2
    const backwardRow3 = row - deltaRow * 3
    const backwardCol3 = col - deltaCol * 3

    if (
      backwardRow3 >= 0 &&
      backwardRow3 < BOARD_SIZE &&
      backwardCol3 >= 0 &&
      backwardCol3 < BOARD_SIZE &&
      board[backwardRow1][backwardCol1] === opponent &&
      board[backwardRow2][backwardCol2] === opponent &&
      board[backwardRow3][backwardCol3] === player
    ) {
      capturedCells.push([backwardRow1, backwardCol1], [backwardRow2, backwardCol2])
    }
  }

  return capturedCells
}

function boardHasWinner(capturesBlack: number, capturesWhite: number) {
  if (capturesBlack >= 10) {
    return 1 as const
  }

  if (capturesWhite >= 10) {
    return 2 as const
  }

  return null
}

function GomokuBoard({ mode, onStatsChange, onBotResponseTime }: GomokuBoardProps) {
  const [board, setBoard] = useState<number[][]>(() => createEmptyBoard())
  const [currentPlayer, setCurrentPlayer] = useState<Player>(1)
  const [capturesBlack, setCapturesBlack] = useState(0)
  const [capturesWhite, setCapturesWhite] = useState(0)
  const [hoveredCell, setHoveredCell] = useState<{ row: number; col: number } | null>(null)
  const [isLocked, setIsLocked] = useState(false)
  const [winner, setWinner] = useState<Player | null>(null)

  const stats = useMemo<GameStats>(
    () => ({
      capturesBlack,
      capturesWhite,
      currentPlayer,
      isLocked,
      winner,
    }),
    [capturesBlack, capturesWhite, currentPlayer, isLocked, winner],
  )

  useEffect(() => {
    onStatsChange?.(stats)
  }, [onStatsChange, stats])

  useEffect(() => {
    if (capturesBlack >= 10) {
      setWinner(1)
    } else if (capturesWhite >= 10) {
      setWinner(2)
    }
  }, [capturesBlack, capturesWhite])

  useEffect(() => {
    setHoveredCell(null)
  }, [mode])

  const applyResolvedMove = (move: { board: number[][]; capturedPairs: number }, player: Player) => {
    setBoard(move.board)

    if (move.capturedPairs > 0) {
      if (player === 1) {
        setCapturesBlack((value) => value + move.capturedPairs * 2)
      } else {
        setCapturesWhite((value) => value + move.capturedPairs * 2)
      }
    }
  }

  const handleHumanMove = async (row: number, col: number) => {
    if (isLocked || winner !== null || board[row][col] !== 0) {
      return
    }

    const humanPlayer = currentPlayer
    const aiPlayer: Player = humanPlayer === 1 ? 2 : 1

    const humanMove = resolveMove(board, row, col, humanPlayer)

    if (!humanMove) {
      return
    }

    applyResolvedMove(humanMove, humanPlayer)

    if (mode === 'local') {
      setCurrentPlayer(aiPlayer)
      return
    }

    setIsLocked(true)

    try {
      const startedAt = performance.now()
      const controller = new AbortController()
      const timeoutId = window.setTimeout(() => controller.abort(), 500)

      const response = await fetch('/api/bot', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({ board: humanMove.board }),
        signal: controller.signal,
      })

      window.clearTimeout(timeoutId)
      onBotResponseTime?.(Math.round(performance.now() - startedAt))

      if (!response.ok) {
        throw new Error('Bot response failed')
      }

      const data: { row?: number; col?: number } = await response.json()

      if (
        typeof data.row !== 'number' ||
        typeof data.col !== 'number' ||
        data.row < 0 ||
        data.row >= BOARD_SIZE ||
        data.col < 0 ||
        data.col >= BOARD_SIZE ||
        humanMove.board[data.row][data.col] !== 0
      ) {
        return
      }

      const aiMove = resolveMove(humanMove.board, data.row, data.col, aiPlayer)

      if (!aiMove) {
        return
      }

      applyResolvedMove(aiMove, aiPlayer)
      setCurrentPlayer(humanPlayer)
    } catch {
      onBotResponseTime?.(500)
      // Mock AI fallback: if the API fails, simply unlock the board.
    } finally {
      setIsLocked(false)
    }
  }

  const previewPlayer = currentPlayer

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
    <div className="w-full">
      <div className="relative mx-auto w-full max-w-[min(88vmin,76vh,720px)] sm:max-w-[min(84vmin,72vh,680px)]">
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
                      onPointerEnter={() => setHoveredCell({ row, col })}
                      onPointerLeave={() => {
                        if (hoveredCell?.row === row && hoveredCell?.col === col) {
                          setHoveredCell(null)
                        }
                      }}
                      onFocus={() => setHoveredCell({ row, col })}
                      onBlur={() => {
                        if (hoveredCell?.row === row && hoveredCell?.col === col) {
                          setHoveredCell(null)
                        }
                      }}
                      onClick={() => void handleHumanMove(row, col)}
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
                          className={`absolute inset-0 block rounded-full opacity-75 blur-[0.2px] ${getPreviewClass(previewPlayer)}`}
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
      </div>
    </div>
  )
}

export default memo(GomokuBoard)