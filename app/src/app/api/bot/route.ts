
import { NextResponse } from 'next/server'
import { execSync } from 'child_process';

function boardString(board) {
  return board.map(r => r.join("") + "\n").join("");
}

export async function POST(request: Request) {
  const { turn, captures, board } = (await request.json()) as {
    turn: number
    captures: { black: number, white: number }
    board: number[][]
  }

  if (!Array.isArray(board))
    throw "Invalid board!";

  const state = `${turn} ${captures.black}-${captures.white}\n${boardString(board)})`;
  
  console.log("make -C bot");
  const makeRes = execSync("make gomoku", {cwd:"bot"});

  try {
    const startTime = Date.now();
    const result = execSync("./bot/gomoku", {input: state, timeout: 2000});
    const time = Date.now() - startTime;
  } catch (e) {
    console.error(e);
    return NextResponse.json({
      error: true
    })
  }

  const [x,y] = [0,0]

  console.log(`Move: ${x},${y} | time: ${time}`);

  return NextResponse.json({
    row: y-1,
    col: x-1,
    time: time * 1000
  })
}
