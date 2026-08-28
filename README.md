# Gomoku

## Le bot (IA)

Le bot vit entièrement dans `bot/` et est un moteur C++ **autonome**, indépendant
du moteur de règles du frontend (`app/src/app/game/Gomoku.ts`, qui ne sert qu'à
l'affichage/l'aide côté humain). L'API `app/src/app/api/bot/route.ts` compile
le binaire `bot/gomoku` puis lui envoie l'état de la partie sur `stdin` :

```
<règles sur 6 caractères>\n
|x0:y0|x1:y1|x2:y2...
```

et lit le coup choisi sur `stdout` (`|x:y`).

### Moteur de règles — `bot/inc/Gomoku.class.hpp` + `bot/src/Gomoku.cpp`

Port fidèle des règles déjà validées côté frontend, appliqué directement sur
le plateau (bitboards), sans rejouer sur un tableau local :

- **Menaces** (`threatAt`/`getThreats`) : détecte pour une pierre donnée, dans
  les 4 directions, les alignements three-ouvert, four (simple/ouvert/4+4),
  five et overline, par une marche directe sur le plateau + une récursion
  courte (2 niveaux) qui simule de vraies extensions via `rawPlace`/`rawRemove`.
- **Légalité** (`isLegalMove`) : rejette un coup qui introduirait un
  double-trois (ou un 4+4 / un overline interdit selon les règles choisies).
- **Fin de partie** (`applyMove`) : victoire par capture (10 paires), victoire
  par alignement, et la règle "endgame capture" — un five capturable
  (`isUnperfect5`) ne gagne pas immédiatement ; l'adversaire a un coup pour le
  casser par capture, sinon le five est validé au tour suivant.
- **Hachage de Zobrist** : maintenu de façon incrémentale dans `play()`/`undo()`,
  utilisé par la table de transposition du moteur de recherche.

### Moteur de recherche — `bot/src/main.cpp`

- **Négamax + élagage alpha-bêta**, avec **iterative deepening** borné dans le
  temps (~460ms, sous la limite de 500ms imposée par le sujet) : on augmente la
  profondeur tant qu'il reste du temps, et on garde le meilleur coup de la
  dernière profondeur entièrement terminée.
- **Heuristique** (`evaluate`) : scanne les alignements du plateau une seule
  fois (à partir du début de chaque série) et les note selon leur longueur et
  le nombre d'extrémités ouvertes (five/four ouvert/four/three ouvert/etc.),
  plus un bonus quadratique sur les captures (se rapprocher de 10 devient de
  plus en plus important).
- **Génération de coups** : restreinte aux cases vides à proximité d'une pierre
  existante (dans une zone qui grandit avec la partie, jamais un scan complet
  du plateau), triée par un score cheap (motif créé pour soi + motif bloqué
  chez l'adversaire) et limitée aux ~8 meilleurs coups par nœud.
- **Compromis de performance assumé** : la vérification complète de légalité
  (double-trois) est coûteuse (récursive) et n'est faite qu'à la racine — le
  coup réellement joué est donc toujours 100% légal. En profondeur, la
  recherche explore les coups géométriquement plausibles sans redériver cette
  vérification à chaque nœud ; seule la détection de victoire/défaite
  (toujours exacte) est utilisée pour couper l'arbre.
- **Table de transposition** (hachage de Zobrist) : évite de ré-explorer une
  position déjà rencontrée par un autre ordre de coups.

Avec tout ça, le bot atteint une profondeur de recherche de **8 à 10** selon la
densité du plateau, dans le budget de temps imposé.

### Point de performance important

`route.ts` ne recompile `bot/gomoku` que si les sources ont changé (comparaison
de timestamps) — recompiler du C++ à chaque coup coûtait bien plus cher que la
recherche elle-même, et faussait le temps de réponse réel perçu par le joueur.

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

## Commandes Docker

Ces commandes s'exécutent depuis la racine du projet. Elles utilisent `docker compose` (ou `podman compose` / `podman-compose` selon votre installation).

- Build et lancer en premier plan :

	docker compose up --build

- Build et lancer en arrière-plan (détaché) :

	docker compose up --build -d

- Rebuild des images sans cache :

	docker compose build --no-cache

- Arrêter et supprimer les conteneurs, réseaux et volumes créés :

	docker compose down

- Arrêter les conteneurs (sans suppression) :

	docker compose stop

- Voir et suivre les logs :

	docker compose logs -f

- Afficher l'état des services :

	docker compose ps

- Recréer / rebuild d'un seul service :

	docker compose up -d --no-deps --build <nom_du_service>

- Supprimer les images locales créées par Compose :

	docker compose down --rmi local

Remarques :

- Si vous utilisez Podman, remplacez `docker compose` par `podman compose` ou utilisez `podman-compose` selon votre distribution.
- Certaines CLI proposent une option `--watch` pour recharger automatiquement les services lors de modifications de fichiers; si votre CLI ne la supporte pas, utilisez un outil de rechargement adapté au service (ex. `nodemon`, watchers intégrés).

Examples rapides :

```bash
docker compose up --build
# ou en détaché
docker compose up --build -d
# stopper et nettoyer
docker compose down
```

