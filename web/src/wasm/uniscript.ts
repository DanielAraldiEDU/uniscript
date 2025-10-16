let modPromise: Promise<any> | null = null

export type CompileKind = 'lexical' | 'syntactic' | 'semantic' | 'unknown'

export interface SymbolInfo {
  name: string
  type: string
  initialized: boolean
  used: boolean
  scope: number
  isParameter: boolean
  position: number
  isArray: boolean
  isFunction: boolean
  isConstant: boolean
  modality: string
}

export interface DiagnosticInfo {
  severity: 'error' | 'warning' | 'info'
  message: string
}

export interface CompileResult {
  ok: boolean
  kind?: CompileKind
  message?: string
  pos?: number
  symbolTable: SymbolInfo[]
  diagnostics: DiagnosticInfo[]
}

export async function compileSource(code: string): Promise<CompileResult> {
  if (!modPromise) {
    modPromise = (async () => {
      const factory = (window as any).createUniscriptModule
      if (!factory) {
        throw new Error('WASM ausente. Gere web/public/uniscript.js com `make wasm` ou `make wasm-docker`.')
      }

      const base = import.meta.env.BASE_URL ?? '/'
      const baseUrl = new URL(base, window.location.origin).toString()

      return await factory({
        locateFile: (path: string) => new URL(path, baseUrl).toString()
      })
    })()
  }

  const Module = await modPromise
  const fn = Module.cwrap('uniscript_compile', 'number', ['string'])
  const ptr = fn(code)
  const json = Module.UTF8ToString(ptr)
  Module._free(ptr)
  const raw = JSON.parse(json)
  return normalizeCompileResult(raw)
}

function normalizeCompileResult(raw: any): CompileResult {
  const kind = typeof raw?.kind === 'string' && isCompileKind(raw.kind) ? raw.kind : undefined
  const message = typeof raw?.message === 'string' ? raw.message : undefined
  const pos = typeof raw?.pos === 'number' ? raw.pos : undefined
  const symbolTable = Array.isArray(raw?.symbolTable) ? raw.symbolTable.map(normalizeSymbolInfo) : []
  const diagnostics = Array.isArray(raw?.diagnostics) ? raw.diagnostics.map(normalizeDiagnosticInfo) : []

  return {
    ok: Boolean(raw?.ok),
    kind,
    message,
    pos,
    symbolTable,
    diagnostics
  }
}

function normalizeSymbolInfo(raw: any): SymbolInfo {
  const scopeValue = Number(raw?.scope)
  const positionValue = Number(raw?.position)
  return {
    name: typeof raw?.name === 'string' ? raw.name : '',
    type: typeof raw?.type === 'string' ? raw.type : '',
    initialized: Boolean(raw?.initialized),
    used: Boolean(raw?.used),
    scope: Number.isFinite(scopeValue) ? scopeValue : -1,
    isParameter: Boolean(raw?.isParameter),
    position: Number.isFinite(positionValue) ? positionValue : -1,
    isArray: Boolean(raw?.isArray),
    isFunction: Boolean(raw?.isFunction),
    isConstant: Boolean(raw?.isConstant),
    modality: typeof raw?.modality === 'string' ? raw.modality : ''
  }
}

function normalizeDiagnosticInfo(raw: any): DiagnosticInfo {
  const severity = typeof raw?.severity === 'string' ? raw.severity : 'info'
  return {
    severity: severity === 'error' || severity === 'warning' ? severity : 'info',
    message: typeof raw?.message === 'string' ? raw.message : ''
  }
}

function isCompileKind(value: any): value is CompileKind {
  return value === 'lexical' || value === 'syntactic' || value === 'semantic' || value === 'unknown'
}

export function posToLineCol(text: string, pos: number) {
  let line = 1, col = 1
  for (let i = 0; i < Math.max(0, pos) && i < text.length; i++) {
    if (text[i] === '\n') { line++; col = 1 } else { col++ }
  }
  return { line, col }
}
