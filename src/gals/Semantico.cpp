#include <iostream>
#include <string>
#include <vector>
#include <cctype>

#include "Semantico.h"
#include "Constants.h"
#include "../SemanticTable.cpp"

using namespace std;

SemanticTable semanticTable;

bool Semantico::isTypeParameter = false;
Semantico::Variable Semantico::currentVariable = {"", Semantico::Type::NULLABLE, {}, -1, false, false, false, false, false, false};
vector<Semantico::Variable> Semantico::currentParameters = {};


static SemanticTable::Types inferLiteralType(const std::string& lex) {
  if (lex == "true" || lex == "false") return SemanticTable::BOOLEAN;
  if (!lex.empty() && lex.front() == '"' && lex.back() == '"') return SemanticTable::STRING;
  if (lex.empty()) return SemanticTable::STRING;

  std::size_t pos = 0;
  bool hasDigit = false;
  bool hasDot = false;

  if (lex[pos] == '+' || lex[pos] == '-') ++pos;
  for (; pos < lex.size(); ++pos) {
    const unsigned char c = static_cast<unsigned char>(lex[pos]);
    if (std::isdigit(c)) {
      hasDigit = true;
      continue;
    }
    if (c == '.' && !hasDot) {
      hasDot = true;
      continue;
    }
    return SemanticTable::STRING;
  }

  if (!hasDigit) return SemanticTable::STRING;
  
  return hasDot ? SemanticTable::FLOAT : SemanticTable::INT;
}

namespace {

void registrarLiteral(const Token* token) {
  if (!token || Semantico::currentVariable.isFunction) return;
  const string lexema = token->getLexeme();
  Semantico::currentVariable.value.push_back(lexema);
  Semantico::currentVariable.isInitialized = true;
  semanticTable.noteExprType(inferLiteralType(lexema));
}

void aplicarTipo(Semantico& semantico, const Token* token) {
  if (!token) return;
  const auto tipo = semantico.getTypeFromString(token->getLexeme());

  if (Semantico::isTypeParameter) {
    auto& parametro = Semantico::currentParameters.back();
    parametro.type = tipo;
    parametro.isUsed = false;
    Semantico::isTypeParameter = false;
    return;
  }

  Semantico::currentVariable.type = tipo;
  Semantico::currentVariable.isInitialized = false;

  if (Semantico::currentVariable.isFunction) {
    vector<SemanticTable::Param> parametros;
    parametros.reserve(Semantico::currentParameters.size());
    for (size_t i = 0; i < Semantico::currentParameters.size(); ++i) {
      const auto& parametro = Semantico::currentParameters[i];
      SemanticTable::Types tipoParametro = SemanticTable::INT;
      if (parametro.type != Semantico::Type::NULLABLE) {
        tipoParametro = static_cast<SemanticTable::Types>(parametro.type);
      }
      parametros.push_back({parametro.name, tipoParametro, static_cast<int>(i)});
    }
    SemanticTable::Types retorno = SemanticTable::INT;
    if (Semantico::currentVariable.type != Semantico::Type::NULLABLE) {
      retorno = static_cast<SemanticTable::Types>(Semantico::currentVariable.type);
    }
    semanticTable.beginFunction(Semantico::currentVariable.name, retorno, parametros);
    semantico.resetCurrentParameters();
    semantico.resetCurrentVariable();
  }
}

void registrarIdentificadorOuParametro(const Token* token) {
  if (!token) return;
  const string nome = token->getLexeme();

  if (Semantico::currentVariable.isFunction) {
    Semantico::currentParameters.push_back({nome, Semantico::Type::NULLABLE, {}, -1, false, false, true, false, false, false});
    Semantico::isTypeParameter = true;
  } else {
    Semantico::currentVariable.name = nome;
    Semantico::isTypeParameter = false;
  }
}

void executarLeitura() {
  string valor;
  if (!(cin >> valor)) return;
  Semantico::currentVariable.value.clear();
  Semantico::currentVariable.value.push_back(valor);
  semanticTable.noteExprType(SemanticTable::INT);
}

void finalizarInstrucao(Semantico& semantico) {
  SemanticTable::SymbolEntry entrada;
  entrada.name        = Semantico::currentVariable.name;
  const bool possuiTipo = Semantico::currentVariable.type != Semantico::Type::NULLABLE;
  entrada.type        = possuiTipo ? static_cast<SemanticTable::Types>(Semantico::currentVariable.type)
                                   : SemanticTable::INT;
  entrada.initialized = Semantico::currentVariable.isInitialized;
  entrada.used        = Semantico::currentVariable.isUsed;
  entrada.scope       = -1;
  entrada.isParameter = Semantico::currentVariable.isParameter;
  entrada.position    = -1;
  entrada.isArray     = Semantico::currentVariable.isArray;
  entrada.isFunction  = false;
  entrada.isConstant  = Semantico::currentVariable.isConstant;
  entrada.hasExplicitType = possuiTipo;

  semanticTable.commitStatement(entrada);
  semantico.resetCurrentVariable();
}
}

