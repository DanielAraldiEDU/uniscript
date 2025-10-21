#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <utility>
#include <optional>

#include "Semantico.h"
#include "Constants.h"
#include "../SemanticTable.cpp"

using namespace std;

#define SEMANTIC_DEBUG 0

SemanticTable semanticTable;

bool Semantico::isTypeParameter = false;
Semantico::Variable Semantico::currentVariable = {"", Semantico::Type::NULLABLE, {}, {}, {}, -1, false, false, false, false, false, false, false, -1, -1, -1};
vector<Semantico::Variable> Semantico::currentParameters = {};
std::string Semantico::sourceCode = "";

static SemanticTable::Types inferLiteralType(const std::string &lex)
{
  if (lex == "true" || lex == "false")
    return SemanticTable::BOOLEAN;
  if (!lex.empty() && lex.front() == '"' && lex.back() == '"')
    return SemanticTable::STRING;
  if (!lex.empty())
  {
    const unsigned char first = static_cast<unsigned char>(lex.front());
    if (std::isalpha(first) || first == '_')
    {
      return semanticTable.getSymbolType(lex);
    }
  }
  if (lex.empty())
    return SemanticTable::STRING;

  std::size_t pos = 0;
  bool hasDigit = false;
  bool hasDot = false;

  if (lex[pos] == '+' || lex[pos] == '-')
    ++pos;
  for (; pos < lex.size(); ++pos)
  {
    const unsigned char c = static_cast<unsigned char>(lex[pos]);
    if (std::isdigit(c))
    {
      hasDigit = true;
      continue;
    }
    if (c == '.' && !hasDot)
    {
      hasDot = true;
      continue;
    }
    return SemanticTable::STRING;
  }

  if (!hasDigit)
    return SemanticTable::STRING;

  return hasDot ? SemanticTable::FLOAT : SemanticTable::INT;
}

namespace
{

  void finalizarInstrucao(Semantico &semantico);

  enum class ScopeKind
  {
    IfBranch,
    WhileLoop,
    DoLoop,
    ForLoop,
    SwitchRoot,
    CaseBranch
  };

  enum class ForHeaderPhase
  {
    Init,
    Condition,
    Update,
    Body
  };

  struct ForHeaderState
  {
    ForHeaderPhase phase = ForHeaderPhase::Init;
    int parenthesisDepth = 0;
    bool initializerCommitted = false;
  };

  static vector<ScopeKind> activeScopes;
  static vector<ForHeaderState> forHeaderStates;
  static bool waitingDoWhileCondition = false;
  struct ArrayLiteralState
  {
    SemanticTable::Types declaredType = SemanticTable::INT;
    bool hasDeclaredType = false;
    SemanticTable::Types elementType = SemanticTable::INT;
    bool hasElementType = false;
  };
  static vector<ArrayLiteralState> arrayLiteralStates;

  enum class OperatorKind
  {
    LogicalOr,
    LogicalAnd,
    BitwiseOr,
    BitwiseAnd,
    BitwiseXor,
    ShiftLeft,
    ShiftRight,
    Add,
    Subtract,
    Multiply,
    Divide,
    Modulo,
    Power,
    RelationalCompare,
    RelationalEquality
  };

  enum class UnaryKind
  {
    LogicalNot,
    ArithmeticNeg,
    BitwiseNot
  };

  struct PendingOperator
  {
    OperatorKind kind = OperatorKind::Add;
    int position = -1;
    int length = 1;
    std::string lexeme;
  };

  struct PendingUnary
  {
    UnaryKind kind = UnaryKind::LogicalNot;
    int position = -1;
    int length = 1;
    std::string lexeme;
  };

  struct ExpressionContext
  {
    bool hasAccumulated = false;
    SemanticTable::Types accumulatedType = SemanticTable::INT;
    std::optional<PendingOperator> pendingOperator;
    std::vector<PendingUnary> pendingUnary;
  };

  static std::vector<ExpressionContext> expressionStack;

  ExpressionContext &ensureExpressionContext()
  {
    if (expressionStack.empty())
    {
      expressionStack.push_back({});
    }
    return expressionStack.back();
  }

  void resetExpressionContexts()
  {
    expressionStack.clear();
  }

  void pushExpressionContext()
  {
    expressionStack.push_back({});
  }

  std::string typeName(SemanticTable::Types type)
  {
    return SemanticTable::typeToStr(type);
  }

  bool isNumeric(SemanticTable::Types type)
  {
    return type == SemanticTable::INT || type == SemanticTable::FLOAT;
  }

  bool isBoolConvertible(SemanticTable::Types type)
  {
    return type == SemanticTable::BOOLEAN ||
           type == SemanticTable::INT ||
           type == SemanticTable::FLOAT ||
           type == SemanticTable::STRING;
  }

