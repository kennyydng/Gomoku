export const BOARD_SIZE = 19
export const BOARD_RANGE = BOARD_SIZE - 1

export const BOARD_THEME = {
	outerGradient: 'bg-[linear-gradient(135deg,_#c79b63_0%,_#b8834a_44%,_#8e5b30_100%)]',
	innerGradient: 'bg-[radial-gradient(circle_at_top,_rgba(255,245,220,0.18),_transparent_32%),linear-gradient(180deg,_rgba(255,235,197,0.12),_rgba(255,235,197,0.02))]',
	lineColor: 'rgba(39, 25, 14, 0.72)',
	markColor: 'rgba(67, 46, 23, 0.9)',
	surfaceFill: 'rgba(255, 235, 197, 0.08)',
	cellBackground: 'rgba(255,235,197,0.05)',
	cellBorder: 'rgba(39,25,14,0.35)',
	forbiddenStone: 'radial-gradient(circle at 30% 30%, #ff6b6b, #c92a2a)',
} as const

function joinClasses(...parts: Array<string | false | null | undefined>) {
	return parts.filter(Boolean).join(' ')
}

export const STONE_THEME = {
	black: {
		stone: 'bg-[radial-gradient(circle_at_30%_30%,_#7c7c7c_0%,_#1e1e1e_52%,_#050505_100%)] shadow-[inset_0_2px_3px_rgba(255,255,255,0.16),0_12px_18px_rgba(0,0,0,0.45)]',
		preview: 'bg-black/25',
		turn: 'border-stone-700/60 ring-1 ring-black/15',
		captureFilled: 'scale-110 border-stone-700/60',
		captureEmpty: joinClasses(
			'bg-[radial-gradient(circle_at_30%_30%,_rgba(122,122,122,0.42)_0%,_rgba(49,49,49,0.34)_48%,_rgba(15,15,15,0.3)_100%)] shadow-[inset_0_1px_2px_rgba(255,255,255,0.08),0_4px_8px_rgba(0,0,0,0.16)]',
			'border-stone-500/20 opacity-55',
		),
		help: joinClasses(
			'bg-[radial-gradient(circle_at_30%_30%,_#7c7c7c_0%,_#1e1e1e_52%,_#050505_100%)] shadow-[inset_0_2px_3px_rgba(255,255,255,0.16),0_12px_18px_rgba(0,0,0,0.45)]',
			'border-stone-700/60 ring-1 ring-black/15',
		),
	},
	white: {
		stone: 'bg-[radial-gradient(circle_at_30%_30%,_#fffdf8_0%,_#ddd3c2_52%,_#a89475_100%)] shadow-[inset_0_2px_3px_rgba(255,255,255,0.4),0_12px_18px_rgba(0,0,0,0.3)]',
		preview: 'bg-white/35',
		turn: 'border-stone-300/70 ring-1 ring-white/20',
		captureFilled: 'scale-110 border-stone-300/70',
		captureEmpty: joinClasses(
			'bg-[radial-gradient(circle_at_30%_30%,_rgba(255,255,255,0.42)_0%,_rgba(226,218,206,0.3)_48%,_rgba(168,148,117,0.24)_100%)] shadow-[inset_0_1px_2px_rgba(255,255,255,0.16),0_4px_8px_rgba(0,0,0,0.12)]',
			'border-stone-300/20 opacity-55',
		),
		help: joinClasses(
			'bg-[radial-gradient(circle_at_30%_30%,_#fffdf8_0%,_#ddd3c2_52%,_#a89475_100%)] shadow-[inset_0_2px_3px_rgba(255,255,255,0.4),0_12px_18px_rgba(0,0,0,0.3)]',
			'border-stone-300/70 ring-1 ring-white/20',
		),
	},
	helpForbidden: 'border-red-600/40 bg-[radial-gradient(circle_at_30%_30%,_#ff6b6b_0%,_#c92a2a_100%)] shadow-[0_4px_8px_rgba(0,0,0,0.3)]',
} as const

export function getTurnOrbClass(currentPlayer: 1 | 2) {
	return currentPlayer === 1 ? joinClasses(getStoneClass(1), STONE_THEME.black.turn) : joinClasses(getStoneClass(2), STONE_THEME.white.turn)
}

export function getCaptureOrbClass(isFilled: boolean, tone: 'black' | 'white') {
	if (tone === 'black') {
		return isFilled ? joinClasses(getStoneClass(1), STONE_THEME.black.captureFilled) : STONE_THEME.black.captureEmpty
	}

	return isFilled ? joinClasses(getStoneClass(2), STONE_THEME.white.captureFilled) : STONE_THEME.white.captureEmpty
}

export function getStoneClass(player: 1 | 2) {
	return player === 1 ? STONE_THEME.black.stone : STONE_THEME.white.stone
}

export function getPreviewClass(player: 1 | 2) {
	return player === 1 ? STONE_THEME.black.preview : STONE_THEME.white.preview
}