void Semantico::resetCurrentVariable()
{
  Semantico::currentVariable = {"", Semantico::Type::NULLABLE, {}, -1, false, false, false, false, false, false};
  Semantico::isTypeParameter = false;
}

void Semantico::resetCurrentParameters()
{
  Semantico::currentParameters.clear();
  Semantico::isTypeParameter = false;
}

void Semantico::resetState()
{
  semanticTable.reset();
  resetCurrentVariable();
  resetCurrentParameters();
}

bool Semantico::isConstant(const string &variableName)
{
  return variableName == "const";
}

Semantico::Type Semantico::getTypeFromString(const string &typeString)
{
  if (typeString == "int")
    return Semantico::Type::INT;
  else if (typeString == "float")
    return Semantico::Type::FLOAT;
  else if (typeString == "string")
    return Semantico::Type::STRING;
  else if (typeString == "bool")
    return Semantico::Type::BOOLEAN;
  else if (typeString == "void")
    return Semantico::Type::VOID;
  else
    throw SemanticError("Tipo desconhecido: " + typeString);
}

void Semantico::executeAction(int action, const Token *token)
{
  switch (action)
  {
  case 1:
    // VALUE
    registrarLiteral(token);
    break;
  case 2:  // OR
  case 3:  // AND
  case 4:  // BIT OR
  case 5:  // EXPO
  case 6:  // BIT AND
  case 7:  // OP REL
  case 8:  // OP BITWISE
  case 9:  // ARIT LOWER
  case 10: // ARIT UPPER
  case 11: // NEG
  case 12: // LEFT PARENTHESIS
  case 13: // RIGHT PARENTHESIS
    break;
  case 14:
    // FUNCTION CALL
    if (token) {
      Semantico::currentVariable.name = token->getLexeme();
    }
    
    if (!Semantico::currentVariable.name.empty()) {
      semanticTable.markUseIfDeclared(Semantico::currentVariable.name);
    }
    Semantico::currentVariable.isUsed = true;
    break;
  case 15:
    // INDEXED VALUE
    Semantico::currentVariable.name = token->getLexeme();
    semanticTable.markUseIfDeclared(Semantico::currentVariable.name);
    Semantico::currentVariable.value.push_back(Semantico::currentVariable.name);
    break;
  case 16:
    // COMMENTS
    break;
  case 17:
    // PRINT
    for (const auto &value : Semantico::currentVariable.value)
    {
      if (!value.empty() && (std::isalpha(static_cast<unsigned char>(value.front())) || value.front() == '_'))
      {
        if (value != "true" && value != "false")
        {
          semanticTable.markUseIfDeclared(value);
        }
      }
      cout << value << " ";
    }
    Semantico::currentVariable.isUsed = true;
    break;
  case 18:
    // READ
    executarLeitura();
    break;
  case 19:
    // TYPE
    aplicarTipo(*this, token);
    break;
  case 20:
    // OPEN BRACKET
    Semantico::currentVariable.isArray = true;
    break;
  case 21:
    // CLOSE BRACKET
    break;
  case 22:
    // ATTRIBUTION
    registrarIdentificadorOuParametro(token);
    break;
  case 23:
    // FUNCTION DECLARATION
    Semantico::currentVariable.name = token->getLexeme();
    break;
  case 24:
    // VALUE INCREMENT/DECREMENT
    Semantico::currentVariable.name = token->getLexeme();
    semanticTable.markUseIfDeclared(Semantico::currentVariable.name);
    break;
  case 25:
    // CONST/VAR
    Semantico::currentVariable.isConstant = isConstant(token->getLexeme());
    break;
  case 26:
    // MULTI VAR DECLARATION
    break;
  case 27:
    // ATTRIBUTION INCREMENT/DECREMENT
    Semantico::currentVariable.isInitialized = true;
    Semantico::currentVariable.isUsed = true;
    break;
  case 28:
    // OPEN BRACKET INDEX
    break;
  case 29:
    // CLOSE BRACKET INDEX
    break;
  case 30:
    // FUNCTION TYPE
    Semantico::currentVariable.isFunction = true;
    break;
  case 31:
    // RETURN
    semanticTable.maybeCloseFunction();
    break;
  case 32:
    // BREAK
    break;
  case 33:
    // THROW
    break;
  case 34:
    // IF/ELIF/ELSE
    break;
  case 35:
    // DO
    break;
  case 36:
    // WHILE
    break;
  case 37:
    // FOR
    break;
  case 38:
    // FOR VARIABLE
    break;
  case 39:
    // FOR VARIABLE TYPE
    break;
  case 40:
    // FOR VALUE
    break;
  case 41:
    // SWITCH/CASE/DEFAULT
    break;
  case 42:
    // SEMICOLON
    finalizarInstrucao(*this);
    break;
  case 43:
    // FUNCTION FINAL
    break;
  case 99:
    // FINAL CODE
    semanticTable.closeAllScopes();
    semanticTable.printTable(std::cout);
    semanticTable.printDiagnostics(std::cout);
    break;
  default:
    cout << "Ação semântica desconhecida" << endl;
    break;
  }
}

