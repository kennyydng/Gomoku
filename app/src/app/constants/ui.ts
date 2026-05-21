export const MENU_CARD_THEME = {
  base: 'rounded-2xl border p-6 text-left transition',
  selected: 'border-amber-300 bg-amber-200/10 shadow-[0_0_0_1px_rgba(252,211,77,0.16)]',
  idle: 'border-stone-700/40 bg-[#120d09] hover:border-amber-700/50',
} as const

export const GAME_PAGE_THEME = {
  headerPanel: 'relative w-full rounded-[2rem] border border-amber-900/35 bg-[linear-gradient(180deg,_rgba(29,20,13,0.98),_rgba(18,12,8,0.98))] px-6 py-5 text-center shadow-[0_24px_90px_rgba(0,0,0,0.35)] sm:px-8',
  sectionPanel: 'w-full rounded-[2rem] border border-amber-900/35 bg-[linear-gradient(180deg,_rgba(29,20,13,0.98),_rgba(18,12,8,0.98))] p-4 shadow-[0_24px_90px_rgba(0,0,0,0.35)] sm:p-6',
  iconButton: 'inline-flex h-11 w-11 items-center justify-center rounded-full border border-stone-700/50 bg-black/25 text-stone-100 transition hover:border-amber-500/50 hover:bg-black/40',
  statusPill: 'rounded-full border border-amber-400/20 bg-amber-300/10 px-4 py-2 text-sm text-amber-100',
} as const
