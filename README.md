# Uniscript

Uma linguagem de programação simples, prática e única. Essa linguagem de programação surge na matéria de compiladores, criada pelos alunos [Daniel Sansão Araldi](https://github.com/DanielAraldi), [Rafael Mota Alves](https://github.com/RafaelMotaAlvess) e [Nilson Neto](https://github.com/NilsonAndradeNeto).

## Como executar

- CLI (console):
  - Compilar: `make`
  - Rodar: `make run`

- GUI (IDE):
  - Pré‑requisitos: Qt6 (ou Qt5) e `pkg-config` instalados.
  - Compilar: `make gui`
  - Rodar: `make run-gui`

A GUI fica em `src/gui` e oferece:
- Editor de código
- Botão/ação “Compilar” para rodar a análise sintática.
- Console de mensagens para erros e depuração.

## Web (React + WASM)

Interface web client-side em `web/`, compilando o núcleo C++ para WebAssembly.

Uso via npm:
- Gerar WASM e instalar deps: `npm run setup`
- Desenvolvimento: `npm run dev`
- Build produção: `npm run build`
- Preview: `npm run preview`


## AJustes do professor

1. problema com read lendo valores não não literais. deveria ler apenas variaveis e vetores.
2. problema na incrementação em vetores  vetor[i]++
3. colocar para que quando abra um PR, que suba um github-pages com as alterações.