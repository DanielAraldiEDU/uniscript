#include "BipGenerator.h"

#include <cctype>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <algorithm>
#include <limits>

namespace
{
  constexpr const int TEMP_BASE_ADDRESS = 1100;
  constexpr const int TEMP_MAX_ADDRESS = 2047;
  constexpr const char *TEMP_VECTOR_INDEX = "1002";
  constexpr const char *TEMP_VECTOR_VALUE = "1000";
  constexpr const char *TEMP_VECTOR_VALUE_ALT = "1001";

  struct Entry
  {
    std::string name;
    bool isArray = false;
    std::size_t elementCount = 1;
    bool hasInitializer = false;
    std::vector<std::string> literalValues;
  };

  std::vector<Entry> entries;
  std::vector<std::string> instructions;
  std::vector<std::pair<std::size_t, std::vector<std::string>>> statementInstructions;
  std::string cachedCode;
  constexpr const char *OUTPUT_FILE = "output.bip";
  static bool readStatementsScanned = false;

  std::string trim(const std::string &text)
  {
    std::size_t start = 0;
    while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start])))
      ++start;
    std::size_t end = text.size();
    while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1])))
      --end;
    return text.substr(start, end - start);
  }

  std::size_t findMatchingParenthesis(const std::string &src, std::size_t openPos)
  {
    int depth = 0;
    for (std::size_t idx = openPos; idx < src.size(); ++idx)
    {
      char c = src[idx];
      if (c == '(')
      {
        ++depth;
      }
      else if (c == ')')
      {
        --depth;
        if (depth == 0)
        {
          return idx;
        }
      }
    }
    return std::string::npos;
  }

  struct Expr;
  using ExprPtr = std::unique_ptr<Expr>;

  struct Expr
  {
    enum class Kind
    {
      Literal,
      Variable,
      ArrayAccess,
      Binary
    } kind;
    std::string value;
    char op = '\0';
    ExprPtr left;
    ExprPtr right;
    ExprPtr index;

    static ExprPtr makeLiteral(std::string literal)
    {
      auto node = std::make_unique<Expr>();
      node->kind = Kind::Literal;
      node->value = std::move(literal);
      return node;
    }

    static ExprPtr makeVariable(std::string name)
    {
      auto node = std::make_unique<Expr>();
      node->kind = Kind::Variable;
      node->value = std::move(name);
      return node;
    }

    static ExprPtr makeArrayAccess(std::string name, ExprPtr idx)
    {
      auto node = std::make_unique<Expr>();
      node->kind = Kind::ArrayAccess;
      node->value = std::move(name);
      node->index = std::move(idx);
      return node;
    }

    static ExprPtr makeBinary(char operation, ExprPtr lhs, ExprPtr rhs)
    {
      auto node = std::make_unique<Expr>();
      node->kind = Kind::Binary;
      node->op = operation;
      node->left = std::move(lhs);
      node->right = std::move(rhs);
      return node;
    }
  };

  struct Token
  {
    enum class Type
    {
      End,
      Identifier,
      Number,
      Operator,
      LBracket,
      RBracket,
      LParen,
      RParen,
      Comma
    } type = Type::End;
    std::string lexeme;
  };

  class Lexer
  {
  public:
    explicit Lexer(const std::string &input) : text(input), length(input.size()) {}

    Token next()
    {
      skipWhitespace();
      if (pos >= length)
        return Token{Token::Type::End, ""};

      char c = text[pos];

      if (std::isalpha(static_cast<unsigned char>(c)) || c == '_' || c == '$')
        return identifier();
      if (std::isdigit(static_cast<unsigned char>(c)))
        return number();

      ++pos;
      switch (c)
      {
      case '+':
      case '-':
      case '&':
      case '|':
      case '^':
        return Token{Token::Type::Operator, std::string(1, c)};
      case '[':
        return Token{Token::Type::LBracket, "["};
      case ']':
        return Token{Token::Type::RBracket, "]"};
      case '(':
        return Token{Token::Type::LParen, "("};
      case ')':
        return Token{Token::Type::RParen, ")"};
      case ',':
        return Token{Token::Type::Comma, ","};
      default:
        throw std::runtime_error("Token desconhecido: " + std::string(1, c));
      }
    }

  private:
    const std::string &text;
    const std::size_t length;
    std::size_t pos = 0;

    void skipWhitespace()
    {
      while (pos < length && std::isspace(static_cast<unsigned char>(text[pos])))
        ++pos;
    }

    Token identifier()
    {
      std::size_t start = pos;
      while (pos < length)
      {
        char c = text[pos];
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '$')
          break;
        ++pos;
      }
      return Token{Token::Type::Identifier, text.substr(start, pos - start)};
    }

    Token number()
    {
      std::size_t start = pos;
      while (pos < length && std::isdigit(static_cast<unsigned char>(text[pos])))
        ++pos;
      return Token{Token::Type::Number, text.substr(start, pos - start)};
    }
  };

  class Parser
  {
  public:
    explicit Parser(const std::string &input) : lexer(input)
    {
      advance();
    }

    ExprPtr parseExpression()
    {
      auto expr = parseFactor();
      while (matchOperator())
      {
        char op = previous.lexeme[0];
        auto rhs = parseFactor();
        expr = Expr::makeBinary(op, std::move(expr), std::move(rhs));
      }
      return expr;
    }

    bool atEnd() const
    {
      return current.type == Token::Type::End;
    }

  private:
    Lexer lexer;
    Token current;
    Token previous;

    void advance()
    {
      previous = current;
      current = lexer.next();
    }

    bool match(Token::Type type)
    {
      if (current.type == type)
      {
        advance();
        return true;
      }
      return false;
    }

    bool matchOperator()
    {
      if (current.type == Token::Type::Operator)
      {
        advance();
        return true;
      }
      return false;
    }

    ExprPtr parseFactor()
    {
      if (match(Token::Type::Number))
      {
        return Expr::makeLiteral(previous.lexeme);
      }
      if (match(Token::Type::Identifier))
      {
        std::string name = previous.lexeme;
        if (match(Token::Type::LBracket))
        {
          auto index = parseExpression();
          if (!match(Token::Type::RBracket))
          {
            throw std::runtime_error("Faltando ']' em acesso a vetor");
          }
          return Expr::makeArrayAccess(name, std::move(index));
        }
        return Expr::makeVariable(name);
      }
      if (match(Token::Type::LParen))
      {
        auto expr = parseExpression();
        if (!match(Token::Type::RParen))
        {
          throw std::runtime_error("Faltando ')' na expressão");
        }
        return expr;
      }

      throw std::runtime_error("Expressão inválida");
    }
  };

  std::vector<std::string> splitTopLevel(const std::string &source, char separator)
  {
    std::vector<std::string> parts;
    int depthParen = 0;
    int depthBracket = 0;
    int depthBrace = 0;
    std::size_t last = 0;
    for (std::size_t i = 0; i < source.size(); ++i)
    {
      char c = source[i];
      switch (c)
      {
      case '(':
        ++depthParen;
        break;
      case ')':
        --depthParen;
        break;
      case '[':
        ++depthBracket;
        break;
      case ']':
        --depthBracket;
        break;
      case '{':
        ++depthBrace;
        break;
      case '}':
        --depthBrace;
        break;
      default:
        break;
      }
      if (c == separator && depthParen == 0 && depthBracket == 0 && depthBrace == 0)
      {
        parts.push_back(trim(source.substr(last, i - last)));
        last = i + 1;
      }
    }
    if (last < source.size())
    {
      parts.push_back(trim(source.substr(last)));
    }
    return parts;
  }

  struct ParsedAssignment
  {
    std::string targetName;
    bool targetIsArray = false;
    ExprPtr targetIndex;
    ExprPtr rhsExpr;
    std::vector<ExprPtr> rhsArrayElements;
    std::size_t statementStart = 0;
  };

  bool extractAssignmentSlices(const Semantico::Variable &variable, std::string &lhsOut, std::string &rhsOut, std::size_t &statementStart)
  {
    const std::string &src = Semantico::sourceCode;
    std::size_t start = std::string::npos;
    if (variable.position >= 0)
    {
      start = static_cast<std::size_t>(variable.position);
    }
    else
    {
      int best = std::numeric_limits<int>::max();
      for (int pos : variable.valuePositions)
      {
        if (pos >= 0 && pos < best)
        {
          best = pos;
        }
      }
      if (best != std::numeric_limits<int>::max())
      {
        start = static_cast<std::size_t>(best);
      }
    }
    if (start == std::string::npos || start >= src.size())
      return false;

    while (start > 0 && std::isspace(static_cast<unsigned char>(src[start - 1])))
      --start;

    int depthParen = 0;
    int depthBracket = 0;
    int depthBrace = 0;
    bool inString = false;
    char stringDelim = '\0';
    std::size_t assignPos = std::string::npos;
    for (std::size_t i = start; i < src.size(); ++i)
    {
      char c = src[i];
      if (inString)
      {
        if (c == '\\' && i + 1 < src.size())
        {
          ++i;
          continue;
        }
        if (c == stringDelim)
        {
          inString = false;
        }
        continue;
      }
      if (c == '"' || c == '\'')
      {
        inString = true;
        stringDelim = c;
        continue;
      }
      if (c == '(')
      {
        ++depthParen;
        continue;
      }
      if (c == ')')
      {
        --depthParen;
        continue;
      }
      if (c == '[')
      {
        ++depthBracket;
        continue;
      }
      if (c == ']')
      {
        --depthBracket;
        continue;
      }
      if (c == '{')
      {
        ++depthBrace;
        continue;
      }
      if (c == '}')
      {
        --depthBrace;
        continue;
      }

      if (c == '=' && depthParen == 0 && depthBracket == 0 && depthBrace == 0)
      {
        if (i + 1 < src.size() && src[i + 1] == '=')
        {
          ++i;
          continue;
        }
        if (i > 0)
        {
          char prev = src[i - 1];
          if (prev == '<' || prev == '>' || prev == '!' || prev == '=')
          {
            continue;
          }
        }
        assignPos = i;
        break;
      }
    }

    if (assignPos == std::string::npos)
      return false;

    depthParen = depthBracket = depthBrace = 0;
    inString = false;
    stringDelim = '\0';
    std::size_t endPos = std::string::npos;
    for (std::size_t i = assignPos + 1; i < src.size(); ++i)
    {
      char c = src[i];
      if (inString)
      {
        if (c == '\\' && i + 1 < src.size())
        {
          ++i;
          continue;
        }
        if (c == stringDelim)
        {
          inString = false;
        }
        continue;
      }
      if (c == '"' || c == '\'')
      {
        inString = true;
        stringDelim = c;
        continue;
      }
      if (c == '(')
      {
        ++depthParen;
        continue;
      }
      if (c == ')')
      {
        --depthParen;
        continue;
      }
      if (c == '[')
      {
        ++depthBracket;
        continue;
      }
      if (c == ']')
      {
        --depthBracket;
        continue;
      }
      if (c == '{')
      {
        ++depthBrace;
        continue;
      }
      if (c == '}')
      {
        --depthBrace;
        continue;
      }
      if (c == ';' && depthParen == 0 && depthBracket == 0 && depthBrace == 0)
      {
        endPos = i;
        break;
      }
    }

    if (endPos == std::string::npos)
      return false;

    lhsOut = trim(src.substr(start, assignPos - start));
    rhsOut = trim(src.substr(assignPos + 1, endPos - assignPos - 1));
    statementStart = start;
    return !lhsOut.empty() && !rhsOut.empty();
  }

  ExprPtr parseExpressionString(const std::string &exprString)
  {
    Parser parser(exprString);
    auto expr = parser.parseExpression();
    if (!parser.atEnd())
    {
      throw std::runtime_error("Tokens extras após expressão: " + exprString);
    }
    return expr;
  }

  bool parseAssignmentFromSource(const Semantico::Variable &variable, ParsedAssignment &parsed)
  {
    std::string lhs;
    std::string rhs;
    std::size_t statementStart = 0;
    if (!extractAssignmentSlices(variable, lhs, rhs, statementStart))
      return false;

    if (lhs.empty() || rhs.empty())
      return false;

    parsed.targetName = variable.name;
    std::size_t bracketPos = lhs.find('[');
    if (bracketPos != std::string::npos)
    {
      std::size_t closing = lhs.rfind(']');
      if (closing == std::string::npos || closing <= bracketPos)
        return false;
      parsed.targetIsArray = true;
      parsed.targetName = trim(lhs.substr(0, bracketPos));
      std::string indexExpr = trim(lhs.substr(bracketPos + 1, closing - bracketPos - 1));
      if (indexExpr.empty())
        throw std::runtime_error("Índice vazio em acesso a vetor");
      parsed.targetIndex = parseExpressionString(indexExpr);
    }

    if (!rhs.empty() && rhs.front() == '[')
    {
      if (rhs.back() != ']')
        throw std::runtime_error("Vetor literal mal formado");
      std::string inner = trim(rhs.substr(1, rhs.size() - 2));
      if (inner.empty())
        throw std::runtime_error("Vetor literal vazio não suportado");
      auto parts = splitTopLevel(inner, ',');
      for (const auto &part : parts)
      {
        parsed.rhsArrayElements.push_back(parseExpressionString(part));
      }
      parsed.statementStart = statementStart;
      return true;
    }

    parsed.rhsExpr = parseExpressionString(rhs);
    parsed.statementStart = statementStart;
    return true;
  }

  std::string opcodeForOperator(char op)
  {
    switch (op)
    {
    case '+':
      return "ADD";
    case '-':
      return "SUB";
    case '&':
      return "AND";
    case '|':
      return "OR";
    case '^':
      return "XOR";
    default:
      throw std::runtime_error(std::string("Operador não suportado: ") + op);
    }
  }

  class ExpressionEmitter
  {
  public:
    explicit ExpressionEmitter(std::vector<std::string> &instructionsRef)
        : instructions(instructionsRef)
    {
    }

    void reset()
    {
      nextTemp = TEMP_BASE_ADDRESS;
    }

    std::string emit(const Expr &expr)
    {
      switch (expr.kind)
      {
      case Expr::Kind::Literal:
      {
        std::string temp = allocateTemp();
        instructions.push_back("LDI " + expr.value);
        instructions.push_back("STO " + temp);
        return temp;
      }
      case Expr::Kind::Variable:
      {
        std::string temp = allocateTemp();
        instructions.push_back("LD " + expr.value);
        instructions.push_back("STO " + temp);
        return temp;
      }
      case Expr::Kind::ArrayAccess:
      {
        if (!expr.index)
        {
          throw std::runtime_error("Acesso a vetor sem índice");
        }
        std::string indexTemp = emit(*expr.index);
        instructions.push_back("LD " + indexTemp);
        instructions.push_back(std::string("STO ") + TEMP_VECTOR_INDEX);
        instructions.push_back(std::string("LD ") + TEMP_VECTOR_INDEX);
        instructions.push_back("STO $indr");
        instructions.push_back("LDV " + expr.value);
        std::string temp = allocateTemp();
        instructions.push_back("STO " + temp);
        return temp;
      }
      case Expr::Kind::Binary:
      {
        if (!expr.left || !expr.right)
        {
          throw std::runtime_error("Expressão binária inválida");
        }
        std::string rightTemp = emit(*expr.right);
        std::string leftTemp = emit(*expr.left);
        instructions.push_back("LD " + leftTemp);
        instructions.push_back(opcodeForOperator(expr.op) + " " + rightTemp);
        std::string temp = allocateTemp();
        instructions.push_back("STO " + temp);
        return temp;
      }
      }
      throw std::runtime_error("Expressão desconhecida");
    }

  private:
    std::vector<std::string> &instructions;
    int nextTemp = TEMP_BASE_ADDRESS;

    std::string allocateTemp()
    {
      if (nextTemp > TEMP_MAX_ADDRESS)
      {
        throw std::runtime_error("Sem temporários disponíveis para expressão");
      }
      return std::to_string(nextTemp++);
    }
  };

  bool isSimpleArrayAccess(const Expr &expr)
  {
    if (expr.kind != Expr::Kind::ArrayAccess || !expr.index)
      return false;
    auto kind = expr.index->kind;
    return kind == Expr::Kind::Literal || kind == Expr::Kind::Variable;
  }

  bool collectAddSubTerms(const Expr &expr, std::vector<std::pair<int, const Expr *>> &terms, int sign)
  {
    if (expr.kind == Expr::Kind::Binary && (expr.op == '+' || expr.op == '-'))
    {
      if (!collectAddSubTerms(*expr.left, terms, sign))
        return false;
      int rightSign = expr.op == '+' ? sign : -sign;
      return collectAddSubTerms(*expr.right, terms, rightSign);
    }

    if (expr.kind == Expr::Kind::Literal || expr.kind == Expr::Kind::Variable || isSimpleArrayAccess(expr))
    {
      terms.push_back({sign, &expr});
      return true;
    }

    return false;
  }

  bool loadSimpleArrayInto(const Expr &expr, std::vector<std::string> &code, const std::string &temp, bool storeValue)
  {
    if (!isSimpleArrayAccess(expr))
      return false;

    const Expr *index = expr.index.get();
    if (index->kind == Expr::Kind::Literal)
    {
      code.push_back("LDI " + index->value);
    }
    else if (index->kind == Expr::Kind::Variable)
    {
      code.push_back("LD " + index->value);
    }
    else
    {
      return false;
    }
    code.push_back("STO $indr");
    code.push_back("LDV " + expr.value);
    if (storeValue)
    {
      code.push_back("STO " + temp);
    }
    return true;
  }

  bool emitOptimizedAddSub(const Expr &expr, const std::string &target, std::vector<std::string> &code)
  {
    std::vector<std::pair<int, const Expr *>> terms;
    if (!collectAddSubTerms(expr, terms, 1))
      return false;
    if (terms.size() < 2)
      return false;

    int firstIndex = -1;
    for (std::size_t i = 0; i < terms.size(); ++i)
    {
      if (terms[i].first > 0)
      {
        firstIndex = static_cast<int>(i);
        break;
      }
    }
    if (firstIndex < 0)
      return false;

    std::vector<std::pair<const Expr *, std::string>> arrayTemps;

    auto loadInitial = [&](const Expr *node) -> bool {
      if (node->kind == Expr::Kind::Literal)
      {
        code.push_back("LDI " + node->value);
        return true;
      }
      if (node->kind == Expr::Kind::Variable)
      {
        code.push_back("LD " + node->value);
        return true;
      }
      if (isSimpleArrayAccess(*node))
      {
        return loadSimpleArrayInto(*node, code, "", false);
      }
      return false;
    };

    auto applyTerm = [&](int sign, const Expr *node) -> bool {
      const bool positive = sign > 0;
      if (node->kind == Expr::Kind::Literal)
      {
        code.push_back(std::string(positive ? "ADDI " : "SUBI ") + node->value);
        return true;
      }
      if (node->kind == Expr::Kind::Variable)
      {
        code.push_back(std::string(positive ? "ADD " : "SUB ") + node->value);
        return true;
      }
      if (isSimpleArrayAccess(*node))
      {
        code.push_back(std::string("STO ") + TEMP_VECTOR_VALUE);
        if (!loadSimpleArrayInto(*node, code, TEMP_VECTOR_VALUE_ALT, true))
        {
          return false;
        }
        code.push_back(std::string("LD ") + TEMP_VECTOR_VALUE);
        code.push_back(std::string(positive ? "ADD " : "SUB ") + std::string(TEMP_VECTOR_VALUE_ALT));
        return true;
      }
      return false;
    };

    if (!loadInitial(terms[firstIndex].second))
    {
      return false;
    }

    for (std::size_t i = 0; i < terms.size(); ++i)
    {
      if (static_cast<int>(i) == firstIndex)
        continue;
      if (!applyTerm(terms[i].first, terms[i].second))
        return false;
    }
    code.push_back("STO " + target);
    return true;
  }

  void generateReadIntoExpression(const Expr &expr, std::vector<std::string> &out)
  {
    ExpressionEmitter emitter(out);
    emitter.reset();
    if (expr.kind == Expr::Kind::Variable)
    {
      out.push_back("LD $in_port");
      out.push_back("STO " + expr.value);
      return;
    }
    if (expr.kind == Expr::Kind::ArrayAccess && expr.index)
    {
      if (expr.index->kind == Expr::Kind::Literal)
      {
        out.push_back("LDI " + expr.index->value);
      }
      else if (expr.index->kind == Expr::Kind::Variable)
      {
        out.push_back("LD " + expr.index->value);
      }
      else
      {
        std::string indexTemp = emitter.emit(*expr.index);
        out.push_back("LD " + indexTemp);
      }
      out.push_back("STO $indr");
      out.push_back("LD $in_port");
      out.push_back("STOV " + expr.value);
      return;
    }
    throw std::runtime_error("Destino inválido para leitura");
  }

  void generatePrintExpression(const Expr &expr, std::vector<std::string> &out)
  {
    ExpressionEmitter emitter(out);
    emitter.reset();
    std::string valueTemp = emitter.emit(expr);
    out.push_back("LD " + valueTemp);
    out.push_back("OUT");
  }

  bool isIntegerLiteral(const std::string &lexeme)
  {
    if (lexeme.empty())
    {
      return false;
    }
    std::size_t idx = 0;
    if (lexeme[idx] == '+' || lexeme[idx] == '-')
    {
      ++idx;
    }
    if (idx >= lexeme.size())
    {
      return false;
    }
    for (; idx < lexeme.size(); ++idx)
    {
      if (!std::isdigit(static_cast<unsigned char>(lexeme[idx])))
      {
        return false;
      }
    }
    return true;
  }

  std::size_t inferArrayLength(const Semantico::Variable &variable)
  {
    if (!variable.isArray)
    {
      return 1;
    }

    if (!variable.literalIsArray)
    {
      return 1;
    }

    std::size_t count = 0;
    for (const auto &token : variable.value)
    {
      if (isIntegerLiteral(token))
      {
        ++count;
      }
    }

    if (count == 0)
    {
      count = variable.value.empty() ? 1u : variable.value.size();
    }

    return count;
  }

  std::vector<std::string> extractArrayLiterals(const Semantico::Variable &variable)
  {
    std::vector<std::string> values;
    if (!variable.literalIsArray)
    {
      return values;
    }
    for (const auto &token : variable.value)
    {
      if (isIntegerLiteral(token))
      {
        values.push_back(token);
      }
    }
    return values;
  }

  std::string extractScalarLiteral(const Semantico::Variable &variable)
  {
    if (variable.literalIsArray)
    {
      return "";
    }
    std::string literal;
    for (const auto &token : variable.value)
    {
      if (token.empty())
      {
        continue;
      }
      if (isIntegerLiteral(token))
      {
        if (!literal.empty())
        {
          return "";
        }
        literal = token;
        continue;
      }
      if (token == "," || token == ";" || token == "=")
      {
        continue;
      }
      return "";
    }
    return literal;
  }

  void emitScalarStore(const std::string &name, const std::string &literal)
  {
    instructions.push_back("LDI " + literal);
    instructions.push_back("STO " + name);
    instructions.push_back("STO " + name);
  }

  std::string buildCode()
  {
    std::ostringstream out;
    out << ".data\n";
    for (const auto &entry : entries)
    {
      out << "  " << entry.name << ": ";
      if (entry.isArray)
      {
        const std::size_t length = entry.elementCount > 0 ? entry.elementCount : 1;
        for (std::size_t idx = 0; idx < length; ++idx)
        {
          if (idx > 0)
          {
            out << ",";
          }
          out << "0";
        }
      }
      else
      {
        if (entry.hasInitializer && !entry.literalValues.empty())
        {
          out << entry.literalValues.front();
        }
        else
        {
          out << "0";
        }
      }
      out << "\n";
    }
    std::vector<std::string> textInstructions = instructions;
    std::sort(statementInstructions.begin(), statementInstructions.end(),
              [](const auto &a, const auto &b) { return a.first < b.first; });
    for (const auto &stmt : statementInstructions)
    {
      textInstructions.insert(textInstructions.end(), stmt.second.begin(), stmt.second.end());
    }

    out << ".text\n";
    if (textInstructions.empty())
    {
      out << "  HLT 0\n";
    }
    else
    {
      for (const auto &instr : textInstructions)
      {
        out << "  " << instr << "\n";
      }
      out << "  HLT 0\n";
    }

    return out.str();
  }
}

