import react from '@vitejs/plugin-react'
import { defineConfig } from 'vite'

const repo = process.env.GITHUB_REPOSITORY?.split('/')?.[1]
const isActions = !!process.env.GITHUB_ACTIONS
const isVercel = !!process.env.VERCEL
const baseOverride = process.env.VITE_BASE?.trim()
const base = baseOverride || (isActions && !isVercel && repo ? `/${repo}/` : '/')

export default defineConfig({
  base,
  plugins: [react()],
  server: { port: 5173 }
})
