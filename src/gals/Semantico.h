#ifndef SEMANTICO_H
#define SEMANTICO_H

#include <string>
#include <vector>

#include "Token.h"
#include "SemanticError.h"
#include "SymbolInfo.h"

using namespace std;

class SemanticTable;

struct ExportedSymbol {
  std::string name;
  std::string type;
  bool initialized;
  bool used;
  int scope;
  bool isParameter;
  int position;
  int line;
  int column;
  bool isArray;
  bool isFunction;
  bool isConstant;
};

struct ExportedDiagnostic {
  std::string severity;
  std::string message;
  int position;
  int length;
};

class Semantico
{
public:
  enum Type
  {
    NULLABLE = -1,
    INT = 0,
    FLOAT,
    STRING,
    BOOLEAN,
    VOID
  };
  struct Variable
  {
    string name;
    Type type;
    vector<string> value;
    vector<int> valuePositions;
    vector<int> valueLengths;
    int scope;
    bool isInitialized;
    bool isUsed;
    bool isConstant;
    bool isParameter;
    bool isFunction;
    bool isArray;
    bool literalIsArray;
    int position;
    int line;
    int column;
  };

  static bool isTypeParameter;
  static Variable currentVariable;
  static vector<Variable> currentParameters;
  static std::string sourceCode;

  void resetCurrentVariable();
  void resetCurrentParameters();
  void resetState();
  void printVariable(const Variable &variable);
  void executeAction(int action, const Token *token);
  bool isConstant(const string &variableName);
  Type getTypeFromString(const string &typeString);
  void setSourceCode(const std::string &code);
  std::vector<SymbolInfo> symbolTable() const;
  void clearSymbolTable();
};

extern SemanticTable semanticTable;

std::vector<ExportedSymbol> snapshotSymbolTable();
std::vector<ExportedDiagnostic> snapshotDiagnostics();
void finalizeSemanticAnalysis();

#endif