namespace BipGenerator
{

  void reset()
  {
    entries.clear();
    cachedCode.clear();
    instructions.clear();
    statementInstructions.clear();
    readStatementsScanned = false;
  }

  void registerDeclaration(const Semantico::Variable &variable)
  {
    if (variable.name.empty())
    {
      return;
    }
    if (variable.type != Semantico::Type::INT)
    {
      return;
    }

    Entry entry;
    entry.name = variable.name;
    entry.isArray = variable.isArray;
    entry.elementCount = entry.isArray ? inferArrayLength(variable) : 1;
    if (variable.isInitialized)
    {
      if (entry.isArray)
      {
        entry.literalValues = extractArrayLiterals(variable);
        entry.hasInitializer = !entry.literalValues.empty();
      }
      else
      {
        std::string literal = extractScalarLiteral(variable);
        if (!literal.empty())
        {
          entry.literalValues.push_back(std::move(literal));
          entry.hasInitializer = true;
        }
      }
    }

    entries.push_back(std::move(entry));
    if (entries.back().isArray && !entries.back().literalValues.empty())
    {
      const std::size_t count = entries.back().literalValues.size();
      entries.back().elementCount = count;
      for (std::size_t idx = 0; idx < count; ++idx)
      {
        instructions.push_back("LDI " + std::to_string(static_cast<int>(idx)));
        instructions.push_back("STO $indr");
        instructions.push_back("LDI " + entries.back().literalValues[idx]);
        instructions.push_back("STOV " + entries.back().name);
      }
    }
  }

