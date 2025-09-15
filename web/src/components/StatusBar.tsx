import { theme } from '../theme'

export function StatusBar({ line, col }: { line: number; col: number }) {
  return (
    <footer style={{ background: theme.panel, borderTop: `1px solid ${theme.border}`, color: '#a1a1aa', padding: '6px 10px', fontSize: 12 }}>
      Linha {line}, Coluna {col}
    </footer>
  )
}

