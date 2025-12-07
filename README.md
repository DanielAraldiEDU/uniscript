# Uniscript Web

Interface web da linguagem Uniscript, compilando o núcleo C++ para WebAssembly e servindo em uma aplicação React/Vite. Projeto criado na disciplina de Compiladores por [Daniel Sansão Araldi](https://github.com/DanielAraldi), [Rafael Mota Alves](https://github.com/RafaelMotaAlvess) e [Nilson Neto](https://github.com/NilsonAndradeNeto).

## Requisitos
- Node.js 18+
- Emscripten (`emcc`) instalado ou Docker (usado como fallback para gerar o WASM)

## Passo a passo
1. Preparar (gerar WASM + instalar deps do frontend):
   ```bash
   npm run setup
   ```
   - Usa `emcc` se estiver disponível; caso contrário, tenta Docker.
   - Gera `web/public/uniscript.js` e `web/public/uniscript.wasm`.
2. Ambiente de desenvolvimento (Vite):
   ```bash
   npm run dev
   ```