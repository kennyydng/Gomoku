import { NextResponse } from 'next/server'
import { BOARD_SIZE } from '../../constants/game'

function findFirstEmptyCell(board: number[][]) {
  for (let row = 0; row < BOARD_SIZE; row += 1) {
    for (let col = 0; col < BOARD_SIZE; col += 1) {
      if (board[row]?.[col] === 0) {
        return { row, col }
      }
    }
  }

  return null
}

export async function POST(request: Request) {
  const body = (await request.json()) as {
    turn?: number
    captures?: {
      black?: number
      white?: number
    }
    board?: number[][]
  }
  const board = Array.isArray(body.board) ? body.board : []

  await new Promise((resolve) => setTimeout(resolve, 100))

  const move = findFirstEmptyCell(board)

  return NextResponse.json({
    row: move?.row ?? -1,
    col: move?.col ?? -1,
  })
}