void Semantico::printVariable(const Variable &variable)
{
  cout << "Nome da variável: " << variable.name << endl;
  cout << "Tipo: ";
  switch (variable.type)
  {
  case Semantico::Type::INT:
    cout << "int";
    break;
  case Semantico::Type::FLOAT:
    cout << "float";
    break;
  case Semantico::Type::STRING:
    cout << "string";
    break;
  case Semantico::Type::BOOLEAN:
    cout << "bool";
    break;
  case Semantico::Type::VOID:
    cout << "void";
    break;
  default:
    cout << "nullable";
    break;
  }
  cout << endl;

  cout << "Valor: ";
  for (const auto &valor : variable.value)
  {
    cout << valor << " ";
  }
  cout << endl;

  cout << "Escopo: " << variable.scope << endl;
  cout << "Inicializada: " << (variable.isInitialized ? "sim" : "não") << endl;
  cout << "Utilizada: " << (variable.isUsed ? "sim" : "não") << endl;
  cout << "Constante: " << (variable.isConstant ? "sim" : "não") << endl;
  cout << "Parâmetro: " << (variable.isParameter ? "sim" : "não") << endl;
  cout << "Função: " << (variable.isFunction ? "sim" : "não") << endl;
  cout << "Vetor: " << (variable.isArray ? "sim" : "não") << endl;
}

vector<ExportedSymbol> snapshotSymbolTable()
{
  vector<ExportedSymbol> exported;
  const auto &symbols = semanticTable.getSymbolTable();
  exported.reserve(symbols.size());

  for (const auto &entry : symbols)
  {
    ExportedSymbol symbol;
    symbol.name = entry.name;
    symbol.type = SemanticTable::typeToStr(entry.type);
    symbol.initialized = entry.initialized;
    symbol.used = entry.used;
    symbol.scope = entry.scope;
    symbol.isParameter = entry.isParameter;
    symbol.position = entry.position;
    symbol.isArray = entry.isArray;
    symbol.isFunction = entry.isFunction;
    symbol.isConstant = entry.isConstant;
    symbol.modality = entry.isFunction ? "func" : (entry.isParameter ? "param" : (entry.isArray ? "vetor" : "var"));
    exported.push_back(symbol);
  }

  return exported;
}

vector<ExportedDiagnostic> snapshotDiagnostics()
{
  vector<ExportedDiagnostic> exported;
  const auto &items = semanticTable.getDiagnostics();
  exported.reserve(items.size());

  for (const auto &diag : items)
  {
    exported.push_back({diag.severity, diag.message});
  }

  return exported;
}

void finalizeSemanticAnalysis()
{
  semanticTable.closeAllScopes();
}
