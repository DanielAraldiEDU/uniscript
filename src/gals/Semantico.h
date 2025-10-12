#ifndef SEMANTICO_H
#define SEMANTICO_H

#include <string>
#include <vector>

#include "Token.h"
#include "SemanticError.h"

using namespace std;

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
    int scope;
    bool isInitialized;
    bool isConstant;
    bool isParameter;
    bool isFunction;
    bool isArray;
  };

  static Variable currentVariable;

  void resetCurrentVariable();
  void executeAction(int action, const Token *token);
  bool isConstant(const string &variableName);
  Type getTypeFromString(const string &typeString);
};

#endif
