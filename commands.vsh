# VIM: let b:vsh_lvl=0

docker compose up --build --watch gomoku
docker compose run --rm --build gomoku make -C bot
docker compose exec gomoku bash

npm --prefix app r --package-lock-only swipl-stdio
npm run dev

make -C bot