  SemanticTable::Types applyUnaryOperation(const PendingUnary &unary, SemanticTable::Types operandType)
  {
    switch (unary.kind)
    {
    case UnaryKind::LogicalNot:
      if (!isBoolConvertible(operandType))
      {
        throw SemanticError("Operador '" + unary.lexeme + "' requer valor convertível para booleano, encontrado '" + typeName(operandType) + "'", unary.position, unary.length);
      }
      return SemanticTable::BOOLEAN;
    case UnaryKind::BitwiseNot:
      if (operandType != SemanticTable::INT)
      {
        throw SemanticError("Operador '" + unary.lexeme + "' requer operando inteiro, encontrado '" + typeName(operandType) + "'", unary.position, unary.length);
      }
      return SemanticTable::INT;
    case UnaryKind::ArithmeticNeg:
      if (!isNumeric(operandType))
      {
        throw SemanticError("Operador '" + unary.lexeme + "' requer operando numérico, encontrado '" + typeName(operandType) + "'", unary.position, unary.length);
      }
      return operandType;
    }
    return operandType;
  }

  void applyPendingUnary(ExpressionContext &ctx, SemanticTable::Types &operandType)
  {
    if (ctx.pendingUnary.empty())
    {
      return;
    }
    for (auto it = ctx.pendingUnary.rbegin(); it != ctx.pendingUnary.rend(); ++it)
    {
      operandType = applyUnaryOperation(*it, operandType);
    }
    ctx.pendingUnary.clear();
  }

  SemanticTable::Types evalArithmeticOperation(SemanticTable::Operations op, SemanticTable::Types lhs, SemanticTable::Types rhs, const PendingOperator &info)
  {
    int lhsIdx = static_cast<int>(lhs);
    int rhsIdx = static_cast<int>(rhs);
    int opIdx = static_cast<int>(op);
    int result = SemanticTable::resultType(lhsIdx, rhsIdx, opIdx);
    if (result == SemanticTable::ERR)
    {
      throw SemanticError("Tipos incompatíveis para operador '" + info.lexeme + "': '" + typeName(lhs) + "' e '" + typeName(rhs) + "'", info.position, info.length);
    }
    return static_cast<SemanticTable::Types>(result);
  }

  SemanticTable::Types evaluateBinaryOperation(const PendingOperator &op, SemanticTable::Types lhs, SemanticTable::Types rhs)
  {
    switch (op.kind)
    {
    case OperatorKind::LogicalOr:
    case OperatorKind::LogicalAnd:
      if (!isBoolConvertible(lhs) || !isBoolConvertible(rhs))
      {
        throw SemanticError("Operador '" + op.lexeme + "' requer valores convertíveis para booleano, encontrados '" +
                                typeName(lhs) + "' e '" + typeName(rhs) + "'",
                            op.position, op.length);
      }
      return SemanticTable::BOOLEAN;
    case OperatorKind::BitwiseOr:
    case OperatorKind::BitwiseAnd:
    case OperatorKind::BitwiseXor:
    case OperatorKind::ShiftLeft:
    case OperatorKind::ShiftRight:
      if (lhs != SemanticTable::INT || rhs != SemanticTable::INT)
      {
        throw SemanticError("Operador '" + op.lexeme + "' requer operandos inteiros, encontrados '" + typeName(lhs) + "' e '" + typeName(rhs) + "'", op.position, op.length);
      }
      return SemanticTable::INT;
    case OperatorKind::Add:
      return evalArithmeticOperation(SemanticTable::SUM, lhs, rhs, op);
    case OperatorKind::Subtract:
      return evalArithmeticOperation(SemanticTable::SUB, lhs, rhs, op);
    case OperatorKind::Multiply:
      return evalArithmeticOperation(SemanticTable::MUL, lhs, rhs, op);
    case OperatorKind::Divide:
      return evalArithmeticOperation(SemanticTable::DIV, lhs, rhs, op);
    case OperatorKind::Modulo:
      if (lhs != SemanticTable::INT || rhs != SemanticTable::INT)
      {
        throw SemanticError("Operador '" + op.lexeme + "' requer operandos inteiros, encontrados '" + typeName(lhs) + "' e '" + typeName(rhs) + "'", op.position, op.length);
      }
      return evalArithmeticOperation(SemanticTable::MOD, lhs, rhs, op);
    case OperatorKind::Power:
      if (!isNumeric(lhs) || !isNumeric(rhs))
      {
        throw SemanticError("Operador '" + op.lexeme + "' requer operandos numéricos, encontrados '" + typeName(lhs) + "' e '" + typeName(rhs) + "'", op.position, op.length);
      }
      return evalArithmeticOperation(SemanticTable::POT, lhs, rhs, op);
    case OperatorKind::RelationalCompare:
      if (!isNumeric(lhs) || !isNumeric(rhs))
      {
        throw SemanticError("Operador '" + op.lexeme + "' requer operandos numéricos, encontrados '" + typeName(lhs) + "' e '" + typeName(rhs) + "'", op.position, op.length);
      }
      return SemanticTable::BOOLEAN;
    case OperatorKind::RelationalEquality:
      if (lhs == rhs)
      {
        return SemanticTable::BOOLEAN;
      }
      if (isNumeric(lhs) && isNumeric(rhs))
      {
        return SemanticTable::BOOLEAN;
      }
      throw SemanticError("Operador '" + op.lexeme + "' requer operandos comparáveis, encontrados '" + typeName(lhs) + "' e '" + typeName(rhs) + "'", op.position, op.length);
    }
    return lhs;
  }

