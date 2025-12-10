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
3. Acessar:
   - Abrir `http://localhost:5173` no navegador.


---

## Codigos para testar:

```
var a: int = 0;

for(var i: int = 0; i < 10; i++){
   for(var j: int = 0; j < 5; j++){
      a = a + i + j;
      print(a);
   } 
}
```

```
const a: int = 5; 
const b: int[] = [1,2,3,4];

var c: int;

if(a <= b[2]){
    c = a + b[1];
} else {
    c = b[0] + a;
}
```

```
function soma(a: int, b: int): int {
    return a + b;
}

const c: int = soma(1, 2);
// const d: int = soma(1, 2, 3);  esse tem que dar erro por passar 3 parametros
```
