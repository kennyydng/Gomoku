#!/bin/sh
# Compilation du moteur, en un seul endroit.
#
# Le Dockerfile et l'API web (app/src/app/api/bot/route.ts) l'appellent tous
# les deux. Les options vivent dans le Makefile, que le sujet exige de toute
# façon : ce script n'est plus qu'un point d'entrée stable pour les appelants
# qui ne veulent pas connaître make.
exec make -C "$(dirname "$0")" "$@"