  void registerExpressionOperand(SemanticTable::Types operandType, const Token *token)
  {
    (void)token;
    auto &ctx = ensureExpressionContext();
    applyPendingUnary(ctx, operandType);
    if (ctx.hasAccumulated)
    {
      if (ctx.pendingOperator.has_value())
      {
        operandType = evaluateBinaryOperation(*ctx.pendingOperator, ctx.accumulatedType, operandType);
        ctx.pendingOperator.reset();
      }
    }
    else
    {
      ctx.hasAccumulated = true;
    }
    ctx.accumulatedType = operandType;
    semanticTable.noteExprType(ctx.accumulatedType);
#if SEMANTIC_DEBUG
    std::cerr << "    [expr] operand=" << typeName(operandType) << " accumulated=" << typeName(ctx.accumulatedType) << std::endl;
#endif
  }

  void registerBinaryOperator(OperatorKind kind, const Token *token)
  {
    auto &ctx = ensureExpressionContext();
    PendingOperator op;
    op.kind = kind;
    op.position = token ? token->getPosition() : -1;
    op.length = token ? static_cast<int>(token->getLexeme().size()) : 1;
    op.lexeme = token ? token->getLexeme() : "";
    ctx.pendingOperator = op;
  }

  void registerUnaryOperator(UnaryKind kind, const Token *token)
  {
    auto &ctx = ensureExpressionContext();
    PendingUnary unary;
    unary.kind = kind;
    unary.position = token ? token->getPosition() : -1;
    unary.length = token ? static_cast<int>(token->getLexeme().size()) : 1;
    unary.lexeme = token ? token->getLexeme() : "";
    ctx.pendingUnary.push_back(unary);
  }

  bool hasOpeningBracketBefore(const Token *token)
  {
    if (!token)
      return false;
    const std::string &src = Semantico::sourceCode;
    int pos = token->getPosition();
    if (pos <= 0 || pos > static_cast<int>(src.size()))
      return false;
    int i = pos - 1;
    while (i >= 0 && std::isspace(static_cast<unsigned char>(src[i])))
      --i;
    if (i < 0 || src[i] != '[')
      return false;
    int j = i - 1;
    while (j >= 0 && std::isspace(static_cast<unsigned char>(src[j])))
      --j;
    if (j < 0)
      return true;
    const char before = src[j];
    if (std::isalnum(static_cast<unsigned char>(before)) || before == '_' || before == ']')
      return false;
    return true;
  }

  bool closesArrayAfter(const Token *token)
  {
    if (!token)
      return false;
    const std::string &src = Semantico::sourceCode;
    size_t pos = static_cast<size_t>(token->getPosition()) + token->getLexeme().size();
    while (pos < src.size())
    {
      char c = src[pos];
      if (std::isspace(static_cast<unsigned char>(c)))
      {
        ++pos;
        continue;
      }
      return c == ']';
    }
    return false;
  }

  bool typeHasArraySuffix(const Token *token)
  {
    if (!token)
      return false;
    const std::string &src = Semantico::sourceCode;
    size_t pos = static_cast<size_t>(token->getPosition()) + token->getLexeme().size();
    while (pos < src.size() && std::isspace(static_cast<unsigned char>(src[pos])))
      ++pos;
    if (pos >= src.size() || src[pos] != '[')
      return false;
    ++pos;
    while (pos < src.size() && std::isspace(static_cast<unsigned char>(src[pos])))
      ++pos;
    if (pos >= src.size() || src[pos] != ']')
      return false;
    return true;
  }

  bool hasIndexingAfter(const Token *token)
  {
    if (!token)
      return false;
    const std::string &src = Semantico::sourceCode;
    size_t pos = static_cast<size_t>(token->getPosition()) + token->getLexeme().size();
    while (pos < src.size())
    {
      char c = src[pos];
      if (std::isspace(static_cast<unsigned char>(c)))
      {
        ++pos;
        continue;
      }
      return c == '[';
    }
    return false;
  }

  std::pair<int, int> offsetToLineCol(int pos)
  {
    if (pos < 0)
      return {-1, -1};
    int line = 1;
    int column = 1;
    const std::string &src = Semantico::sourceCode;
    const int limit = static_cast<int>(src.size());
    for (int i = 0; i < pos && i < limit; ++i)
    {
      if (src[i] == '\n')
      {
        ++line;
        column = 1;
      }
      else
      {
        ++column;
      }
    }
    return {line, column};
  }

