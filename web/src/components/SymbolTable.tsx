import type { SymbolInfo } from '../wasm/uniscript'
import { theme } from '../theme'

type Props = {
  symbols: SymbolInfo[]
}

const columns: Array<{ key: keyof SymbolInfo; label: string }> = [
  { key: 'identifier', label: 'Identificador' },
  { key: 'type', label: 'Tipo' },
  { key: 'modality', label: 'Modalidade' },
  { key: 'scope', label: 'Escopo' },
  { key: 'declaredLine', label: 'Linha decl.' },
  { key: 'initialized', label: 'Inicializado' },
  { key: 'used', label: 'Usado' }
]

export function SymbolTable({ symbols }: Props) {
  return (
    <div style={{ display: 'flex', flexDirection: 'column', height: '100%', background: theme.panel }}>
      <div style={{ padding: '8px 12px', borderBottom: `1px solid ${theme.border}`, fontWeight: 600, fontSize: 14 }}>
        Tabela de simbolos
      </div>
      <div style={{ flex: 1, overflow: 'auto' }}>
        <table style={{ width: '100%', borderCollapse: 'collapse', fontSize: 13 }}>
          <thead>
            <tr>
              {columns.map((column) => (
                <th
                  key={column.key}
                  style={{
                    textAlign: 'left',
                    padding: '8px 10px',
                    fontWeight: 500,
                    fontSize: 12,
                    color: theme.subtle,
                    position: 'sticky',
                    top: 0,
                    background: theme.panel,
                    borderBottom: `1px solid ${theme.border}`
                  }}
                >
                  {column.label}
                </th>
              ))}
            </tr>
          </thead>
          <tbody>
            {symbols.length === 0 ? (
              <tr>
                <td colSpan={columns.length} style={{ padding: '16px 10px', textAlign: 'center', color: theme.subtle }}>
                  Tabela vazia. Compile para visualizar os simbolos.
                </td>
              </tr>
            ) : (
              symbols.map((symbol, index) => (
                <tr
                  key={`${symbol.identifier}-${index}`}
                  style={{
                    background: index % 2 === 0 ? 'transparent' : '#141417',
                    borderBottom: `1px solid ${theme.border}`
                  }}
                >
                  {columns.map((column) => (
                    <td key={column.key} style={{ padding: '8px 10px', color: theme.text, whiteSpace: 'nowrap' }}>
                      {renderCell(column.key, symbol)}
                    </td>
                  ))}
                </tr>
              ))
            )}
          </tbody>
        </table>
      </div>
    </div>
  )
}

function renderCell(column: keyof SymbolInfo, symbol: SymbolInfo) {
  switch (column) {
    case 'declaredLine':
      return symbol.declaredLine > 0 ? symbol.declaredLine : '-'
    case 'initialized':
      return symbol.initialized ? 'Sim' : 'Nao'
    case 'used':
      return symbol.used ? 'Sim' : 'Nao'
    default: {
      const value = symbol[column]
      return typeof value === 'string' && value.trim().length > 0 ? value : '-'
    }
  }
}
