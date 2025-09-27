let modPromise: Promise<any> | null = null

export type CompileKind = 'lexical' | 'syntactic' | 'semantic' | 'unknown'

export interface SymbolInfo {
  identifier: string
  type: string
  modality: string
  scope: string
  declaredLine: number
  initialized: boolean
  used: boolean
}

export interface CompileResult {
  ok: boolean
  kind?: CompileKind
  message?: string
  pos?: number
  symbolTable: SymbolInfo[]
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

  return {
    ok: Boolean(raw?.ok),
    kind,
    message,
    pos,
    symbolTable
  }
}

function normalizeSymbolInfo(raw: any): SymbolInfo {
  return {
    identifier: typeof raw?.identifier === 'string' ? raw.identifier : '',
    type: typeof raw?.type === 'string' ? raw.type : '',
    modality: typeof raw?.modality === 'string' ? raw.modality : '',
    scope: typeof raw?.scope === 'string' ? raw.scope : '',
    declaredLine: typeof raw?.declaredLine === 'number' ? raw.declaredLine : -1,
    initialized: Boolean(raw?.initialized),
    used: Boolean(raw?.used)
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
