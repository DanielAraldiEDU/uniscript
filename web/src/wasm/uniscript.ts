let modPromise: Promise<any> | null = null

export async function compileSource(code: string) {
  if (!modPromise) {
    modPromise = (async () => {
      const factory = (window as any).createUniscriptModule
      if (!factory) {
        throw new Error('WASM ausente. Gere web/public/uniscript.js com `make wasm` ou `make wasm-docker`.')
      }
      return await factory()
    })()
  }
  const Module = await modPromise
  const fn = Module.cwrap('uniscript_compile', 'number', ['string'])
  const ptr = fn(code)
  const json = Module.UTF8ToString(ptr)
  Module._free(ptr)
  return JSON.parse(json) as {
    ok: boolean
    kind?: 'lexical' | 'syntactic' | 'semantic' | 'unknown'
    message?: string
    pos?: number
  }
}

export function posToLineCol(text: string, pos: number) {
  let line = 1, col = 1
  for (let i = 0; i < Math.max(0, pos) && i < text.length; i++) {
    if (text[i] === '\n') { line++; col = 1 } else { col++ }
  }
  return { line, col }
}
