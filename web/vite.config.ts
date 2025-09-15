import react from '@vitejs/plugin-react'
import { defineConfig } from 'vite'

const repo = process.env.GITHUB_REPOSITORY?.split('/')?.[1]
const isActions = !!process.env.GITHUB_ACTIONS
const isVercel = !!process.env.VERCEL
const base = isActions && !isVercel && repo ? `/${repo}/` : '/'

export default defineConfig({
  base,
  plugins: [react()],
  server: { port: 5173 }
})
