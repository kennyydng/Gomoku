# Gomoku

## Le bot (IA)

Le bot vit entièrement dans `bot/` et est un moteur C++ **autonome**, indépendant
du moteur de règles du frontend (`app/src/app/game/Gomoku.ts`, qui ne sert qu'à
l'affichage/l'aide côté humain). L'API `app/src/app/api/bot/route.ts` compile
le binaire `bot/Gomoku` puis lui envoie l'état de la partie sur `stdin` :

```
<règles sur 6 caractères>\n
|x0:y0|x1:y1|x2:y2...
```

et lit le coup choisi sur `stdout` (`|x:y`).

### Représentation — `bot/inc/BitBoard.class.hpp`

Le plateau est un **bitboard** : un entier par joueur, un bit par case. 361
cases tiennent dans sept mots de 64 bits, soit 64 octets. Il en existe quatre
dispositions, une par axe (horizontale, verticale, deux diagonales) : chacune
range les mêmes cases dans un ordre différent, de sorte que « avancer d'une
case le long de l'axe » soit toujours le même décalage de bits.

Le moteur ne cherche jamais les alignements. Il compte, pour chacune des
**1 020 fenêtres de cinq cases** du plateau, combien de pierres chaque joueur y
a (`CountBoard`). Poser une pierre fait basculer d'un niveau les fenêtres qui
la contiennent — une poignée d'opérations bit à bit, indépendamment de la
longueur des alignements en jeu. Les motifs cassés type `XX.XX` tombent
gratuitement : c'est une fenêtre à quatre pierres, comme `XXXX`.

Le **score** en découle et se maintient par différence, jamais recalculé.

### Règles — `bot/inc/Gomoku.class.hpp` + `bot/src/Gomoku.cpp`

Les règles sont lues à l'exécution sur la première ligne de `stdin`, pas
figées à la compilation : l'interface les laisse configurer.

- **Capture** : motif `moi, adversaire, adversaire, moi` dans les huit
  directions. Retirer une pierre est l'inverse exact de la poser ; une erreur
  y serait silencieuse, d'où l'oracle de `tests/capture_test.cpp` qui
  reconstruit le plateau de zéro et exige le même score au bit près.
- **Capture de fin de partie** : un cinq dont une pierre est prenable ne gagne
  pas tout de suite — il gagne s'il *survit* au coup adverse. C'est la seule
  règle dont l'issue dépend de l'historique et non de la position, d'où une
  issue mémorisée par `play()` et un bit dédié dans le hachage.
- **Légalité** (double-trois, double-quatre, overline interdit) : détection de
  menaces scalaire, appliquée **à la racine seulement**. La refaire à chaque
  nœud coûterait une récursion par candidat pour rien — seul le coup
  réellement rendu doit être légal.
- **Taille de plateau** : 15x15 ou 19x19. Le stockage reste dimensionné pour
  19x19, mais les cases hors limites ne sont jamais candidates et les fenêtres
  qui débordent sortent du comptage.

### Recherche — `bot/inc/search.hpp` + `bot/inc/vcf.hpp`

La recherche est écrite contre un **concept C++**, pas contre `Gomoku` : elle
se compile et se teste en C++23 standard contre un état factice
(`tests/search_test.cpp`), sur n'importe quelle machine, alors que le moteur
exige un compilateur récent.

