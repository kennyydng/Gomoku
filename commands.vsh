# VIM: let b:vsh_lvl=2

docker image prune --all

#<
docker buildx create \
		   --name container \
		   --driver=docker-container \
		   --driver-opt default-load=true
#>
docker buildx use default
docker buildx use container

docker compose up --build --watch gomoku
docker ps -a

docker compose run --rm --build gomoku make -C bot
docker compose exec gomoku bash

npm --prefix app r --package-lock-only swipl-stdio
npm run dev

make -C bot

git status
git commit -m "First cpp bot"
git push
