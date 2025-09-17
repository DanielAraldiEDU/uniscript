#include <iostream>
#include "gals/Lexico.h"
#include "gals/Sintatico.h"
#include "gals/Semantico.h"

using namespace std;

int main () {
  Lexico lex; 
  Sintatico sint;
  Semantico sem;

  lex.setInput("print(\"Hello, World!\");");

  try {
    sint.parse(&lex, &sem);
    cout << "Analise concluida com sucesso!" << endl;
  } catch (LexicalError err) {
    cerr << "Problema lexico: " << err.getMessage() << endl;
  } catch (SyntacticError err) {
    cerr << "Problema sintatico: " << err.getMessage() << endl;
  } catch (SemanticError err) {
    cerr << "Problema semantico: " << err.getMessage() << endl;
  }
}