#!/bin/sh
# Positions dont on connaît la bonne réponse, vérifiées sur le binaire.
#
# Le moteur n'a pas de tests unitaires : ses bugs les plus coûteux ne plantent
# pas, ils font simplement jouer un mauvais coup. Ce script fixe les cas déjà
# rencontrés pour qu'ils ne reviennent pas sans qu'on s'en aperçoive.
#
#     ./tools/regress.sh ./Gomoku

BIN=${1:-./Gomoku}
fails=0

check() { # check <libellé> <règles> <historique> <coups acceptés>
	got=$(printf "$2\n$3" | "$BIN" 2>/dev/null | sed 's/.*|//')
	case " $4 " in
		*" $got "*) printf '  ok    %s\n' "$1" ;;
		*) printf '  ECHEC %s\n        attendu {%s}, obtenu %s\n' "$1" "$4" "$got"
		   fails=$((fails+1)) ;;
	esac
}

echo "Régressions connues"

# Cinq imparfait : noir a (5,5)-(9,5), mais (5,5) et (5,6) forment une paire
# prenable en (5,7), flanquée par le blanc en (5,4). La victoire est DIFFÉRÉE :
# blanc survit s'il capture, et seulement s'il capture. Bloquer une extrémité
# d'un cinq déjà complet ne sert à rien.
#
# Le bug attrapé ici : rootSearch et negamax notaient tout coup suivi d'une
# victoire comme gagnant POUR CELUI QUI VIENT DE JOUER, sans regarder
# outcome.winner. Blanc jouait donc le coup qui perd en croyant gagner, et
# s'arrêtait à la profondeur 1 en 6 nœuds — persuadé d'avoir trouvé un mat.
check "cinq imparfait : il faut capturer, pas bloquer" 911100 \
	'|5:5\n|5:4\n|6:5\n|18:0\n|7:5\n|18:3\n|8:5\n|18:6\n|5:6\n|18:9\n|9:5\n' \
	"5:7"

# Un quatre adverse ouvert des deux côtés se pare d'un côté ou de l'autre.
check "parer un quatre" 910100 \
	'|5:5\n|0:0\n|6:5\n|18:0\n|7:5\n|0:18\n|8:5\n' \
	"4:5 9:5"

# Un quatre à soi se complète immédiatement.
check "conclure un quatre" 910100 \
	'|5:5\n|0:0\n|6:5\n|18:0\n|7:5\n|0:18\n|8:5\n|18:18\n' \
	"4:5 9:5"

# La paire offerte se prend : le motif est moi, adverse, adverse, moi.
check "prendre la paire offerte" 910100 \
	'|5:5\n|6:5\n|15:15\n|7:5\n' \
	"8:5"

echo "$fails echec(s)"
[ "$fails" -eq 0 ]
