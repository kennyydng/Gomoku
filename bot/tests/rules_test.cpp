// Legalite : double-trois, double-quatre, overline interdit.
//
// Ces regles decident si un coup est JOUABLE. Une erreur ici ne plante pas :
// le bot propose simplement un coup que l'arbitre refuse, ou s'interdit un
// coup permis. Le portage vient de la branche main, ou deux bugs avaient ete
// trouves — alignements comptes deux fois par direction, et confusion
// overline/O4/C4. Les cas ci-dessous couvrent exactement ces deux pieges.

#include "engine_state.hpp"

#include <cstdio>
#include <vector>

static int checks = 0, failures = 0;

static void expect(const char *what, bool ok, const char *detail = "") {
	checks++;
	if (!ok) {
		failures++;
		std::printf("  ECHEC %-52s %s\n", what, detail);
	}
}

// Plateau construit pierre par pierre pour les deux joueurs ; le joueur 1
// recoit des coups de remplissage loin de l'action pour garder la parite.
// Sans eux les pierres changeraient de proprietaire.
static Gomoku board(Rules rules, std::initializer_list<Pos> mine,
                    std::initializer_list<Pos> theirs = {}) {
	static const Pos far[] = {
		{18,0},{18,3},{18,6},{18,9},{18,12},{0,18},{3,18},{6,18},{9,18}
	};
	std::vector<Pos> a(mine), b(theirs);
	Gomoku g{rules};
	std::size_t f = 0;
	for (std::size_t i = 0; i < std::max(a.size(), b.size()); i++) {
		if (i < a.size()) g.play(a[i]);
		else              g.pass();
		if (i < b.size()) g.play(b[i]);
		else              g.play(far[f++]);
	}
	return g;
}

