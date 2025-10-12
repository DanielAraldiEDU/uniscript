#include <iostream>
#include <string>
#include <vector>

#include "Semantico.h"
#include "Constants.h"

using namespace std;

bool Semantico::isTypeParameter = false;
Semantico::Variable Semantico::currentVariable = {"", Semantico::Type::NULLABLE, {}, -1, false, false, false, false, false, false};
vector<Semantico::Variable> Semantico::currentParameters = {};

void Semantico::resetCurrentVariable()
{
  Semantico::currentVariable = {"", Semantico::Type::NULLABLE, {}, -1, false, false, false, false, false, false};
}

void Semantico::resetCurrentParameters()
{
  Semantico::currentParameters.clear();
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
    throw SemanticError("Unknown type: " + typeString);
}

void Semantico::executeAction(int action, const Token *token)
{
  string currentValue;

  switch (action)
  {
  case 1:
    // VALUE
    if (!Semantico::currentVariable.isFunction)
    {
      Semantico::currentVariable.value.push_back(token->getLexeme());
      Semantico::currentVariable.isInitialized = true;
    }
    break;
  case 2:
    // OR
    break;
  case 3:
    // AND
    break;
  case 4:
    // BIT OR
    break;
  case 5:
    // EXPO
    break;
  case 6:
    // BIT AND
    break;
  case 7:
    // OP REL
    break;
  case 8:
    // OP BITWISE
    break;
  case 9:
    // ARIT LOWER
    break;
  case 10:
    // ARIT UPPER
    break;
  case 11:
    // NEG
    break;
  case 12:
    // LEFT PARENTHESIS
    break;
  case 13:
    // RIGHT PARENTHESIS
    break;
  case 14:
    // FUNCTION CALL
    Semantico::currentVariable.isUsed = true;
    Semantico::currentVariable.isFunction = true;
    break;
  case 15:
    // INDEXED VALUE
  case 16:
    // COMMENTS
    break;
  case 17:
    // PRINT
    for (const auto &value : Semantico::currentVariable.value)
    {
      cout << value << " ";
    }
    break;
  case 18:
    cin >> currentValue;

    Semantico::currentVariable.value.clear();
    Semantico::currentVariable.value.push_back(currentValue);
    // READ
    break;
  case 19:
    // TYPE
    if (Semantico::isTypeParameter)
    {
      Semantico::Variable currentParameter = Semantico::currentParameters.back();
      currentParameter.type = getTypeFromString(token->getLexeme());
      currentParameter.isUsed = false;
      Semantico::currentParameters[Semantico::currentParameters.size() - 1] = currentParameter;
      Semantico::isTypeParameter = false;
    }
    else
    {
      Semantico::currentVariable.type = getTypeFromString(token->getLexeme());
      Semantico::currentVariable.isInitialized = false;
      if (Semantico::currentVariable.isFunction)
      {
        Semantico::printVariable(Semantico::currentVariable);
      }
    }
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
    if (Semantico::currentVariable.isFunction)
    {
      Semantico::currentParameters.push_back({token->getLexeme(), Semantico::Type::NULLABLE, {}, -1, false, false, true, false, false});
      Semantico::isTypeParameter = true;
    }
    else
    {
      Semantico::currentVariable.name = token->getLexeme();
      Semantico::isTypeParameter = false;
    }
    break;
  case 23:
    // FUNCTION DECLARATION
    Semantico::currentVariable.name = token->getLexeme();
    break;
  case 24:
    // INCREMENT/DECREMENT
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
    Semantico::printVariable(Semantico::currentVariable);
    Semantico::resetCurrentVariable();
    break;
  case 99:
    // FINAL CODE
    break;
  default:
    cout << "Unknown semantic action" << endl;
    break;
  }
}

void Semantico::printVariable(const Variable &variable)
{
  cout << "Variable Name: " << variable.name << endl;
  cout << "Type: ";
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

  cout << "Value: ";
  for (const auto &value : variable.value)
  {
    cout << value << " ";
  }
  cout << endl;

  cout << "Scope: " << variable.scope << endl;
  cout << "Is Initialized: " << (variable.isInitialized ? "true" : "false") << endl;
  cout << "Is Used: " << (variable.isUsed ? "true" : "false") << endl;
  cout << "Is Constant: " << (variable.isConstant ? "true" : "false") << endl;
  cout << "Is Parameter: " << (variable.isParameter ? "true" : "false") << endl;
  cout << "Is Function: " << (variable.isFunction ? "true" : "false") << endl;
  cout << "Is Array: " << (variable.isArray ? "true" : "false") << endl;
}
