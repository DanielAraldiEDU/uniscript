#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "gals/Lexico.h"
#include "gals/Sintatico.h"
#include "gals/Semantico.h"

using namespace std;

int main (int argc, char* argv[]) {
  Lexico lex; 
  Sintatico sint;
  Semantico sem;

  // Tenta ler o arquivo informado na linha de comando ou usa prompt.txt.
  string filename = (argc > 1) ? argv[1] : "prompt.txt";
  ifstream file;
  file.open(filename);
  
  if (!file.is_open() && argc == 1) {
    string fallback = "../" + filename;
    file.open(fallback);
    if (file.is_open()) {
      filename = fallback;
    }
  }
  
  if (!file.is_open()) {
    cerr << "Erro ao abrir o arquivo: " << filename << endl;
    return 1;
  }
  
  stringstream buffer;
  buffer << file.rdbuf();
  string content = buffer.str();
  file.close();
  
  lex.setInput(content.c_str());
  sem.setSourceCode(content);
  // Não esquecer de remover o arquivo prompt.txt depois ----------------------

  try {
    sint.parse(&lex, &sem);
    finalizeSemanticAnalysis();
    cout << "Analise concluida com sucesso!" << endl;
  } catch (LexicalError err) {
    cerr << "Problema lexico: " << err.getMessage() << endl;
  } catch (SyntacticError err) {
    cerr << "Problema sintatico: " << err.getMessage() << endl;
  } catch (SemanticError err) {
    cerr << "Problema semantico: " << err.getMessage() << endl;
  }
}