int main() {
	std::printf("Legalite : double-trois, double-quatre, overline\n");

	// Deux threes ouverts se croisant en (7,7) : horizontal (6,7),(8,7) et
	// vertical (7,6),(7,8). Y poser une pierre cree les deux d'un coup.
	const std::initializer_list<Pos> cross = {{6,7},{8,7},{7,6},{7,8}};

	{
		Gomoku g = board(Rules{"911110"}, cross);   // double-trois actif
		expect("double-trois interdit", !g.isLegalMove({7,7}, 0));
		expect("un coup ordinaire reste legal", g.isLegalMove({15,2}, 0));
	}
	{
		Gomoku g = board(Rules{"911100"}, cross);   // double-trois desactive
		expect("regle desactivee : le meme coup est legal",
			g.isLegalMove({7,7}, 0),
			"la regle s'applique alors qu'elle n'est pas demandee");
	}

	// UN seul three ouvert ne doit rien interdire. C'est le piege du bug
	// corrige sur main : compter les 8 directions au lieu des 4 axes faisait
	// voir un double-trois dans tout trois simple.
	{
		Gomoku g = board(Rules{"911110"}, {{6,7},{8,7}});
		expect("un three ouvert simple reste legal", g.isLegalMove({7,7}, 0),
			"l'alignement est probablement compte deux fois (8 directions)");
	}

	// Un coup gagnant echappe aux formes interdites.
	{
		Gomoku g = board(Rules{"911110"}, {{6,7},{8,7},{7,6},{7,8},
			{3,7},{4,7},{5,7}});
		expect("un coup gagnant echappe aux formes interdites",
			g.isLegalMove({7,7}, 0));
	}

	// Double-quatre : deux fours crees par le meme coup.
	{
		const std::initializer_list<Pos> two4 = {
			{5,7},{6,7},{8,7},          // horizontal : XX.X autour de (7,7)
			{7,4},{7,5},{7,6}           // vertical   : XXX sous (7,7)
		};
		Gomoku a = board(Rules{"911111"}, two4);
		expect("double-quatre interdit", !a.isLegalMove({7,7}, 0));
		Gomoku b = board(Rules{"911110"}, two4);
		expect("regle desactivee : double-quatre legal",
			b.isLegalMove({7,7}, 0));
	}

	// Overline interdit ('f') : six alignees.
	{
		const std::initializer_list<Pos> five = {{4,7},{5,7},{6,7},{8,7},{9,7}};
		Gomoku a = board(Rules{"911f10"}, five);
		expect("overline interdit quand la regle le demande",
			!a.isLegalMove({7,7}, 0));
		Gomoku b = board(Rules{"911110"}, five);
		expect("overline autorise sinon", b.isLegalMove({7,7}, 0));
	}

	// Lecture de la ligne de regles : les deux moteurs doivent l'interpreter
	// pareil, sinon ils ne jouent pas au meme jeu.
	{
		Rules r{"911110"};
		expect("capture lue", r.capture);
		expect("capture imparfaite lue", r.captureUnperfect);
		expect("overline gagnant lu", r.players[0].overlineWins);
		expect("double-trois lu", r.players[0].threeThree && r.players[1].threeThree);
		expect("double-quatre absent", !r.players[0].fourFour);

		Rules b{"9101b0"};   // 'b' = noir seulement
		expect("'b' ne vise que noir",
			b.players[0].threeThree && !b.players[1].threeThree);
		Rules none{"900000"};
		expect("aucune restriction : sortie immediate",
			!none.restricted(0) && !none.restricted(1));
	}

	// Overline gagnant ou non. Le six doit apparaitre EN UN COUP, sinon le
	// cinq intermediaire aurait deja termine la partie.
	{
		const std::initializer_list<Pos> five = {{4,7},{5,7},{6,7},{8,7},{9,7}};
		Gomoku a = board(Rules{"900100"}, five);   // overline '1' : gagne
		a.play({7,7});
		expect("six alignees gagnent quand overline='1'",
			a.is_over() && a.winner() == 0);

		Gomoku b = board(Rules{"900000"}, five);   // overline '0' : ne gagne pas
		b.play({7,7});
		expect("six alignees ne gagnent pas quand overline='0'",
			!b.is_over(),
			"un six est compte comme un cinq (deux fenetres pleines)");
	}

	// Victoire differee : un cinq dont une pierre est prenable ne gagne qu'a
	// condition de survivre au coup adverse.
	{
		// (5,5)..(9,5) pour noir ; (5,5)-(5,6) forment une paire prenable
		// en (5,7), le flanc oppose (5,4) etant a blanc.
		auto setup = [](Rules r) {
			Gomoku g{r};
			const Pos seq[] = {
				{5,5},{5,4}, {6,5},{18,0}, {7,5},{18,3},
				{8,5},{18,6}, {5,6},{18,9}, {9,5}   // le cinq se ferme ici
			};
			for (Pos p : seq)
				g.play(p);
			return g;
		};

		Gomoku off = setup(Rules{"910100"});   // captureUnperfect desactivee
		expect("sans la regle, le cinq gagne tout de suite",
			off.is_over() && off.winner() == 0);

		Gomoku a = setup(Rules{"911100"});     // captureUnperfect activee
		expect("cinq prenable : victoire differee, pas encore acquise",
			!a.is_over(),
			"le cinq imparfait n'est pas detecte");
		a.play({5,7});                         // blanc capture (5,6) et (5,5)
		expect("le cinq casse ne donne pas la victoire", !a.is_over(),
			"la ligne cassee compte quand meme comme gagnante");
		expect("la capture a bien eu lieu", a.captures(1) == 2);

		Gomoku b = setup(Rules{"911100"});
		b.play({18,12});                       // blanc joue ailleurs
		expect("le cinq intact donne la victoire au coup suivant",
			b.is_over() && b.winner() == 0,
			"la victoire differee n'est jamais confirmee");
	}

	// Endgame capture, seconde clause du sujet : « If the player has already
	// lost four pairs and the opponent can capture one more, the opponent wins
	// by capture. »
	//
	// Elle ne se confond pas avec la premiere. La premiere parle de CASSER la
	// ligne, donc d'une paire prise DANS l'alignement. Celle-ci parle de
	// compter : a huit pierres perdues, n'importe quelle paire prise ailleurs
	// sur le plateau porte l'adversaire a dix et gagne. Un cinq parfait ne
	// suffit donc plus a conclure.
	//
	// Et la troisieme puce dit comment la cabler : « If there is no
	// possibility of this happening, there is no need to continue the game. »
	// S'il y a une possibilite, la partie continue d'un coup — le meme
	// mecanisme de victoire differee que la premiere clause.
	{
		// Quatre paires noires offertes puis capturees par blanc, une
		// cinquieme laissee prenable en (3,8), et un cinq de noir en (5,12)
		// a (9,12) dont aucune paire n'est prenable.
		auto setup = []() {
			Gomoku g{Rules{"911100"}};
			Pos const moves[] = {
				{1,0},{0,0}, {2,0},{3,0},    // blanc capture -> 2
				{1,2},{0,2}, {2,2},{3,2},    // -> 4
				{1,4},{0,4}, {2,4},{3,4},    // -> 6
				{1,6},{0,6}, {2,6},{3,6},    // -> 8
				{1,8},{0,8}, {2,8},{18,18},  // paire prenable en (3,8)
				{5,12},{18,16}, {6,12},{18,14},
				{7,12},{18,12}, {8,12},{18,10},
				{9,12},                      // noir aligne cinq
			};
			for (Pos const m : moves)
				g.play(m);
			return g;
		};

		Gomoku g = setup();
		expect("blanc a bien quatre paires", g.captures(1) == 8);
		expect("et peut en prendre une cinquieme", g.wouldCapture({3,8}, 1) == 2);
		expect("le cinq ne conclut pas quand l'adversaire peut compter jusqu'a dix",
			!g.is_over(),
			"noir gagne alors que blanc atteint dix prises au coup suivant");

		Gomoku taken = setup();
		taken.play({3,8});                   // blanc prend la cinquieme paire
		expect("blanc gagne par capture", taken.is_over() && taken.winner() == 1,
			"la dixieme pierre prise ne donne pas la victoire");

		Gomoku missed = setup();
		missed.play({18,8});                 // blanc joue ailleurs
		expect("sinon le cinq de noir tient", missed.is_over() && missed.winner() == 0,
			"le cinq n'est jamais confirme");

		// Sans la regle de fin de partie, le cinq conclut immediatement :
		// cette clause ne doit pas fuir dans les autres jeux de regles.
		Gomoku off{Rules{"910100"}};
		Pos const moves[] = {
			{1,0},{0,0}, {2,0},{3,0}, {1,2},{0,2}, {2,2},{3,2},
			{1,4},{0,4}, {2,4},{3,4}, {1,6},{0,6}, {2,6},{3,6},
			{1,8},{0,8}, {2,8},{18,18},
			{5,12},{18,16}, {6,12},{18,14}, {7,12},{18,12}, {8,12},{18,10},
			{9,12},
		};
		for (Pos const m : moves)
			off.play(m);
		expect("regle desactivee : le cinq conclut tout de suite",
			off.is_over() && off.winner() == 0);
	}

	// Taille de plateau. Le stockage reste en 19x19, mais un plateau 15x15
	// doit etre joue comme un 15x15 — sinon le bot propose des coups hors de
	// la grille affichee, et compte des fenetres qui ne peuvent jamais etre
	// completees.
	{
		expect("taille lue : '5' donne 15", Rules{"500000"}.size == 15);
		expect("taille lue : '9' donne 19", Rules{"900000"}.size == 19);

		// Quatre pierres noires en (11,0)..(14,0). Sur 19x19, la fenetre
		// 11..15 existe et l'ensemble est un four OUVERT. Sur 15x15 elle
		// deborde : il ne reste qu'une fenetre vivante, donc un four ferme.
		auto four = [](Rules r) {
			Gomoku g{r};
			const Pos seq[] = {
				{11,0},{0,14}, {12,0},{2,14}, {13,0},{4,14}, {14,0}
			};
			for (Pos p : seq)
				g.play(p);
			return g;
		};

		Gomoku big = four(Rules{"900000"});
		expect("19x19 : la fenetre 11..15 existe, four ouvert",
			big.threats(0).open4 >= 1);

		Gomoku small = four(Rules{"500000"});
		expect("15x15 : la fenetre qui deborde ne compte pas",
			small.threats(0).open4 == 0,
			"le moteur voit un four ouvert qui ne peut pas se completer");

		expect("le bord du plateau logique est jouable",
			small.onBoard({14,14}));
		expect("au-dela, non", !small.onBoard({15,0}));
		expect("et le coup y est illegal", !small.isLegalMove({15,0}, 0));
	}

	std::printf("%d verifications, %d echec(s)\n", checks, failures);
	return failures != 0;
}