  void registerAssignment(const Semantico::Variable &variable)
  {
    if (variable.name.empty())
    {
      return;
    }
    ParsedAssignment parsed;

    try
    {
      if (!parseAssignmentFromSource(variable, parsed))
      {
        if (!variable.value.empty())
        {
          std::string literal = extractScalarLiteral(variable);
          if (!literal.empty())
          {
            emitScalarStore(variable.name, literal);
          }
        }
        return;
      }
    }
    catch (const std::exception &)
    {
      if (!variable.value.empty())
      {
        std::string literal = extractScalarLiteral(variable);
        if (!literal.empty())
        {
          emitScalarStore(variable.name, literal);
        }
      }
      return;
    }

    try
    {
      std::vector<std::string> code;

      if (!parsed.targetIsArray && parsed.rhsExpr)
      {
        const Expr &expr = *parsed.rhsExpr;
        auto emitAndStore = [&](std::vector<std::string> generated) {
          statementInstructions.emplace_back(parsed.statementStart, std::move(generated));
        };

        if (expr.kind == Expr::Kind::ArrayAccess && expr.index)
        {
          std::vector<std::string> direct;
          if (expr.index->kind == Expr::Kind::Literal)
          {
            direct.push_back("LDI " + expr.index->value);
          }
          else if (expr.index->kind == Expr::Kind::Variable)
          {
            direct.push_back("LD " + expr.index->value);
          }
          if (!direct.empty())
          {
            direct.push_back("STO $indr");
            direct.push_back("LDV " + expr.value);
            direct.push_back("STO " + parsed.targetName);
            emitAndStore(std::move(direct));
            return;
          }
        }

        if (expr.kind == Expr::Kind::Literal)
        {
          code.push_back("LDI " + expr.value);
          code.push_back("STO " + parsed.targetName);
          emitAndStore(std::move(code));
          return;
        }
        if (expr.kind == Expr::Kind::Variable)
        {
          code.push_back("LD " + expr.value);
          code.push_back("STO " + parsed.targetName);
          emitAndStore(std::move(code));
          return;
        }
        if (emitOptimizedAddSub(expr, parsed.targetName, code))
        {
          emitAndStore(std::move(code));
          return;
        }
      }

      ExpressionEmitter emitter(code);
      emitter.reset();
      bool symbolIsArray = variable.isArray;

      if (!parsed.rhsArrayElements.empty())
      {
        const bool wholeArray = parsed.targetIsArray || symbolIsArray;
        if (!wholeArray)
        {
          throw std::runtime_error("Atribuição de vetor requer destino vetor");
        }
        for (std::size_t idx = 0; idx < parsed.rhsArrayElements.size(); ++idx)
        {
          std::string elementTemp = emitter.emit(*parsed.rhsArrayElements[idx]);
          code.push_back("LDI " + std::to_string(static_cast<int>(idx)));
          code.push_back(std::string("STO ") + TEMP_VECTOR_INDEX);
          code.push_back(std::string("LD ") + TEMP_VECTOR_INDEX);
          code.push_back("STO $indr");
          code.push_back("LD " + elementTemp);
          code.push_back("STOV " + parsed.targetName);
        }
        statementInstructions.emplace_back(parsed.statementStart, std::move(code));
        return;
      }

      if (!parsed.rhsExpr)
      {
        throw std::runtime_error("Expressão de atribuição ausente");
      }

      if (parsed.targetIsArray)
      {
        if (!parsed.targetIndex)
        {
          throw std::runtime_error("Índice de vetor ausente");
        }
        std::string indexTemp = emitter.emit(*parsed.targetIndex);
        code.push_back("LD " + indexTemp);
        code.push_back(std::string("STO ") + TEMP_VECTOR_INDEX);

        std::string valueTemp = emitter.emit(*parsed.rhsExpr);
        code.push_back("LD " + valueTemp);
        code.push_back(std::string("STO ") + TEMP_VECTOR_VALUE);
        code.push_back("LD " + indexTemp);
        code.push_back(std::string("STO ") + TEMP_VECTOR_INDEX);
        code.push_back(std::string("LD ") + TEMP_VECTOR_INDEX);
        code.push_back("STO $indr");
        code.push_back(std::string("LD ") + TEMP_VECTOR_VALUE);
        code.push_back("STOV " + parsed.targetName);
      }
      else
      {
        std::string valueTemp = emitter.emit(*parsed.rhsExpr);
        code.push_back("LD " + valueTemp);
        code.push_back("STO " + parsed.targetName);
      }

      statementInstructions.emplace_back(parsed.statementStart, std::move(code));
    }
    catch (const std::exception &)
    {
      // falha no parse/geração: omite instruções para evitar código incorreto
    }
  }

