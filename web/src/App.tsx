import type * as MonacoNS from 'monaco-editor'
import { useEffect, useMemo, useRef, useState, type MutableRefObject } from 'react'
import { Console as ConsoleComponent, type LogItem } from './components/Console'
import { Editor } from './components/Editor'
import { HeaderBar } from './components/HeaderBar'
import { StatusBar } from './components/StatusBar'
import { SymbolTable } from './components/SymbolTable'
import { BipViewer } from './components/BipViewer'
import { theme } from './theme'
import { compileSource, posToLineCol, type CompileKind, type SymbolInfo, type DiagnosticInfo, type CompileResult } from './wasm/uniscript'

export default function App() {
  const [code, setCode] = useState<string>('print("Hello, World!");')
  const [logs, setLogs] = useState<LogItem[]>([])
  const [symbols, setSymbols] = useState<SymbolInfo[]>([])
  const [bipCode, setBipCode] = useState<string>('')
  const [sidePanelTab, setSidePanelTab] = useState<'symbols' | 'bip'>('symbols')
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
    setBipCode('')
    addLog('Iniciando analise semantica...', theme.blue)
    const src = code.trim()
    if (!src) {
      addLog('Nenhum codigo para compilar.', theme.red)
      setSymbols([])
      setBipCode('')
      return
    }
    try {
      const result = await compileSource(code)
      setSymbols(result.symbolTable)
      setBipCode(result.ok ? result.bipCode : '')

      const diagnostics = result.diagnostics ?? []
      diagnostics.forEach((diag) => {
        const color = diag.severity === 'error' ? theme.red : diag.severity === 'warning' ? theme.yellow : theme.subtle
        const prefix = diag.severity === 'error' ? '[ERRO]' : diag.severity === 'warning' ? '[AVISO]' : '[INFO]'
        const loc = diag.position >= 0 ? (() => {
          const { line, col } = posToLineCol(code, diag.position)
          return ` (linha ${line}, coluna ${col})`
        })() : ''
        addLog(`${prefix} ${diag.message}${loc}`, color)
      })

      applyMarkers(code, diagnostics, result, monacoRef, modelRef)

      const hasDiagnosticErrors = diagnostics.some((diag) => diag.severity === 'error')
      const hasWarnings = diagnostics.some((diag) => diag.severity === 'warning')

      if (!result.ok) {
        if (!hasDiagnosticErrors) {
          const pos = result.pos ?? -1
          const { line, col } = posToLineCol(code, pos)
          const msg = summaryMessage(result.kind, result.message, line, col)
          addLog(msg, theme.red)
        }
        return
      }

      if (!hasDiagnosticErrors) {
        if (hasWarnings) {
          addLog('Analise concluida com avisos.', theme.yellow)
        } else {
          addLog('Analise concluida com sucesso!', theme.green)
        }
        return
      }

      addLog('ERROR: Foram encontrados erros semanticos. Reveja os avisos acima.', theme.red)
    } catch (e: any) {
      setSymbols([])
      addLog(`Erro desconhecido durante a compilacao. ${String(e?.message ?? e)}`, theme.red)
    }
  }

  function summaryMessage(kind: CompileKind | undefined, message: string | undefined, line: number, col: number) {
    const suffix = line > 0 && col > 0 ? ` (linha ${line}, coluna ${col})` : ''
    const detail = message && message.trim().length > 0 ? `: ${message.trim()}` : ''
    switch (kind) {
      case 'lexical':
        return `ERROR${detail || ': Ocorreu um erro lexico'}${suffix}`
      case 'semantic':
        return `ERROR${detail || ': Ocorreu um erro semantico'}${suffix}`
      case 'syntactic':
      default:
        return `ERROR${detail || ': Ocorreu um erro na sintaxe'}${suffix}`
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
          <div style={{ flex: '0 0 880px', borderLeft: `1px solid ${theme.border}`, background: theme.panel, display: 'flex', flexDirection: 'column', minWidth: 880 }}>
            <div style={{ display: 'flex', borderBottom: `1px solid ${theme.border}` }}>
              <button
                type="button"
                onClick={() => setSidePanelTab('symbols')}
                style={{
                  flex: 1,
                  padding: '10px 12px',
                  background: sidePanelTab === 'symbols' ? '#27272a' : 'transparent',
                  border: 'none',
                  color: theme.text,
                  cursor: 'pointer',
                  fontWeight: sidePanelTab === 'symbols' ? 600 : 500
                }}
              >
                Tabela
              </button>
              <button
                type="button"
                onClick={() => setSidePanelTab('bip')}
                style={{
                  flex: 1,
                  padding: '10px 12px',
                  background: sidePanelTab === 'bip' ? '#27272a' : 'transparent',
                  border: 'none',
                  color: theme.text,
                  cursor: 'pointer',
                  fontWeight: sidePanelTab === 'bip' ? 600 : 500
                }}
              >
                Código BIP
              </button>
            </div>
            <div style={{ flex: 1, padding: 12 }}>
              {sidePanelTab === 'symbols' ? <SymbolTable symbols={symbols} /> : <BipViewer code={bipCode} />}
            </div>
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

function rangeFromOffset(code: string, pos: number, length: number) {
  if (pos < 0) return null
  const safeLength = Math.max(1, length)
  const start = posToLineCol(code, pos)
  const end = posToLineCol(code, pos + safeLength)
  return {
    startLineNumber: start.line,
    startColumn: start.col,
    endLineNumber: end.line,
    endColumn: end.col
  }
}

function applyMarkers(code: string, diagnostics: DiagnosticInfo[], result: CompileResult, monacoRef: MutableRefObject<typeof MonacoNS | null>, modelRef: MutableRefObject<MonacoNS.editor.ITextModel | null>) {
  const monaco = monacoRef.current
  const model = modelRef.current
  if (!monaco || !model) return

  const markers: MonacoNS.editor.IMarkerData[] = []

  diagnostics.forEach((diag) => {
    if (diag.position < 0) return
    const range = rangeFromOffset(code, diag.position, diag.length)
    if (!range) return
    markers.push({
      severity: diag.severity === 'error' ? monaco.MarkerSeverity.Error
        : diag.severity === 'warning' ? monaco.MarkerSeverity.Warning
        : monaco.MarkerSeverity.Info,
      message: diag.message,
      source: 'uniscript',
      ...range
    })
  })

  if (!result.ok && result.pos !== undefined && result.pos !== null && result.pos >= 0) {
    const hasPrimaryMarker = diagnostics.some((diag) => diag.severity === 'error' && diag.position === result.pos)
    if (!hasPrimaryMarker) {
      const range = rangeFromOffset(code, result.pos, result.length ?? 1)
      if (range) {
        markers.push({
          severity: monaco.MarkerSeverity.Error,
          message: result.message ?? 'Erro',
          source: 'uniscript',
          ...range
        })
      }
    }
  }

  monaco.editor.setModelMarkers(model, 'uniscript', markers)
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
