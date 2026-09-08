#pragma once

// Recherche portée depuis la branche `main`, où chaque élément ci-dessous a
// été mesuré : négamax + élagage alpha-bêta + PVS/negascout + table de
// transposition + iterative deepening + budget de temps + coupe et tri des
// candidats.
//
// Pourquoi une couche générique plutôt que du code câblé sur Gomoku : la
// branche bitboard exige un compilateur avec la réflexion C++26, que tout le
// monde n'a pas. Écrite contre le concept `State` ci-dessous, la recherche se
// compile et se teste en C++23 standard contre un stub (voir
// tests/search_test.cpp, qui vérifie qu'alpha-bêta rend exactement la même
// valeur que le minimax complet). Seul le branchement final au moteur reste à
// vérifier sur une machine qui peut le compiler.
//
// Ordre de grandeur, pour situer l'enjeu : à branchement 40 et profondeur 4,
// le minimax pur demande ~2.5M nœuds là où alpha-bêta bien ordonné en demande
// ~1600. L'algorithme vaut plus que la représentation.

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <optional>
#include <vector>

namespace search {

// --- Ce que la recherche exige de l'état de jeu ---------------------------
//
// `after()` renvoie une copie avec le coup joué, plutôt qu'un couple
// apply/undo : c'est la forme retenue par le moteur bitboard, dont les
// mesures montrent que copier un état POD coûte moins cher que défaire un
// coup. La recherche ne suppose rien d'autre.
template<class S>
concept State = requires(S const cs, typename S::move_t m) {
	{ cs.player()    } -> std::convertible_to<bool>;
	{ cs.terminal()  } -> std::convertible_to<bool>;
	{ cs.winner()    } -> std::convertible_to<int>;   // -1 = nulle/aucun
	{ cs.evaluate()  } -> std::convertible_to<int>;   // du point de vue du trait
	{ cs.hash()      } -> std::convertible_to<uint64_t>;
	{ cs.candidates()} -> std::convertible_to<std::vector<typename S::move_t>>;
	{ cs.after(m)    } -> std::convertible_to<S>;
	// La table de transposition retrouve son coup dans la liste courante pour
	// l'essayer en premier : sans égalité comparable, tout l'ordonnancement
	// tombe. Exigé ici pour que l'erreur soit lisible au lieu de surgir des
	// entrailles de std::find.
	{ m == m        } -> std::convertible_to<bool>;
};

struct Limits {
	std::chrono::milliseconds budget{460};
	// Le sujet exige *toujours* au moins minDepth plis. Les profondeurs
	// jusque-là ont un plafond élargi ; au-delà, le budget normal s'applique.
	std::chrono::milliseconds hardBudget{800};
	int minDepth = 10;
	int maxDepth = 14;
	// Cinq candidats était le réglage de main, où six sortait du budget de
	// temps. Ce moteur, lui, peut se le payer : mesuré à 911110, largeur 6
	// bat largeur 5 sur 40 parties (72.5%, 10 paires décisives sur 11,
	// p = 0.006) tout en restant à 0 coup sur 36 au-dessus de 500ms.
	// Sept ne rapporte plus rien (50% contre six, p = 0.64) et fait passer
	// 4 coups sur 36 au-dessus de 500ms ; huit sort franchement du critère,
	// avec 550ms de moyenne.
	std::size_t maxBranch = 6;
	// Désactivable pour isoler la table lors d'un diagnostic : elle ne doit
	// jamais changer le résultat, seulement le temps mis pour l'obtenir.
	bool useTT = true;
};

inline constexpr int MATE  = 1'000'000;
inline constexpr int INF   = 2'000'000;

struct Stats {
	unsigned long nodes = 0;
	int depth = 0;
	int score = 0;          // valeur de la racine, du point de vue du trait
	bool aborted = false;
};

// --- Table de transposition ----------------------------------------------

enum class Flag : std::uint8_t { Exact, Lower, Upper };

template<class M>
struct Entry {
	std::uint64_t key = 0;
	int depth = -1;
	int score = 0;
	Flag flag = Flag::Exact;
	std::optional<M> best{};
};

struct Aborted {};

template<State S>
class Searcher {
public:
	using M = typename S::move_t;

	explicit Searcher(Limits limits = {})
		: _limits(limits), _tt(1u << 21) {}

	Stats const &stats() const { return _stats; }

	// Restreint la RACINE à ces coups (dans cet ordre). Sert à n'y jouer que
	// des coups légaux : la légalité complète (double-trois, double-quatre,
	// overline interdit) est trop chère pour être revérifiée à chaque nœud, et
	// seul le coup réellement joué doit être garanti légal. Vide = tous les
	// candidats, comportement par défaut.
	void setRootMoves(std::vector<M> moves) { _rootMoves = std::move(moves); }

