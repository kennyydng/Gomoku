#!/bin/sh
# Compilation du moteur, en un seul endroit.
#
# Le Dockerfile et l'API web (app/src/app/api/bot/route.ts) l'appellent tous
# les deux : deux copies de cette ligne de commande finiraient par diverger,
# et la divergence ne se verrait qu'au moment où l'image ne compile plus.
#
# La réflexion statique C++26 (`template for`, splicers) et AVX2 sont exigées
# par la représentation bitboard — d'où l'image Arch avec GCC 16 du projet.
# Un GCC de distribution plus ancien ne compilera pas ce moteur.
exec g++ -std=c++26 -freflection -fcontract-evaluation-semantic=ignore \
	-O2 -mavx2 -Wall -Wextra -Werror -pedantic -I inc \
	src/main.cpp src/Gomoku.cpp -o Gomoku
