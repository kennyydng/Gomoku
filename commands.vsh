# VIM: let b:vsh_lvl=0

docker compose up --build --watch gomoku
docker compose exec gomoku bash

npm --prefix app i --package-lock-only swipl-stdio