  void resetScopeState()
  {
    activeScopes.clear();
    forHeaderStates.clear();
    waitingDoWhileCondition = false;
    arrayLiteralStates.clear();
  }

  void openScope(ScopeKind kind)
  {
    semanticTable.enterScope();
    activeScopes.push_back(kind);
  }

  void closeScope(ScopeKind expected)
  {
    if (activeScopes.empty())
      return;
    if (activeScopes.back() != expected)
      return;
    activeScopes.pop_back();
    semanticTable.exitScope();
  }

  ForHeaderState *currentForHeaderState()
  {
    if (forHeaderStates.empty())
      return nullptr;
    return &forHeaderStates.back();
  }

  void ensureForInitializerCommitted(Semantico &semantico)
  {
    auto *headerState = currentForHeaderState();
    if (!headerState || headerState->phase != ForHeaderPhase::Init || headerState->initializerCommitted)
      return;

    if (Semantico::currentVariable.name.empty())
    {
      headerState->initializerCommitted = true;
      headerState->phase = ForHeaderPhase::Condition;
      return;
    }

    finalizarInstrucao(semantico);
    headerState->initializerCommitted = true;
    headerState->phase = ForHeaderPhase::Condition;
  }

  void registrarLiteral(Semantico &semantico, const Token *token)
  {
    if (!token || Semantico::currentVariable.isFunction)
      return;
    ensureForInitializerCommitted(semantico);
    const string lexema = token->getLexeme();
    if (lexema == "[" || lexema == "]")
    {
      return;
    }
    if (lexema == ")" || lexema == "(" || lexema == "{" || lexema == "}")
    {
      return;
    }
    const bool startsArray = hasOpeningBracketBefore(token);
    if (startsArray)
    {
      Semantico::currentVariable.literalIsArray = true;
      ArrayLiteralState state;
      if (Semantico::currentVariable.type != Semantico::Type::NULLABLE)
      {
        state.hasDeclaredType = true;
        state.declaredType = static_cast<SemanticTable::Types>(Semantico::currentVariable.type);
        state.elementType = state.declaredType;
      }
      else if (!Semantico::currentVariable.name.empty() && semanticTable.hasSymbol(Semantico::currentVariable.name))
      {
        state.hasDeclaredType = true;
        state.declaredType = semanticTable.getSymbolType(Semantico::currentVariable.name);
        state.elementType = state.declaredType;
      }
      arrayLiteralStates.push_back(state);
    }
    if (!lexema.empty())
    {
      const unsigned char first = static_cast<unsigned char>(lexema.front());
      const bool isIdentifier = std::isalpha(first) || first == '_';
      if (isIdentifier && lexema != "true" && lexema != "false")
      {
        const bool requiresArray = hasIndexingAfter(token);
        semanticTable.markUseIfDeclared(lexema, token ? token->getPosition() : -1, token ? static_cast<int>(token->getLexeme().size()) : 1, requiresArray);
      }
    }
    Semantico::currentVariable.value.push_back(lexema);
    Semantico::currentVariable.valuePositions.push_back(token ? token->getPosition() : -1);
    Semantico::currentVariable.valueLengths.push_back(token ? static_cast<int>(token->getLexeme().size()) : 1);
    Semantico::currentVariable.isInitialized = true;
    const auto literalType = inferLiteralType(lexema);
    if (!arrayLiteralStates.empty())
    {
      auto &state = arrayLiteralStates.back();
      SemanticTable::Types expectedType = state.hasDeclaredType ? state.declaredType : state.elementType;
      if (!state.hasElementType)
      {
        if (state.hasDeclaredType)
        {
          const int compat = SemanticTable::atribType(static_cast<int>(state.declaredType), static_cast<int>(literalType));
          if (compat != SemanticTable::OK)
          {
            throw SemanticError("Tipos incompatíveis no elemento do vetor: esperado '" +
                                SemanticTable::typeToStr(state.declaredType) + "', encontrado '" +
                                SemanticTable::typeToStr(literalType) + "'");
          }
          state.elementType = state.declaredType;
        }
        else
        {
          state.elementType = literalType;
        }
        state.hasElementType = true;
      }
      else
      {
        expectedType = state.hasDeclaredType ? state.declaredType : state.elementType;
        const int compat = SemanticTable::atribType(static_cast<int>(expectedType), static_cast<int>(literalType));
        if (compat != SemanticTable::OK)
        {
          throw SemanticError("Tipos incompatíveis no elemento do vetor: esperado '" +
                              SemanticTable::typeToStr(expectedType) + "', encontrado '" +
                              SemanticTable::typeToStr(literalType) + "'");
        }
      }
    }
    registerExpressionOperand(literalType, token);

    const bool endsArray = closesArrayAfter(token);
    if (endsArray && !arrayLiteralStates.empty())
    {
      ArrayLiteralState state = arrayLiteralStates.back();
      arrayLiteralStates.pop_back();
      if (!state.hasElementType)
      {
        if (state.hasDeclaredType)
        {
          semanticTable.noteExprType(state.declaredType);
        }
        else
        {
          throw SemanticError("Não é possível inferir o tipo de um vetor vazio");
        }
      }
      else
      {
        SemanticTable::Types elemento = state.hasDeclaredType ? state.declaredType : state.elementType;
        if (!state.hasDeclaredType)
        {
          Semantico::currentVariable.type = static_cast<Semantico::Type>(elemento);
        }
        semanticTable.noteExprType(elemento);
      }
      Semantico::currentVariable.isInitialized = true;
    }
  }

