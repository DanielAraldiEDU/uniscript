import { Editor as MonacoEditor } from '@monaco-editor/react'
import type * as MonacoNS from 'monaco-editor'
import { theme } from '../theme'

type Props = {
  value: string
  onChange: (v: string) => void
  onCursor: (line: number, col: number) => void
  monacoRef: React.MutableRefObject<typeof MonacoNS | null>
  modelRef: React.MutableRefObject<MonacoNS.editor.ITextModel | null>
}

export function Editor({ value, onChange, onCursor, monacoRef, modelRef }: Props) {
  return (
    <MonacoEditor
      height="100%"
      defaultLanguage="plaintext"
      value={value}
      onMount={(editor, monaco) => {
        monacoRef.current = monaco as unknown as typeof MonacoNS
        modelRef.current = editor.getModel()
        editor.updateOptions({ fontSize: 14, minimap: { enabled: false }, wordWrap: 'off' })
        monaco.editor.defineTheme('uniscript-dark', {
          base: 'vs-dark', inherit: true, rules: [], colors: {
            'editor.background': theme.bg,
          }
        })
        monaco.editor.setTheme('uniscript-dark')
        editor.onDidChangeCursorPosition((e) => {
          onCursor(e.position.lineNumber, e.position.column)
        })
      }}
      onChange={(val) => onChange(val ?? '')}
      options={{ padding: { top: 8, bottom: 8 } }}
    />
  )
}