- **Négamax + alpha-bêta + PVS**, *iterative deepening* borné dans le temps
  (460 ms, plafond 800 ms jusqu'au 10e pli pour garantir la profondeur exigée).
- **Table de transposition** de 2^21 entrées, hachage de Zobrist incluant le
  trait et l'éventuel cinq en attente.
- **Génération de coups** restreinte aux cases voisines d'une pierre, triées
  par la variation de score qu'elles produisent — calculée *sans* jouer le
  coup ni copier l'état — et coupée aux **six** meilleures par nœud.
- **VCF** (victoire par fours consécutifs) avant la recherche principale : il
  n'explore que les coups forçants, donc descend bien plus profond pour une
  fraction du coût. Sa règle de sûreté : l'ensemble des réponses adverses doit
  être un *sur*-ensemble des défenses réelles, jamais un sous-ensemble.

Profondeur atteinte : **11 à 12 plis**, pour 424 ms de moyenne par coup et
aucun coup au-dessus de 500 ms sur une partie complète.

### Compilation — `bot/build.sh`

Le moteur exige la **réflexion statique C++26** (`template for`, splicers) et
AVX2, donc **GCC 16 au minimum**. Ce n'est pas un compilateur exotique pour
autant : le GCC livré par une distribution récente suffit — vérifié avec celui
du dépôt Fedora 44 (16.1.1), `make` en 3 s sans un avertissement sous
`-Wall -Wextra -Werror -pedantic`. Un GCC plus ancien, lui, ne le compilera pas.

Les options vivent dans `bot/Makefile`, que le sujet exige de toute façon.
`bot/build.sh` n'est plus qu'un point d'entrée qui délègue à `make`, pour les
appelants qui ne veulent pas le connaître : le Dockerfile *et* `route.ts`.
Une seule ligne de compilation dans le projet, donc rien qui puisse diverger.

Les objets séparés et `-MMD -MP` font que toucher un en-tête recompile
exactement ce qui en dépend. `route.ts` appelle `build.sh` à chaque requête
sans se demander si c'est utile : `make` répond « Nothing to be done » en
quelques millisecondes quand rien n'a bougé.

### Mesure — `bot/tools/`

Les nœuds par seconde ne disent rien sur la force de jeu. `selfplay.py` fait
jouer deux binaires l'un contre l'autre sur des ouvertures **appariées à
couleurs inversées**, et `pairs.py` en extrait ce qui départage vraiment : une
paire où la même couleur gagne deux fois rapporte un point à chacun et ne
prouve rien. `regress.sh` fige les positions dont on connaît la bonne réponse,
`turns.py` mesure le critère « victoire en moins de 20 tours ».

## Bonus (au-delà du mandatory)

- **Liste de règles à toggle individuellement** (`app/src/app/page.tsx`) :
  chaque règle du sujet (capture, capture d'une ligne de 5, overline à 4
  états, double-trois, double-quatre) est activable/désactivable
  indépendamment, plutôt qu'un seul jeu de règles fixe.
- **Presets de variantes connues** : 42 Mandatory, Classic, Renju,
  Ninuki-renju, Pente — le bonus "starting conditions" suggéré par le sujet,
  étendu à des jeux de règles complets plutôt qu'aux seules variantes
  Standard/Pro/Swap.
- **Taille de plateau au choix** : 15x15 ou 19x19, plutôt que fixée à 19x19.
- **Undo** (`GomokuBoard.tsx`) : revenir en arrière pour explorer des
  variantes, en mode local et training.
- **Menu d'aide** (`HelpModal.tsx`) : explication des règles en cours de
  partie, directement dans l'UI.

## Lancer le projet

Depuis la racine du dépôt. Les commandes sont les mêmes partout — `docker` et
`podman` sont interchangeables, la CLI Docker n'étant qu'un alias de podman sur
les distributions qui l'installent ainsi.

### En conteneur

    docker compose up --build -d    # http://localhost:3000
    docker compose logs -f
    docker compose down

Les sources sont montées dans le conteneur : **il n'y a rien à relancer après
une modification**. `app/` est rechargé à chaud par Next, et `bot/` est
recompilé à la requête suivante, `route.ts` appelant `make` à chaque appel.
Seul un changement de `Dockerfile`, de `package.json` ou du `Makefile` demande
un `--build`.

### Sans conteneur

Il faut **GCC 16** (réflexion C++26) et Node. Pas de compilateur exotique : le
GCC d'une distribution récente suffit.

    make                            # le moteur, ~3 s
    cd app && npm ci && npm run dev # l'interface, http://localhost:3000

### Le moteur seul

Le plus court chemin pour voir le bot réfléchir. Il lit les règles puis
l'historique sur `stdin`, rend le coup sur `stdout` et son raisonnement sur
`stderr` — profondeur, nœuds, verdict du VCF.

    make
    printf '911100\n|9:9\n|10:10\n' | ./bot/Gomoku

    cd bot
    sh tools/regress.sh ./Gomoku    # positions dont on connaît la bonne réponse
    sh tools/robust.sh  ./Gomoku    # entrées malformées : refuser, jamais planter

> **Sur un Mac Apple Silicon**, tout fonctionne mais par émulation : le moteur
> y atteint 8 à 9 plis en 849 ms, contre 12 plis et 320 ms en natif. Vérifiez-y
> que l'application marche, jamais qu'elle est rapide.
