import { useEffect } from 'react'
import { theme } from '../theme'

export function HeaderBar({ onCompile }: { onCompile: () => void }) {
  useEffect(() => {
    function onKeyDown(e: KeyboardEvent) {
      if (e.key === 'F5') {
        e.preventDefault()
        onCompile()
      }
    }

    document.body.addEventListener('keydown', onKeyDown);

    return () => {
      document.body.removeEventListener('keydown', onKeyDown);
    }
  }, [])

  return (
    <header style={{ display: 'flex', alignItems: 'center', gap: 12, background: theme.panel, padding: 10, borderBottom: `1px solid ${theme.border}` }}>
      <div style={{ fontSize: 16, fontWeight: 700 }}>UniScript</div>
      <div style={{ flex: 1 }} />
      <button title='Compilar (F5)' onClick={onCompile} style={{ color: theme.text, background: theme.button, border: `1px solid ${theme.border}`, borderRadius: 8, padding: '8px 14px' }}>Compilar</button>
    </header>
  )
}

