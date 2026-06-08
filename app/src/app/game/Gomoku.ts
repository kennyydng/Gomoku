"use client"

import { BOARD_SIZE, BOARD_RANGE } from '../constants/game'

type Player = 0 | 1
type Stone = null | Player
type Position = [number,number]
type GameResult = null | 0 | 1 | 'draw'
type Threat = {
  type: 'C3' | 'O3' | 'C4' | '4+4' | 'O4' | '5'
  dir: Direction
  line: [Position,Position]
}
type Direction = [-1|0|1, -1|0|1]

type Board = Stone[][]
type Captures = [number, number]

type Rules = {
  pass: boolean
  capture: boolean
  //captureSelf: boolean
  catureUnperfect: boolean

  foulOverline: boolean
  overline: boolean | 'black'

  threeThree: boolean | 'black'
  fourFour: boolean | 'black'

  flanking: boolean | 'black'
}

const SUBDIRECTIONS: Array<Direction> = [
  [-1,-1], [ 0,-1], [ 1,-1],
  [-1, 0],          [ 1, 0],
  [-1, 1], [ 0, 1], [ 1, 1],
] as const
const DIRECTIONS: Array<Direction> = [
  [ 1, 0], [ 0, 1], 
  [ 1, 1], [ 1,-1],
] as const

function plus(pos: Position, delta: Direction, times = 1): Position {
  return [
    pos[0] + delta[0] * times,
    pos[1] + delta[1] * times
  ]
}

function minus(pos: Position, delta: Direction, times = 1): Position {
  return [
    pos[0] - delta[0] * times,
    pos[1] - delta[1] * times
  ]
}

function reverse(dir: Direction): Position {
  return [-dir[0], -dir[1]]
}

function opponentOf(player: Player): Player {
  return player ^ 1
}

export class Gomoku {
  rules: Rules
  board: Board
  score: Captures
  player: Player
  moves: Array<Position>
  result: GameResult
  threats: Array<Threat>
  delayedWin: boolean

  constructor(copy: Gomoku);
  constructor(rules: Rules);
  constructor(init: Rules | Gomoku) {
    if (init instanceof Gomoku) {
      this.rules = init.rules
      this.board = init.board.map((row) => [...row])
      this.score = [...init.score]
      this.player = init.player
      this.moves = [...init.moves]
      this.result = init.result
      this.threats = []
      this.delayedWin = false
    } else {
      this.rules = init
      this.board = Array.from({ length: BOARD_SIZE },
        () => Array.from({ length: BOARD_SIZE },
          () => null
        )
      )
      this.score = [0,0]
      this.player = 0
      this.moves = []
      this.result = null
      this.threats = []
      this.delayedWin = false
    }
  }

  *positions(): IterableIterator<Position> {
    for (let i = 0; i < BOARD_SIZE; i++)
      for (let j = 0; j < BOARD_SIZE; j++)
        yield [j,i]
  }

  positionID(pos: Position): number {
    return pos[1] * BOARD_SIZE + pos[0]
  }

  validPosition(pos: Position): boolean {
    return (
      pos[0] >= 0 && pos[0] <= BOARD_RANGE &&
      pos[1] >= 0 && pos[1] <= BOARD_RANGE
    )
  }

  stone(pos: Position): Stone {
    return this.board[pos[1]][pos[0]]
  }

  set(pos: Position, stone: Stone) {
    this.board[pos[1]][pos[0]] = stone
  }

  findCaptures(pos: Position) {
    const player = this.player
    const opponent = opponentOf(player)
    const captures: Array<[Position,Player]> = []

    for (const delta of SUBDIRECTIONS) {
      const pos1 = plus(pos,delta)
      const pos2 = plus(pos1,delta)
      const pos3 = plus(pos2,delta)

      if (
        this.validPosition(pos3) &&
        this.stone(pos1) === opponent &&
        this.stone(pos2) === opponent &&
        this.stone(pos3) === player
      ) captures.push([pos1,opponent], [pos2,opponent])
    }

    return captures
  }

  resolveMove(move: Position) {
    if (!this.validPosition(move) || this.stone(move) !== null)
      return null

    let captures = [];
    if (this.rules.capture)
      captures = this.findCaptures(move)

    const threats = this.getMoveThreats(move)
    const winning = threats.filter(({type}) => type === '5')

    if (!captures.length && !winning.length) {
      const overline = threats.filter(({type}) => type === 'overline')
      const four = threats.filter(({type}) => type === 'O4' || type === 'C4')
      const fourFour = threats.filter(({type}) => type === '4+4')
      const three = threats.filter(({type}) => type === 'O3')

      if (this.rules.foulOverline && overline.length)
        return null
      if (this.rules.fourFour && (four.length > 1 || fourFour.length))
         return null
      if (this.rules.threeThree && three.length > 1)
        return null
    }

    return {
      move,
      captures,
      threats //: threats.filter(({type}) => type !== 'overline')
    }
  }

