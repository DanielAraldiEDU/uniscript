import { theme } from '../theme'

export function HeaderBar({ onCompile }: { onCompile: () => void }) {
  return (
    <header style={{ display: 'flex', alignItems: 'center', gap: 12, background: theme.panel, padding: 10, borderBottom: `1px solid ${theme.border}` }}>
      <div style={{ fontSize: 16, fontWeight: 700 }}>UniScript</div>
      <div style={{ flex: 1 }} />
      <button onClick={onCompile} style={{ color: theme.text, background: theme.button, border: `1px solid ${theme.border}`, borderRadius: 8, padding: '8px 14px' }}>Compilar</button>
    </header>
  )
}

