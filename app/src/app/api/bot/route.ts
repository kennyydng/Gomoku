
import { NextResponse } from 'next/server'
import { execSync } from 'child_process';
import { existsSync, statSync } from 'fs';
import { join } from 'path';
import type { Gomoku, Rules, Position } from '../../game/Gomoku'

const BOT_CWD = 'bot'
const BOT_BINARY = 'gomoku'
const BOT_SOURCES = ['src/main.cpp', 'src/Gomoku.cpp', 'inc/Gomoku.class.hpp', 'inc/utils.hpp']

function botNeedsRebuild(): boolean {
  const binaryPath = join(BOT_CWD, BOT_BINARY)
  if (!existsSync(binaryPath))
    return true

  const binaryTime = statSync(binaryPath).mtimeMs
  return BOT_SOURCES.some((relativePath) => {
    const sourcePath = join(BOT_CWD, relativePath)
    return existsSync(sourcePath) && statSync(sourcePath).mtimeMs > binaryTime
  })
}

export async function POST(request: Request) {
  const { game: {rules, moves} } = (await request.json()) as { game: Gomoku }

  if (!Array.isArray(moves))
    throw "Invalid move list!";

  const gridToken = rules.grid === '15x15' ? '5' : '9'
  const orderedRuleKeys: Array<keyof Rules> = [
    'pass',
    'capture',
    'captureUnperfect',
    'foulOverline',
    'overline',
    'threeThree',
    'fourFour',
    'flanking',
  ]
  const rulesPayload = orderedRuleKeys
    .map((key) => {
      const value = rules[key]
      if (value === 'black')
        return 'b'
      return value ? '1' : '0'
    })
    .join('')

  const state = `${gridToken}${rulesPayload}\n${moves.map(([x,y]) => `|${x}:${y}`).join('')}`;

  //console.log(state)
  
  if (botNeedsRebuild()) {
    console.log("(Re)Compiling");
    execSync("g++ -std=c++23 -O2 -Wall -Wextra -Werror -pedantic -I inc src/main.cpp src/Gomoku.cpp -o gomoku", {cwd: BOT_CWD});
  }

  console.log("Asking bot for move...");
  const startTime = Date.now();
  const result = execSync("./gomoku", {cwd: BOT_CWD, input: state, timeout: 500000}).toString();
  const time = Date.now() - startTime;

  const moveRegex = /\|(\d+):(\d+)/g
  let best: RegExpExecArray | null = null
  let match: RegExpExecArray | null
  do {
    match = moveRegex.exec(result)
    if (match)
      best = match
  } while (match)

  if (!best) {
    return NextResponse.json({ move: null, time })
  }

  const x = Number(best[1])
  const y = Number(best[2])

  console.log(`Move: ${x},${y} | time: ${time / 1000}`);

  //execSync("gprof -b ./bot/gomoku > /var/profile/bot.profile");

  return NextResponse.json({
    move: [x, y] as Position,
    time,
  })
}
