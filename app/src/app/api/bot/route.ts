
import { NextResponse } from 'next/server'
import { execSync } from 'child_process';
import type { Gomoku, Rules, Position } from '../../game/Gomoku'

function boardString(board) {
  return board.map(r => r.join("") + "\n").join("");
}

export async function POST(request: Request) {
  const { game: {rules, moves} } = (await request.json()) as { game: Gomoku }

  if (!Array.isArray(moves))
    throw "Invalid move list!";

  const rules_array = Object.entries(rules).map(([_,v]) => v === 'black' ? 'b' : v ? '0' : '1')
  const state = `${rules_array.join('')}\n${moves.map(([x,y]) => `|${x}:${y}`).join('')}`;

  //console.log(state)
  
  console.log("(Re)Compiling");
  const makeRes = execSync("make gomoku", {cwd:"bot"});

  console.log("Asking bot for move...");
  const startTime = Date.now();
  const result = execSync("./bot/gomoku", {input: state, timeout: 500000});
  const time = Date.now() - startTime;

  const [x,y] = [0,0]

  console.log(`Move: ${x},${y} | time: ${time / 1000}`);

  //execSync("gprof -b ./bot/gomoku > /var/profile/bot.profile");

  return NextResponse.json({
    row: y-1,
    col: x-1,
    time: time
  })
}
