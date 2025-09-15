import { useEffect, useMemo, useRef, useState } from 'react'
import { Editor } from '@monaco-editor/react'
import type * as MonacoNS from 'monaco-editor'
import { compileSource, posToLineCol } from './wasm/uniscript'

type LogItem = { time: string; text: string; color: string }

export default function App() {
  const [code, setCode] = useState<string>('while (x < 10){read(x);}\n')
  const [logs, setLogs] = useState<LogItem[]>([])
  const [cursor, setCursor] = useState({ line: 1, col: 1 })
  const monacoRef = useRef<typeof MonacoNS | null>(null)
  const modelRef = useRef<MonacoNS.editor.ITextModel | null>(null)

  const theme = useMemo(() => ({
    bg: '#09090b',
    panel: '#18181b',
    text: '#e4e4e7',
    subtle: '#a1a1aa',
    button: '#27272a',
    border: '#3f3f46',
    blue: '#60a5fa',
    green: '#34d399',
    red: '#f87171',
  }), [])

  function addLog(text: string, color = theme.subtle) {
    const now = new Date()
    const hh = String(now.getHours()).padStart(2, '0')
    const mm = String(now.getMinutes()).padStart(2, '0')
    const ss = String(now.getSeconds()).padStart(2, '0')
    setLogs((l) => [...l, { time: `[${hh}:${mm}:${ss}]`, text, color }])
  }

  function clearMarkers() {
    const monaco = monacoRef.current
    const model = modelRef.current
    if (!monaco || !model) return
    // Remover completamente os markers/underline de erro
    monaco.editor.setModelMarkers(model, 'uniscript', [])
  }

  async function handleCompile() {
    clearMarkers()
    setLogs([])
    addLog('Iniciando análise sintática…', theme.blue)
    const src = code.trim()
    if (!src) {
      addLog('Nenhum código para compilar.', theme.red)
      return
    }
    try {
      const result = await compileSource(code)
      if (result.ok) {
        addLog('Análise concluída com sucesso!', theme.green)
        return
      }
      const pos = result.pos ?? -1
      const { line, col } = posToLineCol(code, pos)
      const msg = summaryMessage(result.kind, line, col)
      addLog(msg, theme.red)
    } catch (e: any) {
      addLog(`Erro desconhecido durante a compilação. ${String(e?.message ?? e)}`, theme.red)
    }
  }

  function capitalize(s: string) {
    return s ? s.charAt(0).toUpperCase() + s.slice(1) : s
  }

  function summaryMessage(kind: string | undefined, line: number, col: number) {
    const suffix = (line > 0 && col > 0) ? ` (linha ${line}, coluna ${col}) ` : ''
    switch (kind) {
      case 'lexical':
        return `: ERROR: Ocorreu um erro léxico${suffix}`
      case 'semantic':
        return `: ERROR: Ocorreu um erro semântico${suffix}`
      case 'syntactic':
      default:
        return `: ERROR: Ocorreu um erro na sintaxe ${suffix}`
    }
  }

  return (
    <div style={{ display: 'flex', flexDirection: 'column', height: '100vh', background: theme.bg, color: theme.text }}>
      <header style={{ display: 'flex', alignItems: 'center', gap: 12, background: theme.panel, padding: 10, borderBottom: `1px solid ${theme.border}` }}>
        <div style={{ fontSize: 16, fontWeight: 700 }}>UniScript</div>
        <div style={{ flex: 1 }} />
        <button onClick={handleCompile} style={{ color: theme.text, background: theme.button, border: `1px solid ${theme.border}`, borderRadius: 8, padding: '8px 14px' }}>Compilar</button>
      </header>

      <div style={{ flex: 1, display: 'flex', flexDirection: 'column' }}>
        <div style={{ flex: 3, minHeight: 0 }}>
          <Editor
            height="100%"
            defaultLanguage="plaintext"
            value={code}
            onMount={(editor, monaco) => {
              monacoRef.current = monaco as unknown as typeof MonacoNS
              modelRef.current = editor.getModel()
              editor.updateOptions({ fontSize: 14, minimap: { enabled: false }, wordWrap: 'off' })
              monaco.editor.defineTheme('uniscript-dark', {
                base: 'vs-dark', inherit: true, rules: [], colors: {
                  'editor.background': theme.bg,
                }
              })
              monaco.editor.setTheme('uniscript-dark')
              editor.onDidChangeCursorPosition((e) => {
                setCursor({ line: e.position.lineNumber, col: e.position.column })
              })
            }}
            onChange={(val) => setCode(val ?? '')}
            options={{ padding: { top: 8, bottom: 8 } }}
          />
        </div>
        <div style={{ flex: 1, minHeight: 0, borderTop: `1px solid ${theme.border}` }}>
          <Console logs={logs} />
        </div>
      </div>

      <footer style={{ background: theme.panel, borderTop: `1px solid ${theme.border}`, color: '#a1a1aa', padding: '6px 10px', fontSize: 12 }}>
        Linha {cursor.line}, Coluna {cursor.col}
      </footer>
    </div>
  )
}

function Console({ logs }: { logs: LogItem[] }) {
  const ref = useRef<HTMLDivElement | null>(null)
  useEffect(() => {
    ref.current?.scrollTo({ top: ref.current.scrollHeight })
  }, [logs.length])
  return (
    <div ref={ref} style={{ height: '100%', overflow: 'auto', fontFamily: 'ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, \"Liberation Mono\", \"Courier New\", monospace', fontSize: 14, padding: 8 }}>
      {logs.map((l, i) => (
        <div key={i}>
          <span style={{ color: '#a1a1aa' }}>{l.time} </span>
          <span style={{ color: l.color }}>{l.text}</span>
        </div>
      ))}
    </div>
  )
}
