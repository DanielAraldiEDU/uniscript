#!/usr/bin/env node
import { existsSync } from 'node:fs'
import { resolve } from 'node:path'
import { spawnSync } from 'node:child_process'

const js = resolve(process.cwd(), 'web/public/uniscript.js')
const wasm = resolve(process.cwd(), 'web/public/uniscript.wasm')

if (existsSync(js) && existsSync(wasm)) {
  process.exit(0)
}

console.log('[ensure-wasm] WASM missing. Running npm run wasm...')
const res = spawnSync('npm', ['run', 'wasm'], { stdio: 'inherit' })
process.exit(res.status ?? 1)

