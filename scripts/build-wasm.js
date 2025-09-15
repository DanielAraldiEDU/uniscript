#!/usr/bin/env node
import { spawnSync } from 'node:child_process'
import { existsSync, mkdirSync } from 'node:fs'
import { resolve } from 'node:path'

const root = resolve(process.cwd())
const webPublic = resolve(root, 'web/public')

function run(cmd, args, opts = {}) {
  const res = spawnSync(cmd, args, { stdio: 'inherit', shell: false, ...opts })
  if (res.status !== 0) {
    throw new Error(`Command failed: ${cmd} ${args.join(' ')}`)
  }
}

function has(cmd) {
  const check = spawnSync(cmd, ['--version'], { stdio: 'ignore' })
  return check.status === 0
}

function ensureWebPublic() {
  if (!existsSync(webPublic)) mkdirSync(webPublic, { recursive: true })
}

async function main() {
  ensureWebPublic()

  if (has('emcc')) {
    console.log('[wasm] Using local Emscripten (emcc)')
    const cmd = `emcc src/gals/*.cpp bridge.cpp -O3 -s MODULARIZE=1 -s EXPORT_NAME=createUniscriptModule -s ENVIRONMENT=web -fwasm-exceptions -s EXPORTED_FUNCTIONS='["_uniscript_compile","_free"]' -s EXPORTED_RUNTIME_METHODS='["cwrap","UTF8ToString"]' -I src -o web/public/uniscript.js`
    const sh = spawnSync('bash', ['-lc', cmd], { stdio: 'inherit' })
    if (sh.status !== 0) process.exit(sh.status ?? 1)
    console.log('[wasm] Done: web/public/uniscript.js + web/public/uniscript.wasm')
    return
  }

  if (!has('docker')) {
    console.error("[wasm] Neither 'emcc' nor 'docker' found. Install Emscripten or Docker.")
    process.exit(1)
  }

  console.log('[wasm] emcc not found. Falling back to Docker build...')
  run('docker', ['build', '-f', 'docker/Dockerfile', '-t', 'uniscript-build', '--target', 'artifacts', '.'])
  const create = spawnSync('docker', ['create', 'uniscript-build', '/bin/true'], { encoding: 'utf8' })
  if (create.status !== 0) process.exit(create.status ?? 1)
  const cid = (create.stdout || '').trim()
  if (!cid) {
    console.error('[wasm] Failed to create container for artifact copy')
    process.exit(1)
  }
  run('docker', ['cp', `${cid}:/out/.`, webPublic])
  spawnSync('docker', ['rm', cid], { stdio: 'ignore' })
  console.log('[wasm] Done: web/public/uniscript.js + web/public/uniscript.wasm')
}

main().catch((e) => {
  console.error(e?.message || e)
  process.exit(1)
})

