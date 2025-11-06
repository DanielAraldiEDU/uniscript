import { useEffect, useRef, useState } from 'react'
import { theme } from '../theme'

type Props = {
  code: string
}

const headerBg = '#1f1f23'

export function BipViewer({ code }: Props) {
  const [copied, setCopied] = useState(false)
  const timeoutRef = useRef<number | null>(null)
  const hasCode = code.trim().length > 0

  useEffect(() => {
    return () => {
      if (timeoutRef.current !== null) {
        window.clearTimeout(timeoutRef.current)
      }
    }
  }, [])

  const handleCopy = async () => {
    if (!hasCode) return
    try {
      await navigator.clipboard.writeText(code)
      setCopied(true)
      if (timeoutRef.current !== null) {
        window.clearTimeout(timeoutRef.current)
      }
      timeoutRef.current = window.setTimeout(() => {
        setCopied(false)
        timeoutRef.current = null
      }, 1800)
    } catch {
      // ignore clipboard errors silently
    }
  }

  return (
    <div
      style={{
        display: 'flex',
        flexDirection: 'column',
        height: '100%',
        background: theme.panel,
        borderRadius: 8,
        border: `1px solid ${theme.border}`,
        overflow: 'hidden'
      }}
    >
      <div
        style={{
          padding: '12px 16px',
          borderBottom: `1px solid ${theme.border}`,
          background: headerBg,
          color: theme.text
        }}
      >
        <span style={{ fontWeight: 600, fontSize: 15 }}>Código BIP</span>
      </div>
      <div
        style={{
          flex: 1,
          position: 'relative',
          background: theme.panel
        }}
      >
        <div
          style={{
            overflow: 'auto',
            padding: '12px 16px',
            fontFamily:
              'ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, "Liberation Mono", "Courier New", monospace',
            fontSize: 13,
            color: theme.text,
            whiteSpace: 'pre',
            background: theme.panel
          }}
        >
          {hasCode ? code : '// Nenhum código BIP gerado.'}
        </div>
        <button
          type="button"
          onClick={handleCopy}
          disabled={!hasCode}
          aria-label="Copiar código BIP"
          style={{
            position: 'absolute',
            left: 16,
            bottom: 16,
            display: 'flex',
            alignItems: 'center',
            gap: 6,
            padding: '8px 12px',
            borderRadius: 6,
            border: '1px solid #3f3f46',
            background: hasCode ? '#27272a' : '#1f1f23',
            color: hasCode ? '#e4e4e7' : '#71717a',
            cursor: hasCode ? 'pointer' : 'not-allowed',
            fontSize: 12,
            fontWeight: 500,
            transition: 'background 0.2s ease, color 0.2s ease, transform 0.15s ease'
          }}
          onMouseEnter={(e) => {
            if (!hasCode) return
            e.currentTarget.style.background = '#323238'
          }}
          onMouseLeave={(e) => {
            if (!hasCode) return
            e.currentTarget.style.background = '#27272a'
          }}
          onMouseDown={(e) => {
            if (!hasCode) return
            e.currentTarget.style.transform = 'scale(0.96)'
          }}
          onMouseUp={(e) => {
            if (!hasCode) return
            e.currentTarget.style.transform = 'scale(1)'
          }}
        >
          <svg
            width="16"
            height="16"
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            strokeWidth="2"
            strokeLinecap="round"
            strokeLinejoin="round"
          >
            <rect x="9" y="9" width="13" height="13" rx="2" ry="2"></rect>
            <path d="M5 15H4a2 2 0 0 1-2-2V4a2 2 0 0 1 2-2h9a2 2 0 0 1 2 2v1"></path>
          </svg>
          Copiar
        </button>
      </div>
      {copied && (
        <div
          style={{
            position: 'absolute',
            right: 16,
            bottom: 72,
            padding: '10px 18px',
            borderRadius: 12,
            backdropFilter: 'blur(16px)',
            background: 'rgba(24, 24, 27, 0.78)',
            color: '#bbf7d0',
            fontSize: 13,
            fontWeight: 600,
            border: '1px solid rgba(52, 211, 153, 0.35)',
            boxShadow: '0 10px 28px rgba(15, 118, 110, 0.22)',
            animation: 'bipFadeSlideIn 0.25s ease'
          }}
        >
          Copiado para área de transferência
        </div>
      )}
    </div>
  )
}
