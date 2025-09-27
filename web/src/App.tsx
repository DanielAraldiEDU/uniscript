import type * as MonacoNS from 'monaco-editor'
import { useEffect, useMemo, useRef, useState } from 'react'
import { Console as ConsoleComponent, type LogItem } from './components/Console'
import { Editor } from './components/Editor'
import { HeaderBar } from './components/HeaderBar'
import { StatusBar } from './components/StatusBar'
import { theme } from './theme'
import { compileSource, posToLineCol } from './wasm/uniscript'

export default function App() {
  const [code, setCode] = useState<string>('print(\"Hello, World World!\");')
  const [logs, setLogs] = useState<LogItem[]>([])
  const [cursor, setCursor] = useState({ line: 1, col: 1 })
  const monacoRef = useRef<typeof MonacoNS | null>(null)
  const modelRef = useRef<MonacoNS.editor.ITextModel | null>(null)

  useMemo(() => theme, [])

  function addLog(text: string, color: string = theme.subtle) {
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
        return `ERROR: Ocorreu um erro léxico${suffix}`
      case 'semantic':
        return `ERROR: Ocorreu um erro semântico${suffix}`
      case 'syntactic':
      default:
        return `ERROR: Ocorreu um erro na sintaxe ${suffix}`
    }
  }

  return (
    <div style={{ display: 'flex', flexDirection: 'column', height: '100vh', background: theme.bg, color: theme.text }}>
      <HeaderBar onCompile={handleCompile} />

      <div style={{ flex: 1, display: 'flex', flexDirection: 'column' }}>
        <div style={{ flex: 3, minHeight: 0 }}>
          <Editor
            value={code}
            onChange={(v) => setCode(v)}
            onCursor={(line, col) => setCursor({ line, col })}
            monacoRef={monacoRef}
            modelRef={modelRef}
          />
        </div>
        <div style={{ flex: 1, minHeight: 0, borderTop: `1px solid ${theme.border}` }}>
          <ConsoleComponent logs={logs} />
        </div>
      </div>

      <StatusBar line={cursor.line} col={cursor.col} />
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
