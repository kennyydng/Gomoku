
import { NextResponse } from 'next/server'
import swipl from 'swipl-stdio'

const plEngine = new swipl.Engine();
await plEngine.call("working_directory(_,'/var/www/app/bot/')")
await plEngine.call("consult('bot.pl')")
await plEngine.call("test");
//await plEngine.call("test_bot");

function plBoard(board) {
  return ".(" + board.map(
    r => ".(" + r.map(
      c => c == 0 ? "-" : c.toString()
    ).join(",") + ")"
  ).join(",") + ")";
}

export async function POST(request: Request) {
  const { turn, captures, board } = (await request.json()) as {
    turn: number
    captures: { black: number, white: number }
    board: number[][]
  }

  if (!Array.isArray(board))
    throw "Invalid board!";
  const plState = `gomoku(${turn},${captures.black}-${captures.white},${plBoard(board)})`;
  await plEngine.call(`make`);
  const plResult = await plEngine.call(`bot_move(${plState},Move,Time)`);
  const [x,y] = plResult.Move.args;
  const time = plResult.Time;

  console.log(`Move: ${x},${y} | time: ${time}`);

  return NextResponse.json({
    row: x-1,
    col: y-1,
    time: time * 1000
  })
}
