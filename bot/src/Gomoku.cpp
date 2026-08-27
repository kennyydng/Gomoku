
#include "Gomoku.class.hpp"
#include <cassert>
#include <iomanip>
#include <iostream>

// Numéros de ligne/colonne en 1-indexé (comme affiché à l'écran par l'UI),
// pour repérer visuellement une case citée dans les logs de debug (ex:
// "Candidates ... 8:11").
std::ostream &operator<<(std::ostream &o, Gomoku const &gomoku) {
	o << gomoku.turn() << " " <<
		Stone(Stone::WHITE) << gomoku.score(0) << "-" <<
		Stone(Stone::BLACK) << gomoku.score(1) << "\n";

	const int8_t size = (int8_t)current_board_size();

	o << "   ";
	for (int8_t x = 0; x < size; x++)
		o << std::setw(2) << (int)(x + 1);
	o << "\n";

	Gomoku::forall([&](Pos &&pos){
		if (pos.x == 0)
			o << std::setw(2) << (int)(pos.y + 1) << " ";
		o << gomoku.stone(pos) << (pos.x == size - 1 ? "\n" : " ");
	});
	return o;
}

void Gomoku::play(Pos move) {
	const bool player = this->player();
	uint16_t cap_mask = rules.capture ? 0b111'1'1'111 : 0; // les 8 bits hauts restent libres
	const int8_t max_capture_anchor = (int8_t)rules.boardSize - 4;

	assert(move.valid() && stone(move).empty());

	if (move.x < 3 ) cap_mask &= (uint16_t)0b110'1'0'110;
	if (move.x > max_capture_anchor) cap_mask &= (uint16_t)0b011'0'1'011;
	if (move.y < 3 ) cap_mask &= (uint16_t)0b111'1'1'000;
	if (move.y > max_capture_anchor) cap_mask &= (uint16_t)0b000'1'1'111;

	stones[player] += move;
	hash ^= zobristKey(move, player);
	for (int n = 0; n < 8; n++) {
		if (cap_mask>>n & 1) {
			const Pos dir = SUBDIRECTIONS[n];
			const Pos pos0 = move + dir;
			const Pos pos1 = pos0 + dir;
			const Pos pos2 = pos1 + dir;
			if (
				stones[!player][pos0] &&
				stones[!player][pos1] &&
				stones[ player][pos2]
			) {
				stones[!player] -= pos0;
				stones[!player] -= pos1;
				hash ^= zobristKey(pos0, !player);
				hash ^= zobristKey(pos1, !player);
				captures[player] += 2;
			} else {
				cap_mask &= ~(1<<n);
			}
		}
	}
	hash ^= zobristSideKey();

	moves.push_back({move,cap_mask});
}

void Gomoku::undo() {
	const Move record = moves.back();
	const auto [move, cap_mask, delayedWinBefore, delayedLine, delayedDirBefore, delayedPlayerBefore] = record;
	moves.pop_back();

	// Calculé après le pop : player() redevient celui qui a joué ce coup.
	const bool player = this->player();

	hash ^= zobristSideKey();
	for (int n = 0; n < 8; n++) {
		if ((cap_mask>>n) & 1) {
			const Pos dir = SUBDIRECTIONS[n];
			const Pos pos0 = move + dir;
			const Pos pos1 = pos0 + dir;

			stones[!player] += pos0;
			stones[!player] += pos1;
			hash ^= zobristKey(pos0, !player);
			hash ^= zobristKey(pos1, !player);

			captures[player] -= 2;
		}
	}
	stones[player] -= move;
	hash ^= zobristKey(move, player);

	delayedWin = delayedWinBefore;
	delayedStart = delayedLine[0];
	delayedEnd = delayedLine[1];
	delayedDir = delayedDirBefore;
	delayedPlayer = delayedPlayerBefore;
};

Pos Gomoku::runStart(Pos pos, Pos dir, bool player) const {
	Pos p = pos;
	Pos back = p - dir;
	while (back.valid() && stones[player][back]) {
		p = back;
		back = p - dir;
	}
	return p;
}