  void aplicarTipo(Semantico &semantico, const Token *token)
  {
    if (!token)
      return;
    const auto tipo = semantico.getTypeFromString(token->getLexeme());

    if (Semantico::isTypeParameter)
    {
      auto &parametro = Semantico::currentParameters.back();
      const bool arraySuffix = typeHasArraySuffix(token);
      parametro.type = tipo;
      parametro.isUsed = false;
      Semantico::isTypeParameter = false;
      parametro.isArray = arraySuffix;
      parametro.literalIsArray = false;
      return;
    }

    const bool arraySuffix = typeHasArraySuffix(token);
    Semantico::currentVariable.type = tipo;
    Semantico::currentVariable.isInitialized = false;
    Semantico::currentVariable.isArray = arraySuffix;
    Semantico::currentVariable.literalIsArray = false;

    if (Semantico::currentVariable.isFunction)
    {
      vector<SemanticTable::Param> parametros;
      parametros.reserve(Semantico::currentParameters.size());
      for (size_t i = 0; i < Semantico::currentParameters.size(); ++i)
      {
        const auto &parametro = Semantico::currentParameters[i];
        SemanticTable::Types tipoParametro = SemanticTable::INT;
        if (parametro.type != Semantico::Type::NULLABLE)
        {
          tipoParametro = static_cast<SemanticTable::Types>(parametro.type);
        }
        SemanticTable::Param info{parametro.name, tipoParametro, parametro.position};
        info.isArray = parametro.isArray;
        info.line = parametro.line;
        info.column = parametro.column;
        parametros.push_back(info);
      }
      SemanticTable::Types retorno = SemanticTable::INT;
      if (Semantico::currentVariable.type != Semantico::Type::NULLABLE)
      {
        retorno = static_cast<SemanticTable::Types>(Semantico::currentVariable.type);
      }
      semanticTable.beginFunction(Semantico::currentVariable.name, retorno, parametros, Semantico::currentVariable.position, Semantico::currentVariable.line, Semantico::currentVariable.column);
      semantico.resetCurrentParameters();
      semantico.resetCurrentVariable();
    }
  }

  void registrarIdentificadorOuParametro(const Token *token)
  {
    if (!token)
      return;
    const string nome = token->getLexeme();

    if (Semantico::currentVariable.isFunction)
    {
      const int position = token ? token->getPosition() : -1;
      const auto [line, column] = offsetToLineCol(position);
      Semantico::currentParameters.push_back({nome, Semantico::Type::NULLABLE, {}, {}, {}, -1, false, false, true, false, false, false, false, position, line, column});
      Semantico::isTypeParameter = true;
    }
    else
    {
      Semantico::currentVariable.name = nome;
      Semantico::isTypeParameter = false;
      Semantico::currentVariable.isArray = semanticTable.isArraySymbol(nome);
      Semantico::currentVariable.literalIsArray = false;
      Semantico::currentVariable.type = Semantico::Type::NULLABLE;
      Semantico::currentVariable.position = token ? token->getPosition() : -1;
      const auto [line, column] = offsetToLineCol(Semantico::currentVariable.position);
      Semantico::currentVariable.line = line;
      Semantico::currentVariable.column = column;
    }
  }

  void executarLeitura()
  {
    string valor;
    if (!(cin >> valor))
      return;
    Semantico::currentVariable.value.clear();
    Semantico::currentVariable.valuePositions.clear();
    Semantico::currentVariable.valueLengths.clear();
    Semantico::currentVariable.value.push_back(valor);
    Semantico::currentVariable.valuePositions.push_back(-1);
    Semantico::currentVariable.valueLengths.push_back(static_cast<int>(valor.size()));
    semanticTable.noteExprType(SemanticTable::INT);
  }

