import type { SymbolInfo } from '../wasm/uniscript';

type Props = {
  symbols: SymbolInfo[]
}

const columns: Array<{ key: keyof SymbolInfo; label: string }> = [
  { key: 'name', label: 'Nome' },
  { key: 'type', label: 'Tipo' },
  { key: 'isConstant', label: 'Mutabilidade' },
  { key: 'initialized', label: 'Inicializada' },
  { key: 'used', label: 'Usada' },
  { key: 'scope', label: 'Escopo' },
  { key: 'line', label: 'Posição' },
  { key: 'isParameter', label: 'Parâmetro' },
  { key: 'isArray', label: 'Vetor' },
  { key: 'isFunction', label: 'Função' }
]

const theme = {
  panel: '#18181b',
  border: '#27272a',
  text: '#e4e4e7',
  subtle: '#71717a',
  headerBg: '#1f1f23',
  rowEven: '#18181b',
  rowOdd: '#141417',
  numberCol: '#36363aff'
}

export default function SymbolTable({ symbols = [] }: Props) {
  const displaySymbols = symbols

  return (
    <div style={{ 
      display: 'flex', 
      flexDirection: 'column', 
      height: '100%', 
      background: theme.panel,
      borderRadius: '8px',
      border: `1px solid ${theme.border}`,
      overflow: 'hidden'
    }}>
      <div style={{ 
        padding: '12px 16px', 
        borderBottom: `1px solid ${theme.border}`, 
        background: theme.headerBg
      }}>
        <div style={{ fontWeight: 600, fontSize: 15, color: theme.text }}>
          Tabela de Símbolos
        </div>
      </div>

      <div style={{ flex: 1, overflow: 'auto', position: 'relative' }}>
        <table style={{ width: '100%', borderCollapse: 'collapse', fontSize: 13 }}>
          <thead>
            <tr>
              <th style={{
                position: 'sticky',
                left: 0,
                zIndex: 20,
                textAlign: 'center',
                padding: '8px 10px',
                fontWeight: 600,
                fontSize: 12,
                color: theme.subtle,
                background: theme.headerBg,
                borderBottom: `2px solid ${theme.border}`,
                borderRight: `1px solid ${theme.border}`,
                width: 50
              }}>
                #
              </th>

              {columns.map((column) => (
                <th
                  key={column.key}
                  style={{
                    textAlign: 'left',
                    padding: '8px 10px',
                    fontWeight: 600,
                    fontSize: 12,
                    color: theme.subtle,
                    position: 'sticky',
                    top: 0,
                    background: theme.headerBg,
                    borderBottom: `2px solid ${theme.border}`,
                    whiteSpace: 'normal'
                  }}
                >
                  {column.label}
                </th>
              ))}
            </tr>
          </thead>

          <tbody>
            {displaySymbols.length === 0 ? (
              <tr>
                <td 
                  colSpan={columns.length + 1} 
                  style={{ 
                    padding: '32px 16px', 
                    textAlign: 'center', 
                    color: theme.subtle,
                    fontSize: 13
                  }}
                >
                  Nenhum símbolo encontrado
                </td>
              </tr>
            ) : (
              displaySymbols.map((symbol, index) => (
                <tr
                  key={`${symbol.name}-${index}`}
                  style={{
                    background: index % 2 === 0 ? theme.rowEven : theme.rowOdd,
                    borderBottom: `1px solid ${theme.border}`
                  }}
                >
                  <td style={{
                    position: 'sticky',
                    left: 0,
                    zIndex: 10,
                    padding: '10px 12px',
                    textAlign: 'center',
                    fontSize: 12,
                    fontWeight: 600,
                    color: theme.subtle,
                    background: theme.numberCol,
                    borderRight: `1px solid ${theme.border}`
                  }}>
                    {index + 1}
                  </td>

                  {columns.map((column) => (
                  <td 
                    key={column.key} 
                    style={{ 
                      padding: '8px 10px', 
                      color: theme.text, 
                      whiteSpace: 'normal',
                      fontSize: 13
                    }}
                  >
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
    case 'name':
      return <span style={{ fontFamily: 'monospace', fontWeight: 500 }}>{symbol.name}</span>

    case 'type':
      return <span style={{ 
        display: 'inline-block',
        padding: '2px 8px',
        background: '#7c3aed20',
        color: '#a78bfa',
        borderRadius: 4,
        fontSize: 12,
        fontWeight: 500
      }}>{symbol.type}</span>

    case 'initialized':
    case 'used':
    case 'isParameter':
    case 'isArray':
    case 'isFunction':
      return symbol[column] ? (
        <span style={{ color: '#4ade80', fontWeight: 500 }}>sim</span>
      ) : (
        <span style={{ color: '#f87171', fontWeight: 500 }}>não</span>
      )

    case 'isConstant':
      return symbol.isConstant ? (
        <span style={{ 
          display: 'inline-block',
          padding: '2px 8px',
          background: '#3b82f620',
          color: '#60a5fa',
          borderRadius: 4,
          fontSize: 11,
          fontWeight: 600
        }}>CONST</span>
      ) : (
        <span style={{ 
          display: 'inline-block',
          padding: '2px 8px',
          background: '#f59e0b20',
          color: '#fbbf24',
          borderRadius: 4,
          fontSize: 11,
          fontWeight: 600
        }}>VAR</span>
      )

    case 'scope':
      return <span style={{ fontFamily: 'monospace', fontWeight: 500 }}>{symbol.scope >= 0 ? symbol.scope : '-'}</span>

    case 'line':
      return symbol.line > 0 ? (
        <span style={{ fontFamily: 'monospace', fontWeight: 500 }}>{symbol.line}:{symbol.column > 0 ? symbol.column : 1}</span>
      ) : (
        <span style={{ fontFamily: 'monospace', fontWeight: 500 }}>-</span>
      )
    case 'column':
    case 'position':
      return symbol[column] ?? '-'

    default:
      return symbol[column] ?? '-'
  }
}



export { SymbolTable };
