"use client"

import { memo, useMemo } from 'react'
import type { CSSProperties } from 'react'
import { BOARD_THEME, getPreviewClass, getStoneClass } from '../../constants/game'
import { Gomoku } from '../../game/Gomoku'
import type { Player, Position } from '../../game/Gomoku'

interface GomokuBoardSurfaceProps {
  game: Gomoku
  isLocked: boolean
  hintCell: Position | null
  hoveredCell: Position | null
  onCellHover: (cell: Position | null) => void
  onCellClick: (cell: Position) => void
}

const EDGE = 1
const MIN = EDGE
const MAX = 100 - EDGE
const SIZE = MAX-MIN

function makeGrid(boardSize: number, boardRange: number) {
  const cellSize = SIZE / boardSize
  const grid = []
  const min_l = MIN + cellSize / 2
  const max_l = MAX - cellSize / 2
  const size_l = SIZE - cellSize

  for (let index = 0; index < boardSize; index += 1) {
    const percent = min_l + (index / boardRange) * size_l

    grid.push(
      <line key={`v-${index}`} x1={`${percent}%`} x2={`${percent}%`} y1={`${min_l}%`} y2={`${max_l}%`} stroke={BOARD_THEME.lineColor} strokeWidth="1.2" />, 
      <line key={`h-${index}`} y1={`${percent}%`} y2={`${percent}%`} x1={`${min_l}%`} x2={`${max_l}%`} stroke={BOARD_THEME.lineColor} strokeWidth="1.2" />,
    )
  }

  const starPoints = boardSize === 15 ? [3, 7, 11] : [3, 9, 15]

  for (let x of starPoints) {
    for (let y of starPoints) {
      const percent_x = min_l + (x / boardRange) * size_l
      const percent_y = min_l + (y / boardRange) * size_l

      grid.push(<circle key={`s-${x}-${y}`} cx={`${percent_x}%`} cy={`${percent_y}%`} r="0.65%" fill={BOARD_THEME.markColor} opacity="0.95" />)
    }
  }

  return grid
}

function GomokuBoardSurface({ game, isLocked, hintCell, hoveredCell, onCellHover, onCellClick }: GomokuBoardSurfaceProps) {
  const shape: CSSProperties = {
    borderRadius: '50%',
    position: 'absolute',
    height: '80%', top : '10%',
    width : '80%', left: '10%',
  }

  const grid = useMemo(() => makeGrid(game.boardSize, game.boardRange), [game.boardRange, game.boardSize])

  const forbidden = useMemo(() => game.getForbiddenCells(), [game])
  const cells = useMemo(() => {
    return Array.from({ length: game.boardSize }, (_,y) => (
      (<tr key={`row-${y}`}>
        {Array.from({ length: game.boardSize }, (_,x) => {
          const pos: Position = [x,y]
          const id = game.positionID(pos)
          const stone = game.stone(pos)
          const hovered = hoveredCell && hoveredCell[0] === x && hoveredCell[1] === y
          const hinted = hintCell && hintCell[0] === x && hintCell[1] === y
          const canPlay = !isLocked && stone === null

          let display = null;
          if (stone !== null)
            display = <div style={shape} className={getStoneClass(stone as Player)} />
          else if (forbidden.has(id))
            display = <div style={{...shape, opacity: 0.4, background: BOARD_THEME.forbiddenStone}} />
          else if (hovered)
            display = <div style={shape} className={getPreviewClass(game.player)} />

          return (
            <td
              key={`cell-${id}`}
              aria-label={`Intersection ${pos[0]+1}, ${pos[1]+1}`}
              style={{position: "relative"}}
            >
              {display} 
              <div
                style={{
                  ...shape,
                  border: hinted ? "1px solid yellow" : "none",
                  cursor: canPlay ? "pointer" : "no-drop",
                }}
                onClick={() => {
                  if (!canPlay)
                    return
                  onCellClick(pos)
                }}
                onPointerEnter={() => onCellHover(pos)}
                onPointerLeave={() => {if (hovered) onCellHover(null)}}
                onFocus={() => onCellHover(pos)}
                onBlur={() => {if (hovered) onCellHover(null)}}
              />
            </td>
          )
        })}
      </tr>)
    ))
  }, [forbidden, game.boardSize, hintCell, hoveredCell, isLocked, onCellClick, onCellHover])

  return (
    <div className={`relative aspect-square rounded-[1.8rem] border border-amber-800/45 ${BOARD_THEME.outerGradient} p-[18px]`}>
      <div className={`relative h-full w-full rounded-[1.1rem] ${BOARD_THEME.innerGradient}`}>
        <svg className="absolute inset-0 h-full w-full" aria-hidden="true">
          <rect x="0" y="0" width="100%" height="100%" rx="16" fill={BOARD_THEME.surfaceFill} />
          {grid}
        </svg>
        <table className={`absolute inset-0`}
          style={{
            width : `${SIZE}%`, left: `${MIN}%`,
            height: `${SIZE}%`, top : `${MIN}%`,
          }}
        >
          <tbody> {cells} </tbody>
        </table>
      </div>
    </div>
  )
}

export default memo(GomokuBoardSurface)