  applyResolvedMove(
    {move, captures, threats}: ReturnType<resolveMove>
  ) {
    const player = this.player
    this.set(move,player)

    this.score[player] += captures.length
    for (let [cpos,_] of captures)
      this.set(cpos,null)

    this.result = this.updateTurn(move)

    this.moves.push(move)
    this.threats = threats
  }

  updateTurn(move: Position) {
    const player = this.player
    const opponent = opponentOf(player)
    const player5Lines = this.getThreats(move, player, 5
                                        ).filter(({type}) => type === '5')

    if (this.rules.capture) {
      if (this.score[opponent] >= 10 && this.score[player] >= 10)
        return 'draw'
      if (this.score[opponent] >= 10) // Can happen with self-captures
        return opponent

      if (this.delayedWin) {
        const opponent5Lines = this.getThreats(moves.at(-1), opponent, 5
                                              ).filter(({type}) => type === '5')

        if (opponent5Lines.length) {
          if (this.score[player] >= 10)
            return 'draw'
          else
            return opponent
        }
      }

      if (this.score[player] >= 10)
        return player

      if (player5Lines.length) {
        if (this.rules.captureUnperfect && player5Lines.some(({line}) => this.isUnperfect5(line,player)))
          this.delayedWin = true
        else
          return player
      }
    } else if (player5Lines.length) {
      return player
    }

    this.player = opponent
    if (this.hasAnyLegalMove())
      return null

    if (this.rules.pass) {
        this.player = player
        if (this.hasAnyLegalMove())
          return null
    }

    return 'draw'
  }

  getForbiddenCells() {
    const forbidden = new Set<number>()

    for (let pos of this.positions()) {
        if (this.stone(pos) === null && this.resolveMove(pos) === null)
          forbidden.add(this.positionID(pos))
    }

    return forbidden
  }

  hasAnyLegalMove() {
    for (let pos of this.positions())
      if (this.resolveMove(pos))
        return true
    return false
  }

  isUnperfect5(line: Array<Position>, player: Player) {
    const opponent = opponentOf(player)

    for (let pos of line) {
      for (let delta of SUBDIRECTIONS) {
        const flank0 = minus(pos,delta,1)
        const pos1 = plus(pos,delta,1)
        const flank1 = plus(pos,delta,2)

        if (!this.validPosition(flank0)) continue
        if (!this.validPosition(flank1)) continue
        if (this.stone(pos1) !== player) continue
        if ((this.stone(flank0) === opponent && this.stone(flank1) === null) ||
            (this.stone(flank1) === opponent && this.stone(flank0) === null))
          return true
      }
    }
    return false
  }

  getSection(pos: Position, dir: Direction): [number,Section] {
    const offset_x =  pos[0]*dir[0]
    const offset_y = (pos[1]*dir[1] + BOARD_SIZE) % BOARD_SIZE
    const offset =
      dir[0] === 0 ? offset_y :
      dir[1] === 0 ? offset_x :
      Math.min(offset_x,offset_y)

    let stones: Stone[] = []
    let start = minus(pos,dir,offset)
    let next = start
    while (this.validPosition(next)) {
      stones.push(this.stone(next))
      next = plus(next,dir)
    }

    return [offset, new Section(stones, start, dir)]
  }

  simulate_play(pos: Position) {
    this.set(pos,this.player)

    if (this.rules.capture) {
      const captured = this.findCaptures(pos)
      for (let [cpos,_] of captured)
        this.set(cpos,null)
      return captured
    }
    return []
  }

  simulate_unplay(pos: Position, captured: Position[]) {
    for (let capt of captured)
      this.set(...capt)
    this.set(pos,null)
  }

  *sectionThrough(pos: Position): Iterable<ReturnType<getSection>> {
    for (const dir of DIRECTIONS)
      yield this.getSection(pos, dir)
  }

  rulesFor(player: Player): Rules {
    return Object.fromEntries(
      Object.entries(this.rules).map(
        ([k,v]) => [k,v === 'black' ? player === 0 : v]
      )
    )
  }

