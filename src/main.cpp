#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "gals/Lexico.h"
#include "gals/Sintatico.h"
#include "gals/Semantico.h"

using namespace std;

int main () {
  Lexico lex; 
  Sintatico sint;
  Semantico sem;

  // Temporalmente, o arquivo de entrada estara fixo --------------------------
  string filename = "../prompt.txt";
  ifstream file(filename);
  
  if (!file.is_open()) {
    cerr << "Erro ao abrir o arquivo: " << filename << endl;
    return 1;
  }
  
  stringstream buffer;
  buffer << file.rdbuf();
  string content = buffer.str();
  file.close();
  
  lex.setInput(content.c_str());
  // Não esquecer de remover o arquivo prompt.txt depois ----------------------

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