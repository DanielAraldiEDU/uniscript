# Uniscript

Uma linguagem de programação simples, prática e única. Essa linguagem de programação surge na matéria de compiladores, criada pelos alunos [Daniel Sansão Araldi](https://github.com/DanielAraldi), [Rafael Mota Alves](https://github.com/RafaelMotaAlvess) e [Nilson Neto](https://github.com/NilsonAndradeNeto).

## Como executar

- CLI rápida:
  - Compilar: `make`
  - Rodar: `make run`
  - Script direto (compila e executa): `./scripts/compile-main.sh [args...]`

## IDE Web (React + WASM)

Interface web client-side em `web/`, compilando o núcleo C++ para WebAssembly.

Uso via npm:

- Preparar (WASM + deps): `npm run setup`
- Desenvolvimento: `npm run dev`
- Build produção: `npm run build`
- Preview: `npm run preview`


#### Problemas

- [ ] Não ta impedindo criação de variavel com o mesmo nome.
- [ ] Tem que encerrar a execução caso o erro seja encontrado? ou comentar todos os warnings quero ideias. 
- [ ] implementar os bereguenaite de for, acho que precisa, boa noite.