  getMoveThreats(move: Position) {
    const captures = this.simulate_play(move)
    const threats = this.getThreats(move)
    this.simulate_unplay(move,captures)

    return threats
  }

  getThreats(pos: Position, min: number = 3) {
    const player = this.player
    const rules = this.rulesFor(player)
    const threats: Array<Threat> = []
    for (const [offset,section] of this.sectionThrough(pos)) {
      const secThreat = section.threatAt(offset,rules,player,min)
      if (secThreat.type) {
        if (secThreat.requires?.every((where) => this.resolveMove(section.from(where)) === null))
          continue

        const dir = section.dir
        const {type,line: [start,end]} = secThreat
        const startPos = section.from(start)
        const line = Array.from({length: end-start}, (_,i) => plus(startPos,dir,i))
        threats.push({type, dir, line})
      }
    }
    return threats
  }
}

class Section extends Array<Stone> {
  start: Position
  dir: Direction

  constructor(
    stones: Stone[],
    start: Position,
    dir: Direction
  ) {
    super(...stones)
    this.start = start
    this.dir = dir
  }

  from(where: number): Position {
    return plus(this.start, this.dir, where)
  }

  play(where: number, player: Player, capturesEnabled: boolean) {
    const opponent = opponentOf(player)
    this[where] = player

    if (capturesEnabled) {
      let captures: Stones[] = []
      if (where > 2 &&
          this[where-1] === opponent &&
          this[where-2] === opponent &&
          this[where-3] === player
      ) captures.push([where-1,opponent], [where-2,opponent])
      if (where < this.length-3 &&
          this[where+1] === opponent &&
          this[where+2] === opponent &&
          this[where+3] === player
      ) captures.push([where+1,opponent], [where+2,opponent])

      for (let [c,_] of captures)
        this[c] = null
      return captures
    }

    return []
  }

  unplay(where: number, captures: [number,Player][]) {
    for (let [c,player] of captures)
      this[c] = player
    this[where] = null
  }

  getContiguous(where: number, player: Player): [number,number] {
    let start = where + 1 
    while (start > 0 && this[start-1] === player)
      start -= 1

    let end = where 
    while (end < this.length && this[end] === player)
      end += 1

    return [start,end]
  }

  threatOf(move: number, rules: Rules, player: Player, min: number = 3) {
    if ( this.length < 5 || move < 0 || move >= this.length || this[move] !== null )
      return {}
    const captures = this.play(move, player, rules.capture)
    const ret = this.threatAt(move, rules, player, min)
    this.unplay(move, captures)
    return ret
  }


  threatAt(move: number, rules: Rules, player: Player, min: number = 3) {
    if ( this.length < 5 )
      return {}

    const opponent = opponentOf(player)
    const line = this.getContiguous(move, player)
    const len = line[1] - line[0]
    const flanked = [
      rules.flanking && line[0] > 0      && this[line[0]-1] === opponent,
      rules.flanking && line[1] < length && this[line[1]  ] === opponent,
    ]

    if (len > 5 && rules.overline)
      return {type: 'overline', line}
    else if (len >= 5) {
      if (len > 5 || !flanked[0] || !flanked[1])
        return {type: '5', line, flanked: len === 5 && (flanked[0] || flanked[1])}
    } else if (min < 5) {
      const plays = [line[0]-1,line[1]]
      const ext = plays.map((play) => this.threatOf(play, rules, player, min+1))

      const is5 = (i) => (ext[i].type === '5')
      const isFlanked = (i) => (ext[i].flanked)

      if (is5(0) || is5(1)) {
        const line4 = line.map((pos,i) => is5(i) ? ext[i].line[i] : pos)
        if (is5(0) && !isFlanked(0) && is5(1) && !isFlanked(1))
          return {type: len === 4 ? 'O4' : '4+4', line: line4}
        else
          return {type: 'C4', line}
      } else if (min < 4) {
        const isO4 = (i) => (ext[i].type === 'O4')
        if (isO4(0) || isO4(1)) {
          const line3 = line.map((pos,i) => isO4(i) ? ext[i].line[i] : pos)
          return {type: 'O3', line: line3, requires: plays.filter((_,i) => isO4(i))}
        }

        // Disabled because requires returning both O3 + C3 threats at once
        //const is4 = (i) => (ext[i].type === 'C4' || ext[i].type === '4+4')
        //if (is4(0) || is4(1)) {
        //  const line = line.map((pos,i) => is4(i) ? ext[i].line[i] : pos)
        //  return {type: 'C3', line, requires: plays.filter((_,i) => is4(i))}
        //}
      }
    }

    return {}
  }
}
