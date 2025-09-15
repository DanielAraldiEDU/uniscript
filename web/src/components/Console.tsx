import { useEffect, useRef } from 'react'

export type LogItem = { time: string; text: string; color: string }

export function Console({ logs }: { logs: LogItem[] }) {
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