Pos Gomoku::runEnd(Pos pos, Pos dir, bool player) const {
	Pos p = pos;
	Pos fwd = p + dir;
	while (fwd.valid() && stones[player][fwd]) {
		p = fwd;
		fwd = p + dir;
	}
	return p;
}

// Suppose que `pos` contient déjà la pierre de `player` (comme getThreats).
Threat Gomoku::threatAt(Pos pos, Pos dir, bool player, int min) {
	const Pos start = runStart(pos, dir, player);
	const Pos end = runEnd(pos, dir, player);
	const int len = runLenOf(start, end, dir);

	const bool overlineApplies = rules.players[player].overline;
	const bool flankingApplies = rules.players[player].flanking;
	const Pos beforeStart = start - dir;
	const Pos afterEnd = end + dir;
	const bool flank0 = flankingApplies && beforeStart.valid() && stones[!player][beforeStart];
	const bool flank1 = flankingApplies && afterEnd.valid() && stones[!player][afterEnd];

	if (len > 5 && overlineApplies)
		return {ThreatType::Overline, start, end, dir};
	if (len >= 5) {
		if (len > 5 || !flank0 || !flank1)
			return {ThreatType::Five, start, end, dir};
		return {};
	}
	if (min >= 5)
		return {};

	const Threat ext0 = threatOf(beforeStart, dir, player, min + 1);
	const Threat ext1 = threatOf(afterEnd, dir, player, min + 1);
	const bool is5_0 = ext0.type == ThreatType::Five;
	const bool is5_1 = ext1.type == ThreatType::Five;

	if (is5_0 || is5_1) {
		const Pos line4start = is5_0 ? ext0.start : start;
		const Pos line4end = is5_1 ? ext1.end : end;
		if (is5_0 && is5_1)
			return {len == 4 ? ThreatType::O4 : ThreatType::FourFour, line4start, line4end, dir};
		return {ThreatType::C4, start, end, dir};
	}
	if (min >= 4)
		return {};

	const bool isO4_0 = ext0.type == ThreatType::O4;
	const bool isO4_1 = ext1.type == ThreatType::O4;
	if (isO4_0 || isO4_1) {
		const Pos line3start = isO4_0 ? ext0.start : start;
		const Pos line3end = isO4_1 ? ext1.end : end;
		return {ThreatType::O3, line3start, line3end, dir};
	}

	return {};
}

Threat Gomoku::threatOf(Pos pos, Pos dir, bool player, int min) {
	if (!pos.valid() || !stone(pos).empty())
		return {};
	rawPlace(pos, player);
	Threat t = threatAt(pos, dir, player, min);
	rawRemove(pos, player);
	return t;
}

std::vector<Threat> Gomoku::getThreats(Pos pos, bool player, int min) {
	std::vector<Threat> threats;
	for (const Pos &dir : DIRECTIONS) {
		Threat t = threatAt(pos, dir, player, min);
		if (t.type != ThreatType::None)
			threats.push_back(t);
	}
	return threats;
}

bool Gomoku::isUnperfect5(Pos start, Pos end, Pos dir, bool player) {
	const bool opponent = !player;
	const int len = runLenOf(start, end, dir);

	for (int i = 0; i < len; i++) {
		const Pos pos = stepPos(start, dir, i);
		for (const Pos &delta : SUBDIRECTIONS) {
			const Pos flank0 = pos - delta;
			const Pos pos1 = pos + delta;
			const Pos flank1 = pos + delta * 2;

			if (!flank0.valid() || !flank1.valid())
				continue;
			if (!stones[player][pos1])
				continue;

			const bool flank0Opponent = stones[opponent][flank0];
			const bool flank0Empty = stone(flank0).empty();
			const bool flank1Opponent = stones[opponent][flank1];
			const bool flank1Empty = stone(flank1).empty();

			if ((flank0Opponent && flank1Empty) || (flank1Opponent && flank0Empty))
				return true;
		}
	}
	return false;
}

