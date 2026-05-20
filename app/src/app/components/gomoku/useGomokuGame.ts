"use client"

import { useEffect, useMemo, useState } from 'react'

export type GameMode = 'local' | 'ai' | 'training'
export type Player = 1 | 2

type GameSnapshot = {
  board: number[][]
  currentPlayer: Player
  capturesBlack: number
  capturesWhite: number
  winner: Player | null
}

export interface GameStats {
  capturesBlack: number
  capturesWhite: number
  currentPlayer: Player
  isLocked: boolean
  winner: Player | null
  isDraw: boolean
}

interface UseGomokuGameArgs {
  mode: GameMode
  onStatsChange?: (stats: GameStats) => void
  onBotResponseTime?: (ms: number | null) => void
}

export const BOARD_SIZE = 19
export const BOARD_RANGE = BOARD_SIZE - 1

const DIRECTIONS = [
  [0, 1],
  [1, 0],
  [1, 1],
  [1, -1],
] as const

const DOUBLE_THREE_PATTERNS = ['0011100', '0101100', '0011010', '0110100', '0100110', '0110010', '0101010']

function createEmptyBoard() {
  return Array.from({ length: BOARD_SIZE }, () => Array.from({ length: BOARD_SIZE }, () => 0))
}

function cloneBoard(board: number[][]) {
  return board.map((row) => [...row])
}

function isInsideBoard(row: number, col: number) {
  return row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE
}

function getOpponent(player: Player) {
  return player === 1 ? 2 : 1
}

function getLineCells(board: number[][], row: number, col: number, deltaRow: number, deltaCol: number, player: Player) {
  const cells: Array<[number, number]> = [[row, col]]

  let nextRow = row + deltaRow
  let nextCol = col + deltaCol

  while (isInsideBoard(nextRow, nextCol) && board[nextRow][nextCol] === player) {
    cells.push([nextRow, nextCol])
    nextRow += deltaRow
    nextCol += deltaCol
  }

  nextRow = row - deltaRow
  nextCol = col - deltaCol

  while (isInsideBoard(nextRow, nextCol) && board[nextRow][nextCol] === player) {
    cells.unshift([nextRow, nextCol])
    nextRow -= deltaRow
    nextCol -= deltaCol
  }

  return cells
}

