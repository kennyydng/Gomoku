#!/bin/sh
# Entrées malformées : le binaire doit refuser, jamais planter.
#
# Le sujet ne laisse pas de marge — un programme qui plante en quelque
# circonstance que ce soit vaut zéro. Or stdin est une frontière : l'API web
# construit des entrées propres, mais le correcteur lance le binaire à la main.
#
# Ce que ces cas ont attrapé : main() n'avait aucun try/catch alors que Rules
# et les opérateurs de lecture lèvent, et ne bornait pas les coordonnées avant
# de jouer — `|99:99` et `|-3:-3` écrivaient hors des bitboards.
#
#     ./tools/robust.sh ./Gomoku

BIN=${1:-./Gomoku}
fails=0

check() { # check <libellé> <entrée>
	printf '%b' "$2" | timeout 10 "$BIN" >/dev/null 2>&1
	code=$?
	# 0 = coup rendu, 1 = entrée refusée proprement. Tout le reste est un
	# plant : 134 = abort (exception non rattrapée), 139 = segfault.
	case $code in
		0|1) printf '  ok    %s\n' "$1" ;;
		124) printf '  ECHEC %s : TIMEOUT\n' "$1"; fails=$((fails+1)) ;;
		*)   printf '  ECHEC %s : plantage (code %d)\n' "$1" "$code"
		     fails=$((fails+1)) ;;
	esac
}

echo "Robustesse des entrees"

check "entree vide"                 ""
check "regles seules"               "911100\n"
check "regles invalides"            "zzzzzz\n|5:5\n"
check "regles trop courtes"         "91\n|5:5\n"
check "coup hors plateau"           "911100\n|99:99\n"
check "coup negatif"                "911100\n|-3:-3\n"
check "coordonnees non numeriques"  "911100\n|a:b\n"
check "ligne sans separateur"       "911100\n5:5\n"
check "case deja occupee"           "911100\n|5:5\n|5:5\n"
check "octets aleatoires"           "\x7f\x45\x4c\x02\x01\x00\xde\xad\xbe\xef\n"
check "plateau entierement rempli"  "911100\n$(for y in $(seq 0 18); do for x in $(seq 0 18); do printf '|%d:%d\\n' $x $y; done; done)"

echo "$fails echec(s)"
[ "$fails" -eq 0 ]
