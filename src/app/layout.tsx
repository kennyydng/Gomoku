import './globals.css'
import type { Metadata } from 'next'
import { Manrope } from 'next/font/google'

const manrope = Manrope({ subsets: ['latin'] })

export const metadata: Metadata = {
  title: 'Gomoku 42',
  description: 'Boilerplate Gomoku pour Next.js App Router avec mode local et IA mock.',
}

export default function RootLayout({
  children,
}: {
  children: React.ReactNode
}) {
  return (
    <html lang="fr">
      <body className={manrope.className}>{children}</body>
    </html>
  )
}
