// Vérifie l'adaptateur entre le moteur bitboard et la recherche.
//
// À lancer là où la branche compile (réflexion C++26) — le conteneur Arch du
// projet convient :
//
//     g++ -std=c++26 -freflection -O2 -mavx2 -I inc \
//         tests/engine_test.cpp src/Gomoku.cpp -o /tmp/engine_test && /tmp/engine_test
//
// Ce test couvre les conventions dont dépend tout le reste et dont une erreur
// ne provoquerait AUCUN plantage — juste un bot qui joue mal, bien plus
// difficile à diagnostiquer. Il a déjà attrapé une inversion de signe dans le
// tri des candidats, qui faisait rater au bot un cinq immédiat.

#include "engine_state.hpp"

#include <cstdio>

static int checks = 0, failures = 0;

static void expect(const char *what, bool ok, const char *detail = "") {
	checks++;
	if (!ok) {
		failures++;
		std::printf("  ECHEC %-44s %s\n", what, detail);
	}
}

// Plateau construit pierre par pierre pour les DEUX joueurs. Le joueur 1
// reçoit des coups de remplissage loin de l'action quand il en manque : sans
// eux, la parité des tours se décalerait et les pierres changeraient de
// propriétaire.
static Gomoku board(std::initializer_list<Pos> mine,
                    std::initializer_list<Pos> theirs) {
	static const Pos far[] = {
		{18,0},{18,3},{18,6},{18,9},{18,12},{18,15},{0,18},{3,18},{6,18}
	};
	std::vector<Pos> a(mine), b(theirs);
	Gomoku g{Rules{}};
	std::size_t f = 0;
	for (std::size_t i = 0; i < std::max(a.size(), b.size()); i++) {
		if (i < a.size()) g.play(a[i]);
		else              g.pass();
		if (i < b.size()) g.play(b[i]);
		else              g.play(far[f++]);
	}
	return g;
}

static EngineState play(std::initializer_list<Pos> moves) {
	EngineState s;
	for (Pos p : moves)
		s = s.after(p);
	return s;
}

