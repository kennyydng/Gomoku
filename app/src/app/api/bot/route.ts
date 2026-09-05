
import { NextResponse } from 'next/server'
import { execSync } from 'child_process';
import type { Gomoku, Rules, Position } from '../../game/Gomoku'

function boardString(board) {
  return board.map(r => r.join("") + "\n").join("");
}

const rules_keys = [
  "pass",
  "capture",
  "captureUnperfect",
  "foulOverline",
  "overline",
  "threeThree",
  "fourFour",
  "flanking",
]

export async function POST(request: Request) {
  const { game: {rules, moves} } = (await request.json()) as { game: Gomoku }

  if (!Array.isArray(moves))
    throw "Invalid move list!"

  const rules_list = rules_keys.map(k => rules[k] === 'black' ? 'b' : rules[k] ? '1' : '0')
  const state = `${moves.map(([x,y]) => `|${x}:${y}`).join('')}`
  const exec = `bin/gomoku-${rules_list.join('')}`
  
  console.log("(Re)Compiling")
  const makeRes = execSync(`make ${exec}`, {cwd:"bot"})
  console.log(makeRes.toString())

  console.log("Asking bot for move...")
  const startTime = Date.now()

  const result = execSync(`./bot/${exec}`, {input: state, timeout: 500000}).toString()
  execSync(`[ ! -f gmon.out ] || gprof ./bot/${exec} > /var/logs/bot.profile`)

  const time = Date.now() - startTime

  const move = result == "/" ? null : result.split(":").map(Number) as Position

  console.log(`Move: ${move} | time: ${time / 1000}`)

  return NextResponse.json({move, time})
}