  void registerReadStatement(std::size_t position, const std::string &argument)
  {
    for (const auto &entry : statementInstructions)
    {
      if (entry.first == position)
        return;
    }
    try
    {
      std::vector<std::string> code;
      auto expr = parseExpressionString(argument);
      generateReadIntoExpression(*expr, code);
      statementInstructions.emplace_back(position, std::move(code));
    }
    catch (const std::exception &)
    {
      // ignore leituras inválidas
    }
  }

  void registerPrintStatement(std::size_t position, const std::string &expression)
  {
    for (const auto &entry : statementInstructions)
    {
      if (entry.first == position)
        return;
    }
    try
    {
      std::vector<std::string> code;
      auto expr = parseExpressionString(expression);
      generatePrintExpression(*expr, code);
      statementInstructions.emplace_back(position, std::move(code));
    }
    catch (const std::exception &)
    {
      // ignore prints inválidos
    }
  }

  void scanReadStatements()
  {
    if (readStatementsScanned)
      return;
    readStatementsScanned = true;

    const std::string &src = Semantico::sourceCode;
    const std::string keyword = "read";
    const std::size_t len = src.size();

    for (std::size_t i = 0; i + keyword.size() < len; ++i)
    {
      if (src.compare(i, keyword.size(), keyword) != 0)
        continue;
      if (i > 0 && (std::isalnum(static_cast<unsigned char>(src[i - 1])) || src[i - 1] == '_'))
        continue;
      std::size_t j = i + keyword.size();
      while (j < len && std::isspace(static_cast<unsigned char>(src[j])))
        ++j;
      if (j >= len || src[j] != '(')
        continue;
      std::size_t close = findMatchingParenthesis(src, j);
      if (close == std::string::npos)
        continue;
      std::string argument = trim(src.substr(j + 1, close - j - 1));
      if (argument.empty())
        continue;
      registerReadStatement(i, argument);
      i = close;
    }
  }

  std::string render()
  {
    scanReadStatements();
    cachedCode = buildCode();
    return cachedCode;
  }

  const std::string &lastCode()
  {
    return cachedCode;
  }

  void writeToFile(const std::string &code)
  {
    std::ofstream file(OUTPUT_FILE);
    if (!file)
    {
      return;
    }
    file << code;
  }
}