int main() {
	std::printf("Adaptateur moteur : conventions de signe, victoire, hachage\n");

	// 1. SIGNE DE L'ÉVALUATION.
	//    evaluate() doit rendre le point de vue du joueur AU TRAIT. Après que
	//    le joueur 0 a posé quelques pierres alignées sans opposition, c'est
	//    au joueur 1 de jouer : la position doit donc lui être DÉFAVORABLE.
	//    Une inversion ici ferait jouer au bot les pires coups possibles.
	{
		EngineState s = play({ {9,9}, {0,0}, {9,10}, {0,1}, {9,11} });
		// joueur 0 a trois pierres alignées, joueur 1 trois pierres isolées ;
		// c'est au joueur 1 de jouer.
		expect("evaluate() est du point de vue du trait",
			s.player() == 1 && s.evaluate() < 0,
			s.player() == 1 ? "score positif alors que la position est mauvaise"
			                : "mauvais joueur au trait");
	}

	// 2. DÉTECTION DU VAINQUEUR.
	//    is_over() dit qu'un cinq existe, winner() doit dire de qui. Se
	//    tromper de joueur inverserait les scores de mat : le bot foncerait
	//    vers sa propre défaite en croyant gagner.
	{
		EngineState s = play({
			{5,5}, {0,0}, {6,5}, {0,1}, {7,5}, {0,2}, {8,5}, {0,3}, {9,5}
		});
		expect("partie detectee comme finie", s.terminal(), "cinq non detecte");
		expect("vainqueur = joueur 0", s.winner() == 0, "mauvais vainqueur");
	}
	{
		EngineState s = play({ {9,9}, {0,0}, {9,10} });
		expect("pas de vainqueur premature", s.winner() == -1 && !s.terminal());
	}

	// 3. COHÉRENCE DU HACHAGE.
	//    Deux ordres de coups menant à la même position doivent donner le même
	//    hachage, sinon la table de transposition ne mutualise rien (et perd
	//    son intérêt sans qu'aucun test ne le signale).
	{
		EngineState a = play({ {5,5}, {7,7}, {6,6}, {8,8} });
		EngineState b = play({ {6,6}, {8,8}, {5,5}, {7,7} });
		expect("hachage insensible a l'ordre des coups", a.hash() == b.hash());
	}
	{
		//    Et deux positions différentes doivent différer. Un hachage
		//    constant passerait le test précédent tout en cassant la table.
		EngineState a = play({ {5,5}, {7,7} });
		EngineState b = play({ {5,5}, {8,8} });
		expect("hachages differents pour positions differentes",
			a.hash() != b.hash());
	}
	{
		//    Le trait fait partie de la position.
		EngineState a = play({ {5,5} });
		EngineState b = play({ {5,5}, {7,7}, {8,8} });   // meme parite ? non
		(void)b;
		EngineState empty;
		expect("plateau vide != apres un coup", empty.hash() != a.hash());
	}

	// 4. GÉNÉRATION DE CANDIDATS.
	//    Le premier coup n'a aucune pierre voisine : sans cas particulier, la
	//    liste serait vide et la recherche ne rendrait aucun coup.
	{
		EngineState empty;
		auto c = empty.candidates();
		expect("plateau vide : au moins un candidat", !c.empty());

		EngineState s = play({ {9,9} });
		auto c2 = s.candidates();
		expect("candidats voisins d'une pierre", !c2.empty());
		bool allEmpty = true, allNear = true;
		for (Pos p : c2) {
			if (!s.game.stone(p).empty()) allEmpty = false;
			int dx = p.x - 9, dy = p.y - 9;
			if (dx*dx > 4 || dy*dy > 4) allNear = false;
		}
		expect("candidats tous vides", allEmpty);
		expect("candidats tous proches", allNear);
	}

	// 4bis. ORDRE DES CANDIDATS.
	//    La recherche ne garde que les premiers : un coup gagnant relégué en
	//    fin de liste n'est jamais exploré. C'est le piège du signe — after(p)
	//    évalue du point de vue de l'ADVERSAIRE, puisque c'est à lui de jouer.
	{
		// Joueur 0 a quatre alignes et le trait : (9,5) et (4,5) font cinq.
		EngineState s = play({
			{5,5}, {0,0}, {6,5}, {0,1}, {7,5}, {0,2}, {8,5}, {0,3}
		});
		expect("c'est bien au joueur 0 de jouer", s.player() == 0);
		auto c = s.candidates();
		expect("des candidats existent", !c.empty());
		if (!c.empty()) {
			Pos first = c.front();
			bool winning = (first == Pos{9,5}) || (first == Pos{4,5});
			expect("le coup gagnant est en tete des candidats", winning,
				"tri probablement inverse (signe de evaluate)");
			expect("jouer ce coup termine la partie",
				s.after(Pos{9,5}).winner() == 0);
		}
	}

	// 5. LA RECHERCHE RETOURNE UN COUP JOUABLE.
	{
		search::Limits lim;
		lim.minDepth = 2;
		lim.maxDepth = 4;
		lim.budget = lim.hardBudget = std::chrono::milliseconds(200);
		search::Searcher<EngineState> searcher(lim);

		EngineState s = play({ {9,9}, {9,10} });
		auto move = searcher.search(s);
		expect("la recherche rend un coup", move.has_value());
		if (move)
			expect("le coup rendu est sur une case vide",
				s.game.stone(*move).empty());
	}

	// 6. MENACES FORTES : four/three OUVERT contre four/three BLOQUE.
	//    C'est ce que le comptage par fenetres seul ne sait pas dire : il est
	//    lineaire en l'ouverture (2 fenetres contre 1), alors que l'ecart de
	//    valeur reel est celui entre gagner et se faire parer. Une erreur ici
	//    ne plante pas — le bot laisse simplement passer des fours ouverts.
	{
		auto T = [](std::initializer_list<Pos> mine,
		            std::initializer_list<Pos> theirs) {
			return board(mine, theirs).threats(0);
		};

		expect("four ouvert .XXXX. detecte",
			T({{6,5},{7,5},{8,5},{9,5}}, {}).open4 >= 1);
		expect("four bloque OXXXX. non compte comme ouvert",
			T({{6,5},{7,5},{8,5},{9,5}}, {{5,5}}).open4 == 0);
		// XX.XX est a un coup du five comme un four contigu, mais le trou en
		// est l'UNIQUE completion : c'est un four ferme, pas ouvert.
		expect("four casse XX.XX compte comme ferme",
			T({{6,5},{7,5},{9,5},{10,5}}, {}).open4 == 0);

		expect("three ouvert .XXX. detecte",
			T({{6,5},{7,5},{8,5}}, {}).open3 >= 1);
		expect("three bloque OXXX. non compte comme ouvert",
			T({{6,5},{7,5},{8,5}}, {{5,5}}).open3 == 0);
		// Le modele par fenetres ignore les trous par construction : le three
		// casse tombe tout seul, sans bonus dedie.
		expect("three casse .X.XX. detecte",
			T({{6,5},{8,5},{9,5}}, {}).open3 >= 1);

		expect("fourche : deux axes porteurs de menaces",
			T({{6,5},{7,5},{8,5},{7,6},{7,7}}, {}).axes >= 2);

		// La consequence qui compte : l'evaluation doit separer les deux.
		EngineState open{Rules{}}, closed{Rules{}};
		open.game   = board({{6,5},{7,5},{8,5},{9,5}}, {});
		closed.game = board({{6,5},{7,5},{8,5},{9,5}}, {{5,5}});
		expect("le four ouvert est mieux evalue que le bloque",
			open.evaluate() > closed.evaluate(),
			"les termes de menace ne remontent pas dans evaluate()");
	}

	std::printf("%d verifications, %d echec(s)\n", checks, failures);
	return failures != 0;
}
