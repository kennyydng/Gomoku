
import { NextResponse } from 'next/server'
import { execSync, spawnSync } from 'child_process';
import { existsSync } from 'fs';
import type { Gomoku, Rules, Position } from '../../game/Gomoku'

// Deux dispositions coexistent, et le chemin doit valoir pour les deux.
// L'image copie bot/ DANS le dossier de l'app (/var/www/app/bot) ; le dépôt,
// lui, garde app/ et bot/ côte à côte. Un `npm run dev` lancé depuis app/
// cherchait donc app/bot, qui n'existe pas — le moteur ne démarrait pas, et
// l'API rendait une erreur de spawn au lieu d'un coup.
const BOT_CWD = existsSync('bot') ? 'bot' : '../bot'

// La commande de compilation vit dans bot/build.sh, qui délègue au Makefile —
// le Dockerfile passe par le même script. Deux copies des drapeaux (C++26,
// réflexion, AVX2) divergeraient, et la divergence ne se verrait qu'au moment
// où l'image ne compile plus.
//
// On l'appelle à chaque requête sans se demander si c'est utile : make répond
// « Nothing to be done » en quelques millisecondes quand rien n'a bougé. La
// version précédente listait les en-têtes à surveiller à la main, en en
// oubliant six (sets.hpp, terms.hpp, vec.hpp, sugar.hpp, macros.hpp,
// parsing_utils.hpp) — les modifier ne déclenchait aucune recompilation, et le
// bot répondait avec du code périmé. make lit les vraies dépendances (-MMD).
const BOT_BUILD = 'sh build.sh'

export async function POST(request: Request) {
  const { game: {rules, moves} } = (await request.json()) as { game: Gomoku }

  if (!Array.isArray(moves))
    throw new Error("Invalid move list!")

  const gridToken = rules.grid === '15x15' ? '5' : '9'
  const overlineToken: Record<Rules['overline'], string> = {
    win: '1',
    legal: '0',
    forbidden: 'f',
    forbiddenBlack: 'b',
  }
  const rulesPayload =
    (rules.capture ? '1' : '0') +
    (rules.captureUnperfect ? '1' : '0') +
    overlineToken[rules.overline] +
    (['threeThree', 'fourFour'] as const)
      .map((key) => {
        const value = rules[key]
        if (value === 'black')
          return 'b'
        return value ? '1' : '0'
      })
      .join('')

  const state = `${gridToken}${rulesPayload}\n${moves.map(([x,y]) => `|${x}:${y}`).join('')}`;

  console.log("(Re)building bot...");
  execSync(BOT_BUILD, {cwd: BOT_CWD, stdio: 'inherit'})

  console.log("Asking bot for move...");
  const startTime = Date.now();
  // spawnSync et non execSync : le moteur écrit son raisonnement sur stderr
  // (profondeur atteinte, nœuds visités, verdict du VCF), et execSync ne rend
  // que stdout — ces chiffres finissaient dans les logs du conteneur, hors de
  // portée de l'interface. Le sujet les réclame explicitement : « some sort of
  // debugging process that lets you examine the reasoning process of your AI
  // while it's running… it would help during your defense sessions ».
  const run = spawnSync("./Gomoku", {
    cwd: BOT_CWD, input: state, timeout: 500000, encoding: 'utf8',
  });
  const time = Date.now() - startTime;

  execSync(`[ ! -f gmon.out ] || gprof ./Gomoku > /var/logs/bot.profile`, {cwd: BOT_CWD})

  const result = run.stdout ?? ''
  const diagnostics = run.stderr ?? ''

  // Le moteur rend 1 sur une entrée qu'il refuse, et ne plante plus (voir
  // bot/tools/robust.sh). Un statut non nul est donc un vrai refus, pas un
  // crash : on le remonte au lieu de le confondre avec « aucun coup ».
  if (run.error || run.status !== 0) {
    console.error(`Bot failed (status ${run.status}): ${diagnostics.trim()}`)
    return NextResponse.json({ move: null, time, error: 'engine refused the position' })
  }

  const numberFrom = (re: RegExp) => {
    const m = re.exec(diagnostics)
    return m ? Number(m[1]) : null
  }
  const depth = numberFrom(/Depth reached: (\d+)/)
  const nodes = numberFrom(/Nodes: (\d+)/)
  const vcf = /VCF: victoire forcee/.test(diagnostics)

  const moveRegex = /\|(\d+):(\d+)/g
  let best: RegExpExecArray | null = null
  let match: RegExpExecArray | null
  do {
    match = moveRegex.exec(result)
    if (match)
      best = match
  } while (match)

  if (!best) {
    return NextResponse.json({ move: null, time, depth, nodes, vcf })
  }

  const x = Number(best[1])
  const y = Number(best[2])

  console.log(`Move: ${x + 1}:${y + 1} | Total time (spawn + search + IO): ${time}ms`);

  return NextResponse.json({
    move: [x, y] as Position,
    time,
    depth,
    nodes,
    vcf,
  })
}
