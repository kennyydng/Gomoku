"use client"

import { useEffect, useMemo, useState } from 'react'
import { BOARD_SIZE, BOARD_RANGE } from '../../constants/game'
import { Gomoku } from '../../game/Gomoku'
export type {
  Rules,
  Player,
  Position
} from '../../game/Gomoku'

export type GameMode = 'local' | 'ai' | 'training'

interface UseGomokuGameArgs {
  mode: GameMode
  rules: Rules
  onUpdate?: (game: Gomoku) => void
  onBotResponseTime?: (ms: "pending" | number | null) => void
}

export function useGomokuGame({
  mode,
  rules,
  onUpdate,
  onBotResponseTime
}: UseGomokuGameArgs) {
  const [game, setGameState] = useState<Gomoku>(() => new Gomoku(rules))
  useEffect(() => {
    onUpdate?.(game)
  }, [onUpdate,game])

  const [hoveredCell, setHoveredCell] = useState<Position | null>(null)
  const [hintCell, setHintCell] = useState<Position | null>([])
  const [history, setHistory] = useState<Gomoku[]>([])

  const resetGame = () => {
    setGameState(new Gomoku(rules))
    setHistory([])
    onBotResponseTime?.(null)
  }

  const aiPlayer: Player = 1

  const handleUndo = () => {
    if (history.length === 0)
      return

    const snapshot = history.at(-1)

    if (!snapshot)
      return

    setGameState(new Gomoku(snapshot))
    setHistory(history.slice(0,-1))
    onBotResponseTime?.(null)
  }

  const isHumanMove = mode === 'local' || game.player == aiPlayer;
  const isLocked = !isHumanMove || game.result !== null;
  const shouldSuggestMove = mode === 'local' || mode === 'training'

  const handleHumanMove = (pos: Position) => {
    if (isLocked) return;

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
      const TIMEOUT = 10000;
      const timeoutId = window.setTimeout(() => controller.abort(), TIMEOUT)

      const response = await fetch('/api/bot', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({game}),
        signal: controller.signal,
      })

      window.clearTimeout(timeoutId)

      if (!response.ok)
        throw new Error('Bot response failed')

      const data: { move?: Position, time?: number } = await response.json()
      onBotResponseTime?.(data.time)
      return data.move
    } catch (e) {
      onBotResponseTime?.(null)
      if (!e instanceof DOMException)
        console.error(e);
    }
  }

  const handleAIMove = async (controller) => {
    const pos = await fetchBotMove(controller)
    if (pos) return

    const move = game.resolveMove(data.move)
    if (move) return

    const next = new Gomoku(game)
    next.applyResolvedMove(move)
    setGameState(next)
  }

  const handleAIHint = async (controller) => {
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


  return {
    game,
    resetGame,
    isLocked,
    historyLength: history.length,
    handleUndo,
    setHoveredCell,
    hoveredCell,
    hintCell,
    handleHumanMove
  }
}