  void finalizarInstrucao(Semantico &semantico)
  {
    SemanticTable::SymbolEntry entrada;
    entrada.name = Semantico::currentVariable.name;
    const bool possuiTipo = Semantico::currentVariable.type != Semantico::Type::NULLABLE;
    entrada.type = possuiTipo ? static_cast<SemanticTable::Types>(Semantico::currentVariable.type)
                              : SemanticTable::INT;
    entrada.initialized = Semantico::currentVariable.isInitialized;
    entrada.used = Semantico::currentVariable.isUsed;
    entrada.scope = -1;
    entrada.isParameter = Semantico::currentVariable.isParameter;
    entrada.position = Semantico::currentVariable.position;
    entrada.line = Semantico::currentVariable.line;
    entrada.column = Semantico::currentVariable.column;
    entrada.isArray = Semantico::currentVariable.isArray;
    entrada.isFunction = false;
    entrada.isConstant = Semantico::currentVariable.isConstant;
    entrada.hasExplicitType = possuiTipo;

    if (Semantico::currentVariable.literalIsArray && !Semantico::currentVariable.isArray)
    {
      throw SemanticError("Variável não declarada como vetor: '" + entrada.name + "'", Semantico::currentVariable.position, static_cast<int>(Semantico::currentVariable.name.size()));
    }

    semanticTable.commitStatement(entrada);
    semantico.resetCurrentVariable();
  }
}

void Semantico::resetCurrentVariable()
{
  Semantico::currentVariable = {"", Semantico::Type::NULLABLE, {}, {}, {}, -1, false, false, false, false, false, false, false, -1, -1, -1};
  Semantico::isTypeParameter = false;
  resetExpressionContexts();
}

void Semantico::resetCurrentParameters()
{
  Semantico::currentParameters.clear();
  Semantico::isTypeParameter = false;
}

void Semantico::resetState()
{
  semanticTable.reset();
  resetScopeState();
  resetCurrentVariable();
  resetCurrentParameters();
  sourceCode.clear();
}

