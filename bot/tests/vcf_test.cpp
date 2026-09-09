// VCF : la victoire différée se joue sur le coup du DÉFENSEUR.
//
// Sous captureUnperfect, un cinq dont une paire est prenable ne termine pas la
// partie quand l'attaquant le pose : play() le met en attente, et c'est le coup
// SUIVANT — celui du défenseur — qui le résout. Le défenseur qui répond autre
// chose qu'une capture perd donc sur son propre coup.
//
// Le piège, et la raison d'être de ce test : dans vcf::defends, une réponse qui
// mène à un état terminal n'est pas forcément une parade. Compter toute
// position terminale comme « le défenseur survit » faisait abandonner la
// séquence sur la première réponse perdante rencontrée, sans jamais évaluer la
// capture — la seule vraie parade, et celle qui pouvait être réfutée à son tour.

#include "vcf.hpp"

#include <cstdio>
#include <string>

static int checks = 0, failures = 0;

static void expect(const char *what, bool ok, const char *detail = "") {
	checks++;
	if (!ok) {
		failures++;
		std::printf("  ECHEC %-46s %s\n", what, detail);
	}
}

int main() {
	std::printf("VCF : victoire differee et parade par capture\n");

	// Noir tient 5:5..8:5 et 5:6 ; blanc est en 5:4, le reste au loin.
	// La paire verticale 5:5/5:6 est prenable en 5:7, blanc la flanquant
	// en 5:4. C'est la position de tools/regress.sh, un coup plus tôt.
	Rules const rules{std::string("911100")};
	EngineState start{rules};
	for (Pos m : {Pos{5,5}, Pos{5,4}, Pos{6,5}, Pos{18,0}, Pos{7,5},
	              Pos{18,3}, Pos{8,5}, Pos{18,6}, Pos{5,6}, Pos{18,9}})
		start = start.after(m);
	expect("noir au trait", start.player() == 0);

	// 1. Le cinq de noir est différé : la partie continue.
	EngineState five = start.after({9,5});
	expect("le cinq prenable ne termine pas la partie", !five.terminal());

	// 2. Blanc bloque au lieu de capturer : il perd sur son propre coup.
	EngineState blocked = five.after({4,5});
	expect("blanc bloque -> noir gagne", blocked.terminal() && blocked.winner() == 0);

	// 3. Blanc capture en 5:7 : il casse le cinq et survit.
	EngineState captured = five.after({5,7});
	expect("blanc capture -> la partie continue", !captured.terminal());

	// 4. Mais la capture ne fait que retarder : elle libère 5:5, et la
	//    rangée 6:5..9:5 se complète en 10:5 sur un cinq cette fois parfait.
	EngineState finished = captured.after({10,5});
	expect("noir conclut en 10:5", finished.terminal() && finished.winner() == 0);

	// 5. Donc le VCF doit voir la séquence entière depuis `start`. Il ne la
	//    voit que s'il refuse de compter le blocage perdant de blanc comme
	//    une parade — c'est exactement ce que ce test verrouille : sans ce
	//    refus, vcf::find ne rend RIEN sur cette position.
	//
	//    Les deux extrémités concluent, 9:5 comme 4:5 : le cinq est différé
	//    dans les deux cas, la capture en 5:7 est forcée dans les deux cas, et
	//    la case qu'elle libère se rejoue. On accepte donc l'une ou l'autre.
	vcf::Budget b;
	auto forced = vcf::find(start, b);
	char detail[64] = "aucun coup trouve";
	if (forced)
		std::snprintf(detail, sizeof detail, "trouve %d:%d", (int)forced->x, (int)forced->y);
	expect("le VCF trouve la victoire forcee",
		forced && (*forced == Pos{9,5} || *forced == Pos{4,5}), detail);

	// 6. La séquence par 4:5 conclut elle aussi : blanc doit capturer, et la
	//    case libérée se rejoue en cinq parfait.
	EngineState alt = start.after({4,5}).after({5,7}).after({5,5});
	expect("l'autre extremite conclut aussi", alt.terminal() && alt.winner() == 0);

	std::printf("%d verifications, %d echec(s)\n", checks, failures);
	return failures != 0;
}