bool Gomoku::wouldCapture(Pos move, bool player) {
	for (const Pos &dir : SUBDIRECTIONS) {
		Pos pos1 = move + dir, pos2 = pos1 + dir, pos3 = pos2 + dir;
		if (pos3.valid() && stones[!player][pos1] && stones[!player][pos2] && stones[player][pos3])
			return true;
	}
	return false;
}

bool Gomoku::isLegalMove(Pos pos, bool player) {
	if (!pos.valid() || !stone(pos).empty())
		return false;

	const auto &playerRules = rules.players[player];
	if (!playerRules.threeThree && !playerRules.fourFour && !playerRules.foulOverline)
		return true;

	rawPlace(pos, player);
	std::vector<Threat> threats = getThreats(pos, player, 3);
	rawRemove(pos, player);

	bool winning = false;
	for (const Threat &t : threats)
		if (t.type == ThreatType::Five) { winning = true; break; }

	// Un coup qui capture ou gagne échappe aux règles de forme interdite
	// (même logique que resolveMove() côté frontend).
	if (winning || (rules.capture && wouldCapture(pos, player)))
		return true;

	if (playerRules.foulOverline) {
		for (const Threat &t : threats)
			if (t.type == ThreatType::Overline)
				return false;
	}
	if (playerRules.fourFour) {
		int fourCount = 0;
		bool hasFourFour = false;
		for (const Threat &t : threats) {
			if (t.type == ThreatType::O4 || t.type == ThreatType::C4)
				fourCount++;
			if (t.type == ThreatType::FourFour)
				hasFourFour = true;
		}
		if (fourCount > 1 || hasFourFour)
			return false;
	}
	if (playerRules.threeThree) {
		int threeCount = 0;
		for (const Threat &t : threats)
			if (t.type == ThreatType::O3)
				threeCount++;
		if (threeCount > 1)
			return false;
	}

	return true;
}

Outcome Gomoku::applyMove(Pos pos) {
	const bool mover = this->player();

	const bool wasDelayed = delayedWin;
	const Pos dStart = delayedStart;
	const Pos dEnd = delayedEnd;
	const Pos dDir = delayedDir;
	const bool dPlayer = delayedPlayer;

	play(pos);

	Outcome outcome{};

	if (rules.capture) {
		if (captures[mover] >= 10 && captures[!mover] >= 10)
			outcome = {Result::Draw, false};
		else if (captures[mover] >= 10)
			outcome = {Result::Win, mover};
		else if (captures[!mover] >= 10)
			outcome = {Result::Win, !mover};
	}

	if (outcome.state == Result::Ongoing && wasDelayed) {
		const int len = runLenOf(dStart, dEnd, dDir);
		bool intact = true;
		for (int i = 0; i < len && intact; i++)
			if (!stones[dPlayer][stepPos(dStart, dDir, i)])
				intact = false;

		if (!intact) {
			delayedWin = false;
		} else {
			outcome = {Result::Win, dPlayer};
			delayedWin = false;
		}
	}

	if (outcome.state == Result::Ongoing) {
		std::vector<Threat> threats = getThreats(pos, mover, 5);
		bool hasFive = false;
		Pos fStart{}, fEnd{}, fDir{};

		for (const Threat &t : threats) {
			if (t.type == ThreatType::Overline) {
				outcome = {Result::Win, mover};
				break;
			}
			if (t.type == ThreatType::Five) {
				hasFive = true;
				fStart = t.start;
				fEnd = t.end;
				fDir = t.dir;
			}
		}

		if (outcome.state == Result::Ongoing && hasFive) {
			if (rules.captureUnperfect && isUnperfect5(fStart, fEnd, fDir, mover)) {
				delayedWin = true;
				delayedStart = fStart;
				delayedEnd = fEnd;
				delayedDir = fDir;
				delayedPlayer = mover;
			} else {
				outcome = {Result::Win, mover};
			}
		}
	}

	moves.back().delayedWinBefore = wasDelayed;
	moves.back().delayedLine[0] = dStart;
	moves.back().delayedLine[1] = dEnd;
	moves.back().delayedDir = dDir;
	moves.back().delayedPlayerBefore = dPlayer;

	return outcome;
}
