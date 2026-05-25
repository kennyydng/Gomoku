# Gomoku

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

