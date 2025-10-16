import type * as MonacoNS from 'monaco-editor'
import { useEffect, useMemo, useRef, useState } from 'react'
import { Console as ConsoleComponent, type LogItem } from './components/Console'
import { Editor } from './components/Editor'
import { HeaderBar } from './components/HeaderBar'
import { StatusBar } from './components/StatusBar'
import { SymbolTable } from './components/SymbolTable'
import { theme } from './theme'
import { compileSource, posToLineCol, type CompileKind, type SymbolInfo } from './wasm/uniscript'

export default function App() {
  const [code, setCode] = useState<string>('print("Hello, World!");')
  const [logs, setLogs] = useState<LogItem[]>([])
  const [symbols, setSymbols] = useState<SymbolInfo[]>([])
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
    monaco.editor.setModelMarkers(model, 'uniscript', [])
  }

  async function handleCompile() {
    clearMarkers()
    setLogs([])
    addLog('Iniciando analise semantica...', theme.blue)
    const src = code.trim()
    if (!src) {
      addLog('Nenhum codigo para compilar.', theme.red)
      setSymbols([])
      return
    }
    try {
      const result = await compileSource(code)
      setSymbols(result.symbolTable)

      const diagnostics = result.diagnostics ?? []
      diagnostics.forEach((diag) => {
        const color = diag.severity === 'error' ? theme.red : diag.severity === 'warning' ? theme.yellow : theme.subtle
        const prefix = diag.severity === 'error' ? '[ERRO]' : diag.severity === 'warning' ? '[AVISO]' : '[INFO]'
        addLog(`${prefix} ${diag.message}`, color)
      })

      const hasDiagnosticErrors = diagnostics.some((diag) => diag.severity === 'error')
      const hasWarnings = diagnostics.some((diag) => diag.severity === 'warning')

      if (result.ok && !hasDiagnosticErrors) {
        if (hasWarnings) {
          addLog('Analise concluida com avisos.', theme.yellow)
        } else {
          addLog('Analise concluida com sucesso!', theme.green)
        }
        return
      }

      if (result.ok && hasDiagnosticErrors) {
        addLog('ERROR: Foram encontrados erros semanticos. Reveja os avisos acima.', theme.red)
        return
      }

      const pos = result.pos ?? -1
      const { line, col } = posToLineCol(code, pos)
      const msg = summaryMessage(result.kind, line, col)
      addLog(msg, theme.red)
    } catch (e: any) {
      setSymbols([])
      addLog(`Erro desconhecido durante a compilacao. ${String(e?.message ?? e)}`, theme.red)
    }
  }

  function summaryMessage(kind: CompileKind | undefined, line: number, col: number) {
    const suffix = line > 0 && col > 0 ? ` (linha ${line}, coluna ${col}) ` : ''
    switch (kind) {
      case 'lexical':
        return `ERROR: Ocorreu um erro lexico${suffix}`
      case 'semantic':
        return `ERROR: Ocorreu um erro semantico${suffix}`
      case 'syntactic':
      default:
        return `ERROR: Ocorreu um erro na sintaxe ${suffix}`
    }
  }

  return (
    <div style={{ display: 'flex', flexDirection: 'column', height: '100vh', background: theme.bg, color: theme.text }}>
      <HeaderBar onCompile={handleCompile} />

      <div style={{ flex: 1, display: 'flex', flexDirection: 'column', minHeight: 0 }}>
        <div style={{ flex: 3, minHeight: 0, display: 'flex' }}>
          <div style={{ flex: 1, minWidth: 0 }}>
            <Editor
              value={code}
              onChange={(v) => setCode(v)}
              onCursor={(line, col) => setCursor({ line, col })}
              monacoRef={monacoRef}
              modelRef={modelRef}
            />
          </div>
          <div style={{ flex: '0 0 340px', borderLeft: `1px solid ${theme.border}`, background: theme.panel }}>
            <SymbolTable symbols={symbols} />
          </div>
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
    <div ref={ref} style={{ height: '100%', overflow: 'auto', fontFamily: 'ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, "Liberation Mono", "Courier New", monospace', fontSize: 14, padding: 8 }}>
      {logs.map((l, i) => (
        <div key={i}>
          <span style={{ color: '#a1a1aa' }}>{l.time} </span>
          <span style={{ color: l.color }}>{l.text}</span>
        </div>
      ))}
    </div>
  )
}