void Semantico::setSourceCode(const std::string &code)
{
  sourceCode = code;
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
#if SEMANTIC_DEBUG
  std::cerr << "[SEM] action=" << action;
  if (token)
  {
    std::cerr << " token='" << token->getLexeme() << "' pos=" << token->getPosition();
  }
  else
  {
    std::cerr << " token=<null>";
  }
  std::cerr << std::endl;
#endif
  switch (action)
  {
  case 1:
    // VALUE
    registrarLiteral(*this, token);
    break;
  case 2:  // OR
    registerBinaryOperator(OperatorKind::LogicalOr, token);
    break;
  case 3:  // AND
    registerBinaryOperator(OperatorKind::LogicalAnd, token);
    break;
  case 4:  // BIT OR
    registerBinaryOperator(OperatorKind::BitwiseOr, token);
    break;
  case 5:  // EXPO
    registerBinaryOperator(OperatorKind::Power, token);
    break;
  case 6:  // BIT AND
    registerBinaryOperator(OperatorKind::BitwiseAnd, token);
    break;
  case 7:  // OP REL
    if (token)
    {
      const std::string lex = token->getLexeme();
      if (lex == "==" || lex == "!=")
      {
        registerBinaryOperator(OperatorKind::RelationalEquality, token);
      }
      else
      {
        registerBinaryOperator(OperatorKind::RelationalCompare, token);
      }
    }
    break;
  case 8:  // OP BITWISE
    if (token)
    {
      const std::string lex = token->getLexeme();
      if (lex == "^")
      {
        registerBinaryOperator(OperatorKind::BitwiseXor, token);
      }
      else if (lex == "<<")
      {
        registerBinaryOperator(OperatorKind::ShiftLeft, token);
      }
      else if (lex == ">>")
      {
        registerBinaryOperator(OperatorKind::ShiftRight, token);
      }
    }
    break;
  case 9:  // ARIT LOWER
    if (token)
    {
      const std::string lex = token->getLexeme();
      if (lex == "+")
      {
        registerBinaryOperator(OperatorKind::Add, token);
      }
      else
      {
        registerBinaryOperator(OperatorKind::Subtract, token);
      }
    }
    break;
  case 10: // ARIT UPPER
    if (token)
    {
      const std::string lex = token->getLexeme();
      if (lex == "*")
      {
        registerBinaryOperator(OperatorKind::Multiply, token);
      }
      else if (lex == "/")
      {
        registerBinaryOperator(OperatorKind::Divide, token);
      }
      else if (lex == "%")
      {
        registerBinaryOperator(OperatorKind::Modulo, token);
      }
    }
    break;
  case 11: // NEG
    if (token)
    {
      const std::string lex = token->getLexeme();
      if (lex == "!")
      {
        registerUnaryOperator(UnaryKind::LogicalNot, token);
      }
      else if (lex == "~")
      {
        registerUnaryOperator(UnaryKind::BitwiseNot, token);
      }
      else if (lex == "-")
      {
        registerUnaryOperator(UnaryKind::ArithmeticNeg, token);
      }
    }
    break;
  case 12: // LEFT PARENTHESIS
  {
    auto *headerState = currentForHeaderState();
    if (headerState && headerState->phase != ForHeaderPhase::Body)
    {
      headerState->parenthesisDepth++;
    }
    pushExpressionContext();
    break;
  }
  case 13: // RIGHT PARENTHESIS
  {
    auto *headerState = currentForHeaderState();
    if (headerState && headerState->phase != ForHeaderPhase::Body && headerState->parenthesisDepth > 0)
    {
      headerState->parenthesisDepth--;
      if (headerState->parenthesisDepth == 0)
      {
        headerState->phase = ForHeaderPhase::Body;
        semanticTable.discardPendingExpression();
        resetCurrentVariable();
        break;
      }
    }
    if (!expressionStack.empty())
    {
      ExpressionContext finished = expressionStack.back();
      expressionStack.pop_back();
      if (finished.pendingOperator.has_value())
      {
        const auto &pending = *finished.pendingOperator;
        throw SemanticError("Operador '" + pending.lexeme + "' sem operando à direita", pending.position, pending.length);
      }
      if (finished.hasAccumulated)
      {
        registerExpressionOperand(finished.accumulatedType, token);
      }
    }
    break;
  }
  case 14:
    // FUNCTION CALL
    if (token)
    {
      const std::string lexema = token->getLexeme();
      if (Semantico::currentVariable.name.empty())
      {
        Semantico::currentVariable.name = lexema;
        Semantico::currentVariable.position = token->getPosition();
      }
      semanticTable.markUseIfDeclared(lexema, token->getPosition(), static_cast<int>(token->getLexeme().size()));
    }
    Semantico::currentVariable.isUsed = true;
    break;
  case 15:
    // INDEXED VALUE
    if (token)
    {
      const std::string lexema = token->getLexeme();
      if (Semantico::currentVariable.name.empty())
      {
        Semantico::currentVariable.name = lexema;
      }
      {
        auto *headerState = currentForHeaderState();
        if (!(headerState && headerState->phase == ForHeaderPhase::Init && Semantico::currentVariable.value.empty()))
        {
          semanticTable.markUseIfDeclared(lexema, token ? token->getPosition() : -1, token ? static_cast<int>(token->getLexeme().size()) : 1, true);
        }
      }
      Semantico::currentVariable.value.push_back(lexema);
      Semantico::currentVariable.valuePositions.push_back(token ? token->getPosition() : -1);
      Semantico::currentVariable.valueLengths.push_back(token ? static_cast<int>(token->getLexeme().size()) : 1);
    }
    break;
  case 16:
    // COMMENTS
    break;
  case 17:
    // PRINT
    for (size_t idx = 0; idx < Semantico::currentVariable.value.size(); ++idx)
    {
      const auto &value = Semantico::currentVariable.value[idx];
      const int valuePos = idx < Semantico::currentVariable.valuePositions.size() ? Semantico::currentVariable.valuePositions[idx] : -1;
      const int valueLen = idx < Semantico::currentVariable.valueLengths.size() ? Semantico::currentVariable.valueLengths[idx] : static_cast<int>(value.size());
      if (!value.empty() && (std::isalpha(static_cast<unsigned char>(value.front())) || value.front() == '_'))
      {
        if (value != "true" && value != "false")
        {
          semanticTable.markUseIfDeclared(value, valuePos, valueLen);
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
    Semantico::currentVariable.position = token ? token->getPosition() : -1;
    {
      const auto [line, column] = offsetToLineCol(Semantico::currentVariable.position);
      Semantico::currentVariable.line = line;
      Semantico::currentVariable.column = column;
    }
    break;
  case 24:
    // VALUE INCREMENT/DECREMENT
    Semantico::currentVariable.name = token->getLexeme();
    Semantico::currentVariable.position = token ? token->getPosition() : -1;
    {
      const auto [line, column] = offsetToLineCol(Semantico::currentVariable.position);
      Semantico::currentVariable.line = line;
      Semantico::currentVariable.column = column;
    }
    semanticTable.markUseIfDeclared(Semantico::currentVariable.name, token ? token->getPosition() : -1, token ? static_cast<int>(token->getLexeme().size()) : 1);
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
    semanticTable.discardPendingExpression();
    resetCurrentVariable();
    break;
  case 32:
    // BREAK
    break;
  case 33:
    // THROW
    break;
  case 34:
    if (token)
    {
      const std::string lexema = token->getLexeme();
      if (lexema == "if" || lexema == "elif" || lexema == "else")
      {
        openScope(ScopeKind::IfBranch);
      }
    }
    else
    {
      openScope(ScopeKind::IfBranch);
    }
    break;
  case 35:
    openScope(ScopeKind::DoLoop);
    waitingDoWhileCondition = false;
    break;
  case 36:
    if (waitingDoWhileCondition)
    {
      waitingDoWhileCondition = false;
    }
    else
    {
      openScope(ScopeKind::WhileLoop);
    }
    break;
  case 37:
  {
    openScope(ScopeKind::ForLoop);
    forHeaderStates.push_back({});
    break;
  }
  case 38:
    if (token)
    {
      registrarIdentificadorOuParametro(token);
    }
    break;
  case 39:
    if (token)
    {
      aplicarTipo(*this, token);
    }
    break;
  case 40:
  {
    auto *headerState = currentForHeaderState();
    if (headerState && headerState->phase == ForHeaderPhase::Init)
    {
      if (token && !Semantico::currentVariable.isFunction)
      {
        const std::string lexema = token->getLexeme();
        Semantico::currentVariable.value.push_back(lexema);
        Semantico::currentVariable.valuePositions.push_back(token->getPosition());
        Semantico::currentVariable.valueLengths.push_back(static_cast<int>(token->getLexeme().size()));
        Semantico::currentVariable.isInitialized = true;
        semanticTable.noteExprType(inferLiteralType(lexema));
      }
    }
    else if (headerState && headerState->phase == ForHeaderPhase::Update)
    {
      finalizarInstrucao(*this);
      headerState->phase = ForHeaderPhase::Body;
    }
    break;
  }
  case 41:
    if (token)
    {
      const std::string lexema = token->getLexeme();
      if (lexema == "switch")
      {
        openScope(ScopeKind::SwitchRoot);
      }
      else if (lexema == "case" || lexema == "default")
      {
        openScope(ScopeKind::CaseBranch);
      }
    }
    break;
  case 42:
  {
    auto *headerState = currentForHeaderState();
    if (headerState && headerState->phase != ForHeaderPhase::Body)
    {
      if (headerState->phase == ForHeaderPhase::Init)
      {
        finalizarInstrucao(*this);
        headerState->phase = ForHeaderPhase::Condition;
      }
      else if (headerState->phase == ForHeaderPhase::Condition)
      {
        semanticTable.discardPendingExpression();
        resetCurrentVariable();
        headerState->phase = ForHeaderPhase::Update;
      }
      else
      {
        finalizarInstrucao(*this);
      }
      break;
    }
    finalizarInstrucao(*this);
    break;
  }
  case 43:
    // FUNCTION FINAL
    semanticTable.maybeCloseFunction();
    break;
  case 44:
    closeScope(ScopeKind::IfBranch);
    semanticTable.discardPendingExpression();
    resetCurrentVariable();
    break;
  case 45:
    closeScope(ScopeKind::WhileLoop);
    semanticTable.discardPendingExpression();
    resetCurrentVariable();
    break;
  case 46:
    closeScope(ScopeKind::DoLoop);
    waitingDoWhileCondition = true;
    semanticTable.discardPendingExpression();
    resetCurrentVariable();
    break;
  case 47:
    waitingDoWhileCondition = false;
    semanticTable.discardPendingExpression();
    resetCurrentVariable();
    break;
  case 48:
    closeScope(ScopeKind::ForLoop);
    if (!forHeaderStates.empty())
    {
      forHeaderStates.pop_back();
    }
    semanticTable.discardPendingExpression();
    resetCurrentVariable();
    break;
  case 49:
    closeScope(ScopeKind::SwitchRoot);
    semanticTable.discardPendingExpression();
    resetCurrentVariable();
    break;
  case 50:
  case 51:
    closeScope(ScopeKind::CaseBranch);
    semanticTable.discardPendingExpression();
    resetCurrentVariable();
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

std::vector<SymbolInfo> Semantico::symbolTable() const
{
  std::vector<SymbolInfo> result;
  const auto &entries = semanticTable.getSymbolTable();
  result.reserve(entries.size());

  for (const auto &entry : entries)
  {
    SymbolInfo info;
    info.identifier = entry.name;
    info.type = SemanticTable::typeToStr(entry.type);
    info.isConstant = entry.isConstant;
    info.initialized = entry.initialized;
    info.used = entry.used;
    info.scope = entry.scope;
    info.line = entry.line;
    info.column = entry.column;
    if ((info.line < 0 || info.column < 0) && entry.position >= 0)
    {
      const auto [line, column] = offsetToLineCol(entry.position);
      info.line = line;
      info.column = column;
    }
    info.isParameter = entry.isParameter;
    info.isArray = entry.isArray;
    info.isFunction = entry.isFunction;
    result.push_back(info);
  }

  return result;
}

void Semantico::clearSymbolTable()
{
  semanticTable.reset();
  resetScopeState();
  resetCurrentVariable();
  resetCurrentParameters();
  sourceCode.clear();
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
    symbol.line = entry.line;
    symbol.column = entry.column;
    if (symbol.position >= 0 && (entry.line < 0 || entry.column < 0))
    {
      const auto [line, column] = offsetToLineCol(symbol.position);
      symbol.line = line;
      symbol.column = column;
    }
    symbol.isArray = entry.isArray;
    symbol.isFunction = entry.isFunction;
    symbol.isConstant = entry.isConstant;
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
    exported.push_back({diag.severity, diag.message, diag.position, diag.length});
  }

  return exported;
}

void finalizeSemanticAnalysis()
{
  semanticTable.closeAllScopes();
}