function countCaptures(board: number[][], row: number, col: number, player: Player) {
  const opponent = getOpponent(player)
  const capturedCells: Array<[number, number]> = []

  for (const [deltaRow, deltaCol] of DIRECTIONS) {
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

function canBreakLineByCapture(board: number[][], lineCells: Array<[number, number]>, opponent: Player) {
  const lineKeySet = new Set(lineCells.map(([lineRow, lineCol]) => `${lineRow}:${lineCol}`))

  for (let row = 0; row < BOARD_SIZE; row += 1) {
    for (let col = 0; col < BOARD_SIZE; col += 1) {
      if (board[row][col] !== 0) {
        continue
      }

      const nextBoard = cloneBoard(board)
      nextBoard[row][col] = opponent
      const capturedCells = countCaptures(nextBoard, row, col, opponent)

      if (capturedCells.some(([capturedRow, capturedCol]) => lineKeySet.has(`${capturedRow}:${capturedCol}`))) {
        return true
      }
    }
  }

  return false
}

function hasAlignmentWin(board: number[][], row: number, col: number, player: Player) {
  const opponent = getOpponent(player)

  for (const [deltaRow, deltaCol] of DIRECTIONS) {
    const lineCells = getLineCells(board, row, col, deltaRow, deltaCol, player)

    if (lineCells.length < 5) {
      continue
    }

    if (!canBreakLineByCapture(board, lineCells, opponent)) {
      return true
    }
  }

  return false
}

function getDirectionPattern(board: number[][], row: number, col: number, deltaRow: number, deltaCol: number, player: Player) {
  const pattern: string[] = []

  for (let offset = -4; offset <= 4; offset += 1) {
    const currentRow = row + deltaRow * offset
    const currentCol = col + deltaCol * offset

    if (!isInsideBoard(currentRow, currentCol)) {
      pattern.push('2')
      continue
    }

    if (board[currentRow][currentCol] === 0) {
      pattern.push('0')
      continue
    }

    pattern.push(board[currentRow][currentCol] === player ? '1' : '2')
  }

  return pattern.join('')
}

function createsDoubleThree(board: number[][], row: number, col: number, player: Player) {
  let openThreeCount = 0

  for (const [deltaRow, deltaCol] of DIRECTIONS) {
    const pattern = getDirectionPattern(board, row, col, deltaRow, deltaCol, player)

    if (DOUBLE_THREE_PATTERNS.some((candidate) => pattern.includes(candidate))) {
      openThreeCount += 1
    }
  }

  return openThreeCount >= 2
}

function resolveMove(board: number[][], row: number, col: number, player: Player) {
  if (board[row][col] !== 0) {
    return null
  }

  const nextBoard = cloneBoard(board)
  nextBoard[row][col] = player

  const capturedCells = countCaptures(nextBoard, row, col, player)
  const capturedPairs = Math.floor(capturedCells.length / 2)

  if (capturedPairs === 0 && createsDoubleThree(nextBoard, row, col, player)) {
    return null
  }

  for (const [capturedRow, capturedCol] of capturedCells) {
    nextBoard[capturedRow][capturedCol] = 0
  }

  return {
    board: nextBoard,
    capturedPairs,
  }
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

export function useGomokuGame({ mode, onStatsChange, onBotResponseTime }: UseGomokuGameArgs) {
  const [board, setBoard] = useState<number[][]>(() => createEmptyBoard())
  const [currentPlayer, setCurrentPlayer] = useState<Player>(1)
  const [capturesBlack, setCapturesBlack] = useState(0)
  const [capturesWhite, setCapturesWhite] = useState(0)
  const [hoveredCell, setHoveredCell] = useState<{ row: number; col: number } | null>(null)
  const [isLocked, setIsLocked] = useState(false)
  const [winner, setWinner] = useState<Player | null>(null)
  const [isDraw, setIsDraw] = useState(false)
  const [history, setHistory] = useState<GameSnapshot[]>([])

  const stats = useMemo<GameStats>(
    () => ({
      capturesBlack,
      capturesWhite,
      currentPlayer,
      isLocked,
      winner,
      isDraw,
    }),
    [capturesBlack, capturesWhite, currentPlayer, isLocked, winner],
  )

  useEffect(() => {
    onStatsChange?.(stats)
  }, [onStatsChange, stats])

  useEffect(() => {
    setHoveredCell(null)
    setBoard(createEmptyBoard())
    setCurrentPlayer(1)
    setCapturesBlack(0)
    setCapturesWhite(0)
    setIsLocked(false)
    setWinner(null)
    setIsDraw(false)
    setHistory([])
    onBotResponseTime?.(null)
  }, [mode])

  const captureSnapshot = (nextBoard: number[][], nextCurrentPlayer: Player, nextCapturesBlack: number, nextCapturesWhite: number, nextWinner: Player | null): GameSnapshot => ({
    board: cloneBoard(nextBoard),
    currentPlayer: nextCurrentPlayer,
    capturesBlack: nextCapturesBlack,
    capturesWhite: nextCapturesWhite,
    winner: nextWinner,
  })

  const restoreSnapshot = (snapshot: GameSnapshot) => {
    setBoard(snapshot.board)
    setCurrentPlayer(snapshot.currentPlayer)
    setCapturesBlack(snapshot.capturesBlack)
    setCapturesWhite(snapshot.capturesWhite)
    setWinner(snapshot.winner)
    setIsDraw(false)
    setHoveredCell(null)
    setIsLocked(false)
    onBotResponseTime?.(null)
  }

  function hasAnyLegalMove(boardToCheck: number[][]) {
    for (let r = 0; r < BOARD_SIZE; r += 1) {
      for (let c = 0; c < BOARD_SIZE; c += 1) {
        if (boardToCheck[r][c] !== 0) continue
        if (resolveMove(boardToCheck, r, c, 1) !== null) return true
        if (resolveMove(boardToCheck, r, c, 2) !== null) return true
      }
    }
    return false
  }

  const applyResolvedMove = (
    move: { board: number[][]; capturedPairs: number },
    player: Player,
    row: number,
    col: number,
    baseCapturesBlack = capturesBlack,
    baseCapturesWhite = capturesWhite,
  ) => {
    const nextCapturesBlack = baseCapturesBlack + (player === 1 ? move.capturedPairs * 2 : 0)
    const nextCapturesWhite = baseCapturesWhite + (player === 2 ? move.capturedPairs * 2 : 0)
    const nextWinner = hasAlignmentWin(move.board, row, col, player) ? player : boardHasWinner(nextCapturesBlack, nextCapturesWhite)

    setBoard(move.board)
    setCapturesBlack(nextCapturesBlack)
    setCapturesWhite(nextCapturesWhite)
    setWinner(nextWinner)
    // detect draw: no legal moves remain for either player
    const draw = !hasAnyLegalMove(move.board)
    setIsDraw(draw)

    return {
      winner: nextWinner,
      capturesBlack: nextCapturesBlack,
      capturesWhite: nextCapturesWhite,
    }
  }

  const handleUndo = () => {
    if (mode !== 'training' || isLocked || history.length === 0) {
      return
    }

    const snapshot = history[history.length - 1]

    if (!snapshot) {
      return
    }

    // One snapshot is enough because it always represents the state before the human turn.
    // In training, that means undoing after the bot answered rewinds both moves at once.
    restoreSnapshot(snapshot)
    setHistory((previousHistory) => previousHistory.slice(0, -1))
  }

  const resetGame = () => {
    setBoard(createEmptyBoard())
    setCurrentPlayer(1)
    setCapturesBlack(0)
    setCapturesWhite(0)
    setIsLocked(false)
    setWinner(null)
    setIsDraw(false)
    setHistory([])
    onBotResponseTime?.(null)
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

    // The stack stores the state before the turn starts.
    // That makes undo consistent for local, AI, and training flows.
    setHistory((previousHistory) => [...previousHistory, captureSnapshot(board, currentPlayer, capturesBlack, capturesWhite, winner)])

    const humanOutcome = applyResolvedMove(humanMove, humanPlayer, row, col)

    if (humanOutcome.winner !== null) {
      return
    }

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

      const aiOutcome = applyResolvedMove(aiMove, aiPlayer, data.row, data.col, humanOutcome.capturesBlack, humanOutcome.capturesWhite)

      if (aiOutcome.winner !== null) {
        return
      }

      setCurrentPlayer(humanPlayer)
    } catch {
      onBotResponseTime?.(500)
    } finally {
      setIsLocked(false)
    }
  }

  return {
    board,
    capturesBlack,
    capturesWhite,
    currentPlayer,
    handleHumanMove,
    handleUndo,
    hoveredCell,
    historyLength: history.length,
    isLocked,
    setHoveredCell,
    winner,
    isDraw,
    resetGame,
  }
}
