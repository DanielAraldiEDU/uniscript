import { theme } from '../theme'

type Props = {
  code: string
}

const headerBg = '#1f1f23'

export function BipViewer({ code }: Props) {
  const hasCode = code.trim().length > 0

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
          fontWeight: 600,
          fontSize: 15,
          color: theme.text
        }}
      >
        Código BIP
      </div>
      <div
        style={{
          flex: 1,
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
    </div>
  )
}
