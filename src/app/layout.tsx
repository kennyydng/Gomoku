import './globals.css'
import type { Metadata } from 'next'
import { Manrope } from 'next/font/google'

const manrope = Manrope({ subsets: ['latin'] })

export const metadata: Metadata = {
  title: 'Gomoku 42',
  description: 'Gomoku boilerplate for Next.js App Router with local mode and mock AI.',
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