	// Iterative deepening : on approfondit tant que le budget le permet, et
	// la profondeur précédente sert de garde-fou si la suivante est coupée.
	// Le coup de la profondeur N-1 alimente aussi l'ordre de la profondeur N
	// via la table, ce qui rend l'approfondissement moins cher qu'il n'y
	// paraît.
	std::optional<M> search(S const &root) {
		_stats = {};
		_start = std::chrono::steady_clock::now();

		std::vector<M> moves = _rootMoves.empty() ? root.candidates() : _rootMoves;
		if (moves.empty())
			return std::nullopt;

		std::optional<M> best = moves.front();
		for (int depth = 1; depth <= _limits.maxDepth; depth++) {
			_deadline = _start + (depth <= _limits.minDepth
				? _limits.hardBudget : _limits.budget);
			if (depth > _limits.minDepth
			 && std::chrono::steady_clock::now() >= _deadline)
				break;
			try {
				auto [move, score] = searchRoot(root, depth);
				best = move;
				_stats.depth = depth;
				_stats.score = score;
				if (score >= MATE)     // victoire prouvée : inutile d'aller plus loin
					break;
			} catch (Aborted const &) {
				_stats.aborted = true;
				break;
			}
		}
		return best;
	}

private:
	std::pair<std::optional<M>, int> searchRoot(S const &root, int depth) {
		std::vector<M> moves;
		if (_rootMoves.empty()) {
			moves = ordered(root, depth);
		} else {
			moves = _rootMoves;
			if (moves.size() > _limits.maxBranch)
				moves.resize(_limits.maxBranch);
		}
		std::optional<M> best = moves.empty() ? std::nullopt
		                                      : std::optional<M>(moves.front());
		int alpha = -INF;

		for (M const &m : moves) {
			tick();
			S next = root.after(m);
			int v = -negamax(next, depth - 1, -INF, -alpha);
			if (v > alpha) {
				alpha = v;
				best = m;
			}
		}
		return {best, alpha};
	}

	int negamax(S const &state, int depth, int alpha, int beta) {
		tick();

		if (state.terminal()) {
			int w = state.winner();
			if (w < 0)
				return 0;
			// +depth : entre deux victoires, préférer la plus rapide.
			return (w == (int)state.player() ? MATE + depth : -(MATE + depth));
		}
		if (depth == 0)
			return state.evaluate();

		const std::uint64_t key = state.hash();
		Entry<M> &slot = _tt[key % _tt.size()];
		std::optional<M> ttMove;
		if (_limits.useTT && slot.key == key) {
			ttMove = slot.best;
			if (slot.depth >= depth) {
				if (slot.flag == Flag::Exact)
					return slot.score;
				if (slot.flag == Flag::Lower)
					alpha = std::max(alpha, slot.score);
				else
					beta = std::min(beta, slot.score);
				if (alpha >= beta)
					return slot.score;
			}
		}

		std::vector<M> moves = ordered(state, depth, ttMove);
		if (moves.empty())
			return state.evaluate();

		const int origAlpha = alpha;
		int best = -INF;
		std::optional<M> bestMove = moves.front();
		bool first = true;

		for (M const &m : moves) {
			S next = state.after(m);
			int v;
			if (first) {
				v = -negamax(next, depth - 1, -beta, -alpha);
			} else {
				// PVS : le premier coup vient de la table, donc les suivants
				// sont a priori moins bons. Une fenêtre nulle suffit à le
				// confirmer, bien moins chère ; on ne re-cherche à fenêtre
				// complète que si l'hypothèse est fausse, ce qui est rare
				// quand l'ordre est bon.
				v = -negamax(next, depth - 1, -alpha - 1, -alpha);
				if (v > alpha && v < beta)
					v = -negamax(next, depth - 1, -beta, -alpha);
			}
			first = false;

			if (v > best) {
				best = v;
				bestMove = m;
			}
			alpha = std::max(alpha, best);
			if (alpha >= beta)
				break;
		}

		if (_limits.useTT)
			slot = {key, depth, best,
				best <= origAlpha ? Flag::Upper
				: best >= beta    ? Flag::Lower : Flag::Exact,
				bestMove};
		return best;
	}

	// Coupe à maxBranch : un coup hors de cette liste n'est jamais exploré,
	// ce qui en fait le filtre le plus décisif du moteur. Mesuré sur `main` :
	// descendre de 5 à 4 fait chuter le score à 12.5%, monter à 6 le fait
	// grimper à 79.2% mais sort du budget de temps. C'est à l'état de fournir
	// des candidats déjà triés par pertinence.
	std::vector<M> ordered(S const &state, int, std::optional<M> ttMove = {}) const {
		std::vector<M> moves = state.candidates();
		if (moves.size() > _limits.maxBranch)
			moves.resize(_limits.maxBranch);
		if (ttMove) {
			auto it = std::find(moves.begin(), moves.end(), *ttMove);
			if (it != moves.end())
				std::rotate(moves.begin(), it, it + 1);
		}
		return moves;
	}

	void tick() {
		// Vérifier l'horloge à chaque nœud coûterait plus cher que la
		// recherche elle-même ; un nœud sur 128 suffit largement à tenir le
		// budget.
		if (++_stats.nodes % 128 == 0
		 && std::chrono::steady_clock::now() >= _deadline)
			throw Aborted{};
	}

	Limits _limits;
	std::vector<Entry<M>> _tt;
	Stats _stats;
	std::vector<M> _rootMoves;
	std::chrono::steady_clock::time_point _start{}, _deadline{};
};

} // namespace search
