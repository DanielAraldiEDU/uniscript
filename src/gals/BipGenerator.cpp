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
#include <stdio.h>
#include <unordered_map>
#include <unordered_set>

namespace
{
  // Reservamos temporários para expressões bem abaixo do limite de 1023 do BIP
  // e longe dos registradores auxiliares 1000/1001/1002.
  constexpr const int TEMP_BASE_ADDRESS = 900;
  constexpr const int TEMP_MAX_ADDRESS = 999;
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
  static bool controlFlowGenerated = false;
  static std::size_t labelCounter = 1;
  struct AliasEntry
  {
    std::string original;
    std::string alias;
    int scopeDepth = 0;
    int position = -1;
  };
  std::vector<AliasEntry> aliasEntries;
  std::unordered_map<std::string, int> aliasCounters;
  std::unordered_set<std::string> seenPrints;

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

  // Converte binário para decimal
  int binaryToDecimal(const std::string &binary)
  {
    int decimal = 0;
    for (char c : binary)
    {
      if (c == '0' || c == '1')
      {
        decimal = decimal * 2 + (c - '0');
      }
    }
    return decimal;
  }

  // Verifica se string é um número binário
  bool isBinaryLiteral(const std::string &str)
  {
    if (str.size() < 3)
      return false;
    if (!(str[0] == '0' && (str[1] == 'b' || str[1] == 'B')))
      return false;
    for (std::size_t i = 2; i < str.size(); ++i)
    {
      char c = str[i];
      if (c != '0' && c != '1')
        return false;
    }
    return true;
  }

  // Converte literal (pode ser binário) para decimal
  std::string convertLiteralToDecimal(const std::string &literal)
  {
    if (isBinaryLiteral(literal))
    {
      return std::to_string(binaryToDecimal(literal.substr(2)));
    }
    return literal;
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

  std::size_t skipWhitespaceAndComments(const std::string &src, std::size_t pos)
  {
    const std::size_t len = src.size();
    while (pos < len)
    {
      char c = src[pos];
      if (std::isspace(static_cast<unsigned char>(c)))
      {
        ++pos;
        continue;
      }
      if (c == '/' && pos + 1 < len)
      {
        char next = src[pos + 1];
        if (next == '/')
        {
          pos += 2;
          while (pos < len && src[pos] != '\n')
            ++pos;
          continue;
        }
        if (next == '*')
        {
          pos += 2;
          while (pos + 1 < len && !(src[pos] == '*' && src[pos + 1] == '/'))
            ++pos;
          if (pos + 1 < len)
            pos += 2;
          continue;
        }
      }
      break;
    }
    return pos;
  }

  std::size_t findMatchingBrace(const std::string &src, std::size_t openPos)
  {
    if (openPos >= src.size() || src[openPos] != '{')
      return std::string::npos;
    int depth = 0;
    const std::size_t len = src.size();
    for (std::size_t idx = openPos; idx < len; ++idx)
    {
      char c = src[idx];
      if (c == '"' || c == '\'')
      {
        const char delim = c;
        ++idx;
        while (idx < len)
        {
          if (src[idx] == '\\' && idx + 1 < len)
          {
            idx += 2;
            continue;
          }
          if (src[idx] == delim)
            break;
          ++idx;
        }
        continue;
      }
      if (c == '/' && idx + 1 < len)
      {
        if (src[idx + 1] == '/')
        {
          idx += 2;
          while (idx < len && src[idx] != '\n')
            ++idx;
          continue;
        }
        if (src[idx + 1] == '*')
        {
          idx += 2;
          while (idx + 1 < len && !(src[idx] == '*' && src[idx + 1] == '/'))
            ++idx;
          continue;
        }
      }
      if (c == '{')
      {
        ++depth;
      }
      else if (c == '}')
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

  bool isKeywordAt(const std::string &src, std::size_t pos, const std::string &keyword)
  {
    const std::size_t len = src.size();
    if (pos + keyword.size() > len)
      return false;
    if (src.compare(pos, keyword.size(), keyword) != 0)
      return false;
    auto isIdentChar = [](char ch) { return std::isalnum(static_cast<unsigned char>(ch)) || ch == '_'; };
    if (pos > 0 && isIdentChar(src[pos - 1]))
      return false;
    const std::size_t after = pos + keyword.size();
    if (after < len && isIdentChar(src[after]))
      return false;
    return true;
  }

  std::string nextLabel()
  {
    return "R" + std::to_string(labelCounter++);
  }

  int scopeDepthAt(std::size_t position)
  {
    const std::string &src = Semantico::sourceCode;
    const std::size_t len = std::min(position, src.size());
    int depth = 0;
    bool inString = false;
    char stringDelim = 0;
    bool inLineComment = false;
    bool inBlockComment = false;

    for (std::size_t i = 0; i < len; ++i)
    {
      char c = src[i];
      if (inString)
      {
        if (c == '\\' && i + 1 < len)
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
      if (inLineComment)
      {
        if (c == '\n')
          inLineComment = false;
        continue;
      }
      if (inBlockComment)
      {
        if (c == '*' && i + 1 < len && src[i + 1] == '/')
        {
          inBlockComment = false;
          ++i;
        }
        continue;
      }
      if (c == '/' && i + 1 < len)
      {
        if (src[i + 1] == '/')
        {
          inLineComment = true;
          ++i;
          continue;
        }
        if (src[i + 1] == '*')
        {
          inBlockComment = true;
          ++i;
          continue;
        }
      }
      if (c == '"' || c == '\'')
      {
        inString = true;
        stringDelim = c;
        continue;
      }
      if (c == '{')
        ++depth;
      else if (c == '}')
        depth = std::max(0, depth - 1);
    }
    return depth;
  }

  bool isInsideForHeader(std::size_t position)
  {
    const std::string &src = Semantico::sourceCode;
    if (position >= src.size())
      return false;
    // procura um "for" antes da posição
    std::size_t searchPos = position;
    while (searchPos > 0)
    {
      --searchPos;
      if (!std::isspace(static_cast<unsigned char>(src[searchPos])))
        break;
    }

    for (std::size_t i = searchPos + 1; i > 0; --i)
    {
      std::size_t idx = i - 1;
      if (src[idx] != 'f')
        continue;
      if (idx + 3 > src.size())
        continue;
      if (!isKeywordAt(src, idx, "for"))
        continue;
      std::size_t parenPos = skipWhitespaceAndComments(src, idx + 3);
      if (parenPos >= src.size() || src[parenPos] != '(')
        continue;
      std::size_t parenClose = findMatchingParenthesis(src, parenPos);
      if (parenClose == std::string::npos)
        continue;
      if (position > parenPos && position < parenClose)
        return true;
      if (position <= idx)
        break;
    }
    return false;
  }

  int depthForPosition(std::size_t position)
  {
    int depth = scopeDepthAt(position);
    if (isInsideForHeader(position))
    {
      depth += 1;
    }
    return depth;
  }

  std::string makeAlias(const std::string &name, int scopeDepth)
  {
    const std::string base = name + "_s" + std::to_string(scopeDepth);
    int count = ++aliasCounters[base];
    if (count == 1)
      return base;
    return base + "_" + std::to_string(count);
  }

  std::string registerAlias(const std::string &name, int position)
  {
    int depth = depthForPosition(static_cast<std::size_t>(std::max(0, position)));
    std::string alias = makeAlias(name, depth);
    aliasEntries.push_back({name, alias, depth, position});
    return alias;
  }

  std::string resolveAlias(const std::string &name, std::size_t refPos)
  {
    int refDepth = depthForPosition(refPos);
    const AliasEntry *best = nullptr;
    for (const auto &entry : aliasEntries)
    {
      if (entry.original != name)
        continue;
      if (entry.scopeDepth > refDepth)
        continue;
      if (!best)
      {
        best = &entry;
        continue;
      }
      if (entry.scopeDepth > best->scopeDepth)
      {
        best = &entry;
      }
      else if (entry.scopeDepth == best->scopeDepth && entry.position > best->position)
      {
        best = &entry;
      }
    }
    if (best)
      return best->alias;
    return name;
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
    std::string op; // Mudado de char para string para suportar operadores de 2 caracteres
    ExprPtr left;
    ExprPtr right;
    ExprPtr index;

    static ExprPtr makeLiteral(std::string literal)
    {
      auto node = std::make_unique<Expr>();
      node->kind = Kind::Literal;
      // Converte binário para decimal se necessário
      node->value = convertLiteralToDecimal(literal);
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

    static ExprPtr makeBinary(std::string operation, ExprPtr lhs, ExprPtr rhs)
    {
      auto node = std::make_unique<Expr>();
      node->kind = Kind::Binary;
      node->op = std::move(operation);
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

      // Verifica operadores de 2 caracteres
      if (pos + 1 < length)
      {
        std::string twoChar = text.substr(pos, 2);
        if (twoChar == "<<" || twoChar == ">>")
        {
          pos += 2;
          return Token{Token::Type::Operator, twoChar};
        }
      }

      ++pos;
      switch (c)
      {
      case '+':
      case '-':
      case '&':
      case '|':
      case '^':
      case '~':
      case '*':
      case '/':
      case '%':
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
        std::string op = previous.lexeme;
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

    auto fallbackFind = [&]() -> std::size_t {
      if (variable.name.empty())
        return std::string::npos;
      std::size_t pos = 0;
      while (true)
      {
        pos = src.find(variable.name, pos);
        if (pos == std::string::npos)
          return std::string::npos;
        // garante que não está no meio de outro identificador
        auto isIdent = [](char ch) { return std::isalnum(static_cast<unsigned char>(ch)) || ch == '_'; };
        if ((pos > 0 && isIdent(src[pos - 1])) || (pos + variable.name.size() < src.size() && isIdent(src[pos + variable.name.size()])))
        {
          pos += variable.name.size();
          continue;
        }
        std::size_t eq = src.find('=', pos + variable.name.size());
        if (eq == std::string::npos)
          return std::string::npos;
        std::size_t semi = src.find(';', eq);
        if (semi == std::string::npos)
          return std::string::npos;
        return pos;
      }
    };

    if (start == std::string::npos || start >= src.size())
    {
      start = fallbackFind();
      if (start == std::string::npos || start >= src.size())
        return false;
    }

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

  std::string opcodeForOperator(const std::string &op)
  {
    if (op == "+") return "ADD";
    if (op == "-") return "SUB";
    if (op == "*") return "MUL"; 
    if (op == "/") return "DIV"; 
    if (op == "%") return "MOD";
    if (op == "&") return "AND";
    if (op == "|") return "OR";
    if (op == "^") return "XOR";
    if (op == "<<") return "SLL";
    if (op == ">>") return "SRL";
    if (op == "~") return "NOT";
    
    throw std::runtime_error(std::string("Operador não suportado: ") + op);
  }

  class ExpressionEmitter
  {
  public:
    ExpressionEmitter(std::vector<std::string> &instructionsRef, std::size_t referencePos)
        : instructions(instructionsRef), refPos(referencePos)
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
        std::string decimalValue = convertLiteralToDecimal(expr.value);
        instructions.push_back("LDI " + decimalValue);
        instructions.push_back("STO " + temp);
        return temp;
      }
      case Expr::Kind::Variable:
      {
        std::string temp = allocateTemp();
        const std::string name = resolveName(expr.value);
        instructions.push_back("LD " + name);
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
        const std::string arrayName = resolveName(expr.value);
        instructions.push_back("LDV " + arrayName);
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
        
        // Para operadores bit a bit e shifts, precisamos tratar de forma especial
        if (expr.op == "<<" || expr.op == ">>")
        {
          // Para shifts: primeiro operando no ACC, segundo como operando imediato
          std::string leftTemp = emit(*expr.left);
          
          // Se o operando direito for literal, podemos usar diretamente
          if (expr.right->kind == Expr::Kind::Literal)
          {
            instructions.push_back("LD " + leftTemp);
            std::string decimalValue = convertLiteralToDecimal(expr.right->value);
            instructions.push_back(opcodeForOperator(expr.op) + " " + decimalValue);
            std::string temp = allocateTemp();
            instructions.push_back("STO " + temp);
            return temp;
          }
          else
          {
            // Se não for literal, precisamos avaliar e usar como operando
            std::string rightTemp = emit(*expr.right);
            instructions.push_back("LD " + leftTemp);
            instructions.push_back(opcodeForOperator(expr.op) + " " + rightTemp);
            std::string temp = allocateTemp();
            instructions.push_back("STO " + temp);
            return temp;
          }
        }
        
        // Para outros operadores binários
        std::string rightTemp = emit(*expr.right);
        std::string leftTemp = emit(*expr.left);
        
        // Operações bit a bit usam instruções com operando de memória ou imediato
        if (expr.op == "&" || expr.op == "|" || expr.op == "^")
        {
          // Se o operando direito for literal, usar versão imediata
          if (expr.right->kind == Expr::Kind::Literal)
          {
            instructions.push_back("LD " + leftTemp);
            std::string decimalValue = convertLiteralToDecimal(expr.right->value);
            instructions.push_back(opcodeForOperator(expr.op) + "I " + decimalValue);
            std::string temp = allocateTemp();
            instructions.push_back("STO " + temp);
            return temp;
          }
          else
          {
            instructions.push_back("LD " + leftTemp);
            instructions.push_back(opcodeForOperator(expr.op) + " " + rightTemp);
            std::string temp = allocateTemp();
            instructions.push_back("STO " + temp);
            return temp;
          }
        }
        
        // Operadores aritméticos padrão
        instructions.push_back("LD " + leftTemp);
        instructions.push_back(opcodeForOperator(expr.op) + " " + rightTemp);
        std::string temp = allocateTemp();
        instructions.push_back("STO " + temp);
        return temp;
      }
      }
      throw std::runtime_error("Expressão desconhecida");
    }

    std::size_t position() const { return refPos; }
    std::string resolveSymbol(const std::string &name) const { return resolveName(name); }

  private:
    std::vector<std::string> &instructions;
    int nextTemp = TEMP_BASE_ADDRESS;
    std::size_t refPos = 0;

    std::string allocateTemp()
    {
      if (nextTemp > TEMP_MAX_ADDRESS)
      {
        throw std::runtime_error("Sem temporários disponíveis para expressão");
      }
      return std::to_string(nextTemp++);
    }

    std::string resolveName(const std::string &name) const
    {
      if (name.empty())
        return name;
      return resolveAlias(name, refPos);
    }
  };

  bool collectAddSubTerms(const Expr &expr, std::vector<std::pair<int, const Expr *>> &terms, int sign);

  bool emitIndexValue(const Expr &expr, std::vector<std::string> &code, ExpressionEmitter &emitter)
  {
    auto emitDirect = [&](const Expr &node) -> bool {
      if (node.kind == Expr::Kind::Literal)
      {
        std::string decimalValue = convertLiteralToDecimal(node.value);
        code.push_back("LDI " + decimalValue);
        return true;
      }
      if (node.kind == Expr::Kind::Variable)
      {
        const std::string name = emitter.resolveSymbol(node.value);
        code.push_back("LD " + name);
        return true;
      }
      return false;
    };

    if (emitDirect(expr))
    {
      return true;
    }

    std::vector<std::pair<int, const Expr *>> terms;
    if (collectAddSubTerms(expr, terms, 1) && !terms.empty())
    {
      bool onlySimple = true;
      for (const auto &term : terms)
      {
        auto kind = term.second->kind;
        if (kind != Expr::Kind::Literal && kind != Expr::Kind::Variable)
        {
          onlySimple = false;
          break;
        }
      }
      if (onlySimple)
      {
        int firstPositive = -1;
        for (std::size_t i = 0; i < terms.size(); ++i)
        {
          if (terms[i].first > 0)
          {
            firstPositive = static_cast<int>(i);
            break;
          }
        }
        if (firstPositive >= 0)
        {
          const Expr *firstTerm = terms[firstPositive].second;
          emitDirect(*firstTerm);
        }
        else
        {
          code.push_back("LDI 0");
        }
        for (std::size_t i = 0; i < terms.size(); ++i)
        {
          if (static_cast<int>(i) == firstPositive)
            continue;
          const Expr *termExpr = terms[i].second;
          bool positive = terms[i].first > 0;
          if (termExpr->kind == Expr::Kind::Literal)
          {
            std::string decimalValue = convertLiteralToDecimal(termExpr->value);
            code.push_back(std::string(positive ? "ADDI " : "SUBI ") + decimalValue);
          }
          else
          {
            const std::string name = emitter.resolveSymbol(termExpr->value);
            code.push_back(std::string(positive ? "ADD " : "SUB ") + name);
          }
        }
        if (firstPositive < 0)
        {
          // all terms were negativos: already handled in loop
        }
        return true;
      }
    }

    std::string temp = emitter.emit(expr);
    code.push_back("LD " + temp);
    return true;
  }

  bool emitArrayValueInto(const Expr &expr, const std::string &destination, std::vector<std::string> &code, ExpressionEmitter &emitter)
  {
    if (expr.kind != Expr::Kind::ArrayAccess || !expr.index)
    {
      return false;
    }
    if (!emitIndexValue(*expr.index, code, emitter))
    {
      return false;
    }
    code.push_back("STO $indr");
    const std::string name = emitter.resolveSymbol(expr.value);
    code.push_back("LDV " + name);
    code.push_back("STO " + destination);
    return true;
  }

  bool collectAddSubTerms(const Expr &expr, std::vector<std::pair<int, const Expr *>> &terms, int sign)
  {
    if (expr.kind == Expr::Kind::Binary && (expr.op == "+" || expr.op == "-"))
    {
      if (!collectAddSubTerms(*expr.left, terms, sign))
        return false;
      int rightSign = expr.op == "+" ? sign : -sign;
      return collectAddSubTerms(*expr.right, terms, rightSign);
    }

    if (expr.kind == Expr::Kind::Literal || expr.kind == Expr::Kind::Variable || expr.kind == Expr::Kind::ArrayAccess)
    {
      terms.push_back({sign, &expr});
      return true;
    }

    return false;
  }

  bool emitOptimizedAddSub(const Expr &expr, const std::string &target, std::vector<std::string> &code, std::size_t refPos)
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

    ExpressionEmitter emitter(code, refPos);
    emitter.reset();
    const std::string running = TEMP_VECTOR_VALUE;
    const std::string scratch = TEMP_VECTOR_VALUE_ALT;

    auto storeTermInto = [&](const std::string &dest, const Expr *node) -> bool {
      if (node->kind == Expr::Kind::Literal)
      {
        std::string decimalValue = convertLiteralToDecimal(node->value);
        code.push_back("LDI " + decimalValue);
        code.push_back("STO " + dest);
        return true;
      }
      if (node->kind == Expr::Kind::Variable)
      {
        const std::string name = emitter.resolveSymbol(node->value);
        code.push_back("LD " + name);
        code.push_back("STO " + dest);
        return true;
      }
      if (node->kind == Expr::Kind::ArrayAccess)
      {
        return emitArrayValueInto(*node, dest, code, emitter);
      }
      std::string temp = emitter.emit(*node);
      code.push_back("LD " + temp);
      code.push_back("STO " + dest);
      return true;
    };

    auto applyTerm = [&](int sign, const Expr *node) -> bool {
      const bool positive = sign > 0;
      if (node->kind == Expr::Kind::Literal)
      {
        code.push_back("LD " + running);
        std::string decimalValue = convertLiteralToDecimal(node->value);
        code.push_back(std::string(positive ? "ADDI " : "SUBI ") + decimalValue);
        code.push_back("STO " + running);
        return true;
      }
      if (node->kind == Expr::Kind::Variable)
      {
        const std::string name = emitter.resolveSymbol(node->value);
        code.push_back("LD " + running);
        code.push_back(std::string(positive ? "ADD " : "SUB ") + name);
        code.push_back("STO " + running);
        return true;
      }
      if (!storeTermInto(scratch, node))
      {
        return false;
      }
      code.push_back("LD " + running);
      code.push_back(std::string(positive ? "ADD " : "SUB ") + scratch);
      code.push_back("STO " + running);
      return true;
    };

    if (firstIndex >= 0)
    {
      if (!storeTermInto(running, terms[firstIndex].second))
      {
        return false;
      }
    }
    else
    {
      code.push_back("LDI 0");
      code.push_back("STO " + running);
    }

    for (std::size_t i = 0; i < terms.size(); ++i)
    {
      if (static_cast<int>(i) == firstIndex)
        continue;
      if (!applyTerm(terms[i].first, terms[i].second))
        return false;
    }

    code.push_back("LD " + running);
    code.push_back("STO " + target);
    return true;
  }

  void generateReadIntoExpression(const Expr &expr, std::vector<std::string> &out, std::size_t refPos)
  {
    ExpressionEmitter emitter(out, refPos);
    emitter.reset();
    if (expr.kind == Expr::Kind::Variable)
    {
      const std::string name = emitter.resolveSymbol(expr.value);
      out.push_back("LD $in_port");
      out.push_back("STO " + name);
      return;
    }
    if (expr.kind == Expr::Kind::ArrayAccess && expr.index)
    {
      if (expr.index->kind == Expr::Kind::Literal)
      {
        std::string decimalValue = convertLiteralToDecimal(expr.index->value);
        out.push_back("LDI " + decimalValue);
      }
      else if (expr.index->kind == Expr::Kind::Variable)
      {
        const std::string name = emitter.resolveSymbol(expr.index->value);
        out.push_back("LD " + name);
      }
      else
      {
        std::string indexTemp = emitter.emit(*expr.index);
        out.push_back("LD " + indexTemp);
      }
      out.push_back("STO $indr");
      out.push_back("LD $in_port");
      const std::string arrayName = emitter.resolveSymbol(expr.value);
      out.push_back("STOV " + arrayName);
      return;
    }
    throw std::runtime_error("Destino inválido para leitura");
  }

  void generatePrintExpression(const Expr &expr, std::vector<std::string> &out, std::size_t refPos)
  {
    ExpressionEmitter emitter(out, refPos);
    emitter.reset();

    auto emitArrayIndex = [&](const Expr &index) {
      if (index.kind == Expr::Kind::Literal)
      {
        std::string decimalValue = convertLiteralToDecimal(index.value);
        out.push_back("LDI " + decimalValue);
        return;
      }
      if (index.kind == Expr::Kind::Variable)
      {
        const std::string name = emitter.resolveSymbol(index.value);
        out.push_back("LD " + name);
        return;
      }
      std::string indexTemp = emitter.emit(index);
      out.push_back("LD " + indexTemp);
    };

    if (expr.kind == Expr::Kind::ArrayAccess)
    {
      if (!expr.index)
      {
        throw std::runtime_error("Acesso a vetor sem índice na impressão");
      }
      emitArrayIndex(*expr.index);
      out.push_back("STO $indr");
      const std::string arrayName = emitter.resolveSymbol(expr.value);
      out.push_back("LDV " + arrayName);
      out.push_back("STO $out_port");
      return;
    }

    if (expr.kind == Expr::Kind::Literal)
    {
      std::string decimalValue = convertLiteralToDecimal(expr.value);
      out.push_back("LDI " + decimalValue);
      out.push_back("STO $out_port");
      return;
    }

    if (expr.kind == Expr::Kind::Variable)
    {
      const std::string name = emitter.resolveSymbol(expr.value);
      out.push_back("LD " + name);
      out.push_back("STO $out_port");
      return;
    }

    // Demais expressões: avalia e imprime o resultado temporário
    std::string temp = emitter.emit(expr);
    out.push_back("LD " + temp);
    out.push_back("STO $out_port");
  }

  struct RelationalParts
  {
    std::string left;
    std::string op;
    std::string right;
  };

  bool splitRelationalExpression(const std::string &exprText, RelationalParts &out)
  {
    int depthParen = 0;
    int depthBracket = 0;
    int depthBrace = 0;
    bool inString = false;
    char stringDelim = 0;
    const std::size_t len = exprText.size();

    for (std::size_t i = 0; i < len; ++i)
    {
      char c = exprText[i];
      if (inString)
      {
        if (c == '\\' && i + 1 < len)
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
      if (depthParen || depthBracket || depthBrace)
        continue;

      if ((c == '<' || c == '>') && i + 1 < len && exprText[i + 1] == c)
      {
        ++i;
        continue;
      }

      if (c == '<' || c == '>' || c == '!' || c == '=')
      {
        std::size_t opLen = 1;
        std::string op(1, c);
        if (i + 1 < len && exprText[i + 1] == '=')
        {
          opLen = 2;
          op = exprText.substr(i, 2);
        }
        else if (c == '!' || c == '=')
        {
          continue;
        }

        std::size_t rightStart = i + opLen;
        if ((op == "==" || op == "!=") && rightStart < len && exprText[rightStart] == '=')
        {
          ++rightStart; // trata === e !== como == e !=
        }

        out.left = trim(exprText.substr(0, i));
        out.right = trim(exprText.substr(rightStart));
        out.op = op;
        return !out.left.empty() && !out.right.empty();
      }
    }

    return false;
  }

  std::string branchOpcodeFor(const std::string &op, bool invert)
  {
    const std::string normalized = (op == "===") ? "==" : (op == "!==") ? "!=" : op;
    if (!invert)
    {
      if (normalized == "<")
        return "BLT";
      if (normalized == ">")
        return "BGT";
      if (normalized == "<=")
        return "BLE";
      if (normalized == ">=")
        return "BGE";
      if (normalized == "==")
        return "BEQ";
      if (normalized == "!=")
        return "BNE";
    }
    else
    {
      if (normalized == "<")
        return "BGE";
      if (normalized == ">")
        return "BLE";
      if (normalized == "<=")
        return "BGT";
      if (normalized == ">=")
        return "BLT";
      if (normalized == "==")
        return "BNE";
      if (normalized == "!=")
        return "BEQ";
    }
    throw std::runtime_error("Operador relacional não suportado: " + op);
  }

  std::vector<std::string> emitRelationalJump(const std::string &conditionText, const std::string &targetLabel, bool invert, std::size_t refPos)
  {
    RelationalParts parts;
    if (!splitRelationalExpression(conditionText, parts))
    {
      return {};
    }

    std::vector<std::string> code;
    try
    {
      auto leftExpr = parseExpressionString(parts.left);
      auto rightExpr = parseExpressionString(parts.right);

      ExpressionEmitter emitter(code, refPos);
      emitter.reset();

      auto emitIntoTemp = [&](const Expr &expr, const std::string &temp) {
        if (expr.kind == Expr::Kind::Literal)
        {
          std::string decimalValue = convertLiteralToDecimal(expr.value);
          code.push_back("LDI " + decimalValue);
          code.push_back("STO " + temp);
          return;
        }
        if (expr.kind == Expr::Kind::Variable)
        {
          const std::string name = emitter.resolveSymbol(expr.value);
          code.push_back("LD " + name);
          code.push_back("STO " + temp);
          return;
        }
        if (expr.kind == Expr::Kind::ArrayAccess && expr.index)
        {
          std::vector<std::string> tmpCode;
          emitArrayValueInto(expr, temp, tmpCode, emitter);
          code.insert(code.end(), tmpCode.begin(), tmpCode.end());
          return;
        }
        std::string tmp = emitter.emit(expr);
        code.push_back("LD " + tmp);
        code.push_back("STO " + temp);
      };

      emitIntoTemp(*leftExpr, TEMP_VECTOR_VALUE);
      emitIntoTemp(*rightExpr, TEMP_VECTOR_VALUE_ALT);

      code.push_back(std::string("LD ") + TEMP_VECTOR_VALUE);
      code.push_back(std::string("SUB ") + TEMP_VECTOR_VALUE_ALT);
      const std::string opcode = branchOpcodeFor(parts.op, invert);
      code.push_back(opcode + " " + targetLabel);
    }
    catch (const std::exception &)
    {
      code.clear();
    }
    return code;
  }

  void addStatementBlock(std::size_t position, std::vector<std::string> code)
  {
    if (code.empty())
      return;
    statementInstructions.emplace_back(position, std::move(code));
  }

  void removeBlocksInRange(std::size_t start, std::size_t end)
  {
    if (start > end)
      return;
    statementInstructions.erase(
        std::remove_if(statementInstructions.begin(), statementInstructions.end(),
                       [&](const auto &entry) { return entry.first >= start && entry.first <= end; }),
        statementInstructions.end());
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
      else if (isBinaryLiteral(token))
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
      else if (isBinaryLiteral(token))
      {
        values.push_back(std::to_string(binaryToDecimal(token)));
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
      if (isBinaryLiteral(token))
      {
        if (!literal.empty())
        {
          return "";
        }
        literal = std::to_string(binaryToDecimal(token));
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

  void emitScalarStore(const std::string &name, const std::string &literal, std::size_t position)
  {
    std::string decimalValue = convertLiteralToDecimal(literal);
    std::vector<std::string> code;
    code.push_back("LDI " + decimalValue);
    code.push_back("STO " + name);
    // Se a posição não for válida, empurra para o final para não bagunçar fluxo
    const std::size_t safePos = position == std::string::npos ? std::numeric_limits<std::size_t>::max() - 1 : position;
    statementInstructions.emplace_back(safePos, std::move(code));
  }

  bool extractConditionAt(const std::string &src, std::size_t keywordPos, std::size_t keywordLen, std::string &condition, std::size_t &condOpen, std::size_t &condClose)
  {
    std::size_t pos = skipWhitespaceAndComments(src, keywordPos + keywordLen);
    if (pos >= src.size() || src[pos] != '(')
      return false;
    condOpen = pos;
    condClose = findMatchingParenthesis(src, condOpen);
    if (condClose == std::string::npos)
      return false;
    condition = trim(src.substr(condOpen + 1, condClose - condOpen - 1));
    return true;
  }

  bool extractBlockAfter(const std::string &src, std::size_t startPos, std::size_t &openBrace, std::size_t &closeBrace)
  {
    std::size_t pos = skipWhitespaceAndComments(src, startPos);
    if (pos >= src.size() || src[pos] != '{')
      return false;
    openBrace = pos;
    closeBrace = findMatchingBrace(src, openBrace);
    return closeBrace != std::string::npos;
  }

  std::vector<std::string> generateUpdateInstructions(const std::string &updateText, std::size_t refPos)
  {
    std::string trimmed = trim(updateText);
    if (trimmed.empty())
      return {};

    auto simpleIncrement = [&](const std::string &name, bool increment) {
      std::vector<std::string> code;
      const std::string alias = resolveAlias(name, refPos);
      code.push_back("LD " + alias);
      code.push_back(std::string(increment ? "ADDI " : "SUBI ") + "1");
      code.push_back("STO " + alias);
      return code;
    };

    if (trimmed.size() >= 2 && trimmed.substr(trimmed.size() - 2) == "++")
    {
      std::string name = trim(trimmed.substr(0, trimmed.size() - 2));
      if (name.empty())
        return {};
      return simpleIncrement(name, true);
    }
    if (trimmed.size() >= 2 && trimmed.substr(trimmed.size() - 2) == "--")
    {
      std::string name = trim(trimmed.substr(0, trimmed.size() - 2));
      if (name.empty())
        return {};
      return simpleIncrement(name, false);
    }
    if (trimmed.size() >= 2 && trimmed[0] == '+' && trimmed[1] == '+')
    {
      std::string name = trim(trimmed.substr(2));
      if (name.empty())
        return {};
      return simpleIncrement(name, true);
    }
    if (trimmed.size() >= 2 && trimmed[0] == '-' && trimmed[1] == '-')
    {
      std::string name = trim(trimmed.substr(2));
      if (name.empty())
        return {};
      return simpleIncrement(name, false);
    }

    int depthParen = 0;
    int depthBracket = 0;
    int depthBrace = 0;
    std::size_t assignPos = std::string::npos;
    for (std::size_t i = 0; i < trimmed.size(); ++i)
    {
      char c = trimmed[i];
      if (c == '(')
        ++depthParen;
      else if (c == ')')
        --depthParen;
      else if (c == '[')
        ++depthBracket;
      else if (c == ']')
        --depthBracket;
      else if (c == '{')
        ++depthBrace;
      else if (c == '}')
        --depthBrace;
      if (depthParen || depthBracket || depthBrace)
        continue;
      if (c == '=')
      {
        if (i > 0 && (trimmed[i - 1] == '<' || trimmed[i - 1] == '>' || trimmed[i - 1] == '='))
          continue;
        if (i + 1 < trimmed.size() && trimmed[i + 1] == '=')
          continue;
        assignPos = i;
        break;
      }
    }

    if (assignPos == std::string::npos)
      return {};

    const std::string targetText = trim(trimmed.substr(0, assignPos));
    const std::string rhsText = trim(trimmed.substr(assignPos + 1));
    if (targetText.empty() || rhsText.empty())
      return {};

    std::vector<std::string> code;
    try
    {
      bool targetIsArray = false;
      std::unique_ptr<Expr> targetIndex;
      std::string targetName = targetText;
      std::size_t bracketPos = targetText.find('[');
      if (bracketPos != std::string::npos)
      {
        std::size_t closeBracket = targetText.rfind(']');
        if (closeBracket != std::string::npos && closeBracket > bracketPos)
        {
          targetIsArray = true;
          targetName = trim(targetText.substr(0, bracketPos));
          const std::string indexExprText = trim(targetText.substr(bracketPos + 1, closeBracket - bracketPos - 1));
          if (!indexExprText.empty())
          {
            targetIndex = parseExpressionString(indexExprText);
          }
        }
      }

      auto rhsExpr = parseExpressionString(rhsText);
      const std::string targetAlias = resolveAlias(targetName, refPos);

      // Caminho otimizado: atualizações simples sem temporários extras.
      auto isSimple = [](const Expr &expr) {
        return expr.kind == Expr::Kind::Literal || expr.kind == Expr::Kind::Variable;
      };
      auto loadSimple = [&](const Expr &expr) -> void {
        if (expr.kind == Expr::Kind::Literal)
        {
          std::string decimalValue = convertLiteralToDecimal(expr.value);
          code.push_back("LDI " + decimalValue);
        }
        else if (expr.kind == Expr::Kind::Variable)
        {
          const std::string name = resolveAlias(expr.value, refPos);
          code.push_back("LD " + name);
        }
      };

      if (!targetIsArray)
      {
        if (rhsExpr->kind == Expr::Kind::Literal)
        {
          std::string decimalValue = convertLiteralToDecimal(rhsExpr->value);
          code.push_back("LDI " + decimalValue);
          code.push_back("STO " + targetAlias);
          return code;
        }
        if (rhsExpr->kind == Expr::Kind::Variable)
        {
          const std::string srcName = resolveAlias(rhsExpr->value, refPos);
          code.push_back("LD " + srcName);
          code.push_back("STO " + targetAlias);
          return code;
        }
        if (rhsExpr->kind == Expr::Kind::Binary && rhsExpr->left && rhsExpr->right &&
            (rhsExpr->op == "+" || rhsExpr->op == "-") &&
            isSimple(*rhsExpr->left) && isSimple(*rhsExpr->right))
        {
          loadSimple(*rhsExpr->left);

          const Expr &rightNode = *rhsExpr->right;
          if (rightNode.kind == Expr::Kind::Literal)
          {
            std::string decimalValue = convertLiteralToDecimal(rightNode.value);
            code.push_back(std::string(rhsExpr->op == "+" ? "ADDI " : "SUBI ") + decimalValue);
          }
          else
          {
            const std::string name = resolveAlias(rightNode.value, refPos);
            code.push_back(std::string(rhsExpr->op == "+" ? "ADD " : "SUB ") + name);
          }

          code.push_back("STO " + targetAlias);
          return code;
        }
      }

      // Caminho geral (usa temporários)
      ExpressionEmitter emitter(code, refPos);
      emitter.reset();
      std::string valueTemp = emitter.emit(*rhsExpr);

      if (targetIsArray && targetIndex)
      {
        std::string idxTemp = emitter.emit(*targetIndex);
        code.push_back("LD " + idxTemp);
        code.push_back(std::string("STO ") + TEMP_VECTOR_INDEX);
        code.push_back("LD " + valueTemp);
        code.push_back(std::string("STO ") + TEMP_VECTOR_VALUE);
        code.push_back(std::string("LD ") + TEMP_VECTOR_INDEX);
        code.push_back("STO $indr");
        code.push_back(std::string("LD ") + TEMP_VECTOR_VALUE);
        code.push_back("STOV " + targetAlias);
      }
      else
      {
        code.push_back("LD " + valueTemp);
        code.push_back("STO " + targetAlias);
      }
    }
    catch (const std::exception &)
    {
      code.clear();
    }
    return code;
  }

  struct ForHeaderInfo
  {
    std::string conditionText;
    std::string updateText;
    std::size_t condStart = 0;
    std::size_t condEnd = 0;
    std::size_t updateStart = 0;
    std::size_t updateEnd = 0;
  };

  bool parseForHeader(const std::string &src, std::size_t parenOpen, std::size_t parenClose, ForHeaderInfo &info)
  {
    if (parenOpen >= parenClose || parenClose >= src.size())
      return false;
    const std::size_t headerStart = parenOpen + 1;
    const std::size_t headerLen = parenClose - parenOpen - 1;
    const std::string header = src.substr(headerStart, headerLen);
    int depth = 0;
    std::size_t firstSemi = std::string::npos;
    std::size_t secondSemi = std::string::npos;
    for (std::size_t i = 0; i < header.size(); ++i)
    {
      char c = header[i];
      if (c == '(')
        ++depth;
      else if (c == ')')
        --depth;
      else if (c == '[')
        ++depth;
      else if (c == ']')
        --depth;
      if (depth == 0 && c == ';')
      {
        if (firstSemi == std::string::npos)
          firstSemi = i;
        else
        {
          secondSemi = i;
          break;
        }
      }
    }
    if (firstSemi == std::string::npos)
      return false;
    const std::size_t condStart = headerStart + firstSemi + 1;
    const std::size_t condEnd = (secondSemi == std::string::npos) ? (parenClose - 1) : (headerStart + secondSemi - 1);
    const std::size_t updateStart = (secondSemi == std::string::npos) ? condEnd : headerStart + secondSemi + 1;
    const std::size_t updateEnd = parenClose - 1;

    info.conditionText = trim(src.substr(condStart, condEnd - condStart + 1));
    info.updateText = trim(src.substr(updateStart, updateEnd - updateStart + 1));
    info.condStart = condStart;
    info.condEnd = condEnd;
    info.updateStart = updateStart;
    info.updateEnd = updateEnd;
    return true;
  }

  class ControlFlowGenerator
  {
  public:
    explicit ControlFlowGenerator(const std::string &source) : src(source), limit(source.size()) {}

    void parse(std::size_t start = 0, std::size_t end = std::string::npos)
    {
      if (end == std::string::npos || end > limit)
        end = limit;
      parseRange(start, end);
    }

  private:
    const std::string &src;
    const std::size_t limit;

    void parseRange(std::size_t start, std::size_t end)
    {
      std::size_t pos = start;
      while (pos < end)
      {
        pos = skipWhitespaceAndComments(src, pos);
        if (pos >= end)
          break;
        if (isKeywordAt(src, pos, "if"))
        {
          if (parseIf(pos, end))
            continue;
        }
        else if (isKeywordAt(src, pos, "while"))
        {
          if (parseWhile(pos, end))
            continue;
        }
        else if (isKeywordAt(src, pos, "do"))
        {
          if (parseDoWhile(pos, end))
            continue;
        }
        else if (isKeywordAt(src, pos, "for"))
        {
          if (parseFor(pos, end))
            continue;
        }
        ++pos;
      }
    }

    bool parseIf(std::size_t &pos, std::size_t end)
    {
      const std::size_t keywordPos = pos;
      std::string condition;
      std::size_t condOpen = 0;
      std::size_t condClose = 0;
      if (!extractConditionAt(src, keywordPos, 2, condition, condOpen, condClose))
      {
        ++pos;
        return false;
      }

      std::size_t bodyOpen = 0;
      std::size_t bodyClose = 0;
      if (!extractBlockAfter(src, condClose + 1, bodyOpen, bodyClose) || bodyClose > end)
      {
        ++pos;
        return false;
      }

      std::size_t afterBody = skipWhitespaceAndComments(src, bodyClose + 1);
      bool hasElse = afterBody < end && isKeywordAt(src, afterBody, "else");
      std::size_t elseOpen = std::string::npos;
      std::size_t elseClose = std::string::npos;

      if (hasElse)
      {
        std::size_t afterElse = skipWhitespaceAndComments(src, afterBody + 4);
        if (!extractBlockAfter(src, afterElse, elseOpen, elseClose))
        {
          // else-if sem bloco explícito: tratamos como ausência de else
          hasElse = false;
        }
      }

      std::string falseLabel = nextLabel();
      auto condInstr = emitRelationalJump(condition, falseLabel, true, condOpen);
      if (!condInstr.empty())
      {
        addStatementBlock(keywordPos, std::move(condInstr));
      }

      parseRange(bodyOpen + 1, bodyClose);

      if (!hasElse || elseOpen == std::string::npos || elseClose == std::string::npos)
      {
        addStatementBlock(bodyClose + 1, {falseLabel + ":"});
        pos = bodyClose + 1;
        return true;
      }

      std::string endLabel = nextLabel();
      addStatementBlock(bodyClose, {"JMP " + endLabel});
      addStatementBlock(elseOpen, {falseLabel + ":"});
      parseRange(elseOpen + 1, elseClose);
      addStatementBlock(elseClose + 1, {endLabel + ":"});
      pos = elseClose + 1;
      return true;
    }

    bool parseWhile(std::size_t &pos, std::size_t end)
    {
      const std::size_t keywordPos = pos;
      std::string condition;
      std::size_t condOpen = 0;
      std::size_t condClose = 0;
      if (!extractConditionAt(src, keywordPos, 5, condition, condOpen, condClose))
      {
        ++pos;
        return false;
      }

      std::size_t bodyOpen = 0;
      std::size_t bodyClose = 0;
      if (!extractBlockAfter(src, condClose + 1, bodyOpen, bodyClose) || bodyClose > end)
      {
        ++pos;
        return false;
      }

      std::string startLabel = nextLabel();
      std::string endLabel = nextLabel();

      addStatementBlock(keywordPos, {startLabel + ":"});
      auto condInstr = emitRelationalJump(condition, endLabel, true, condOpen);
      if (!condInstr.empty())
      {
        addStatementBlock(keywordPos + 1, std::move(condInstr));
      }

      parseRange(bodyOpen + 1, bodyClose);
      // Mantém o salto de repetição colado ao fechamento do bloco,
      // seguido imediatamente pelo rótulo de saída, para evitar que
      // instruções após o while caiam entre o JMP e o label.
      addStatementBlock(bodyClose, {"JMP " + startLabel});
      addStatementBlock(bodyClose, {endLabel + ":"});
      pos = bodyClose + 1;
      return true;
    }

    bool parseDoWhile(std::size_t &pos, std::size_t end)
    {
      const std::size_t keywordPos = pos;
      std::size_t blockOpen = 0;
      std::size_t blockClose = 0;
      std::size_t afterDo = skipWhitespaceAndComments(src, keywordPos + 2);
      if (!extractBlockAfter(src, afterDo, blockOpen, blockClose) || blockClose > end)
      {
        ++pos;
        return false;
      }

      std::string startLabel = nextLabel();
      addStatementBlock(keywordPos, {startLabel + ":"});
      parseRange(blockOpen + 1, blockClose);

      std::size_t afterBlock = skipWhitespaceAndComments(src, blockClose + 1);
      if (afterBlock >= end || !isKeywordAt(src, afterBlock, "while"))
      {
        pos = blockClose + 1;
        return true;
      }

      std::string condition;
      std::size_t condOpen = 0;
      std::size_t condClose = 0;
      if (!extractConditionAt(src, afterBlock, 5, condition, condOpen, condClose))
      {
        pos = blockClose + 1;
        return true;
      }

      auto condInstr = emitRelationalJump(condition, startLabel, false, condOpen);
      if (!condInstr.empty())
      {
        addStatementBlock(afterBlock, std::move(condInstr));
      }

      pos = condClose + 1;
      pos = skipWhitespaceAndComments(src, pos);
      if (pos < end && src[pos] == ';')
        ++pos;
      return true;
    }

    bool parseFor(std::size_t &pos, std::size_t end)
    {
      const std::size_t keywordPos = pos;
      std::size_t parenOpen = skipWhitespaceAndComments(src, keywordPos + 3);
      if (parenOpen >= end || src[parenOpen] != '(')
      {
        ++pos;
        return false;
      }
      std::size_t parenClose = findMatchingParenthesis(src, parenOpen);
      if (parenClose == std::string::npos || parenClose > end)
      {
        ++pos;
        return false;
      }

      ForHeaderInfo header;
      if (!parseForHeader(src, parenOpen, parenClose, header))
      {
        ++pos;
        return false;
      }

      std::size_t bodyOpen = 0;
      std::size_t bodyClose = 0;
      if (!extractBlockAfter(src, parenClose + 1, bodyOpen, bodyClose) || bodyClose > end)
      {
        ++pos;
        return false;
      }

      std::string startLabel = nextLabel();
      std::string endLabel = nextLabel();

      addStatementBlock(header.condStart, {startLabel + ":"});
      if (!header.conditionText.empty())
      {
        auto condInstr = emitRelationalJump(header.conditionText, endLabel, true, header.condStart);
        if (!condInstr.empty())
        {
          addStatementBlock(header.condStart + 1, std::move(condInstr));
        }
      }

      parseRange(bodyOpen + 1, bodyClose);

      removeBlocksInRange(header.updateStart, header.updateEnd);
      auto updateInstr = generateUpdateInstructions(header.updateText, header.updateStart);
      if (!updateInstr.empty())
      {
        const std::size_t updatePos = bodyClose > 0 ? bodyClose - 1 : bodyClose;
        addStatementBlock(updatePos, std::move(updateInstr));
      }

      addStatementBlock(bodyClose, {"JMP " + startLabel});
      addStatementBlock(bodyClose + 1, {endLabel + ":"});
      pos = bodyClose + 1;
      return true;
    }
  };

  void generateControlFlow()
  {
    if (controlFlowGenerated)
      return;
    controlFlowGenerated = true;
    ControlFlowGenerator parser(Semantico::sourceCode);
    parser.parse();
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
    std::stable_sort(statementInstructions.begin(), statementInstructions.end(),
                     [](const auto &a, const auto &b) { return a.first < b.first; });
    for (const auto &stmt : statementInstructions)
    {
      textInstructions.insert(textInstructions.end(), stmt.second.begin(), stmt.second.end());
    }

    textInstructions.insert(textInstructions.begin(), "main:");
    textInstructions.insert(textInstructions.begin(), "JMP main");

    out << ".text\n";
    if (textInstructions.empty())
    {
      out << "  HLT 0\n";
    }
    else
    {
      for (const auto &instr : textInstructions)
      {
        const bool isLabel = !instr.empty() && instr.back() == ':';
        if (isLabel)
        {
          out << instr << "\n";
        }
        else
        {
          out << "    " << instr << "\n";
        }
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
    controlFlowGenerated = false;
    labelCounter = 1;
    aliasEntries.clear();
    aliasCounters.clear();
    seenPrints.clear();
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

    const std::size_t declPos = variable.position >= 0 ? static_cast<std::size_t>(variable.position) : 0;
    const std::string alias = registerAlias(variable.name, static_cast<int>(declPos));

    Entry entry;
    entry.name = alias;
    entry.isArray = variable.isArray;
    entry.elementCount = entry.isArray ? inferArrayLength(variable) : 1;
    
    bool hasSimpleLiteral = false;
    
    if (variable.isInitialized)
    {
      if (entry.isArray)
      {
        entry.literalValues = extractArrayLiterals(variable);
        entry.hasInitializer = !entry.literalValues.empty();
        hasSimpleLiteral = entry.hasInitializer;
      }
      else
      {
        std::string literal = extractScalarLiteral(variable);
        if (!literal.empty())
        {
          entry.literalValues.push_back(std::move(literal));
          entry.hasInitializer = true;
          hasSimpleLiteral = true;
        }
      }
    }

    entries.push_back(std::move(entry));
    Entry &current = entries.back();
    
    // Inicialização de arrays com valores literais (inserida na posição da declaração)
    if (current.isArray && !current.literalValues.empty())
    {
      const std::size_t count = current.literalValues.size();
      current.elementCount = count;
      std::vector<std::string> code;
      for (std::size_t idx = 0; idx < count; ++idx)
      {
        code.push_back("LDI " + std::to_string(static_cast<int>(idx)));
        code.push_back("STO $indr");
        code.push_back("LDI " + current.literalValues[idx]);
        code.push_back("STOV " + current.name);
      }
      if (!code.empty())
      {
        statementInstructions.emplace_back(declPos, std::move(code));
      }
    }
    // Inicialização de variável escalar com literal simples
    else if (!current.isArray && current.hasInitializer && !current.literalValues.empty())
    {
      std::vector<std::string> code;
      code.push_back("LDI " + current.literalValues.front());
      code.push_back("STO " + current.name);
      statementInstructions.emplace_back(declPos, std::move(code));
    }
    // Se tem inicialização mas não é literal simples, tenta processar como atribuição
    else if (!current.isArray && variable.isInitialized && !hasSimpleLiteral)
    {
      // Tem inicialização com expressão complexa, processa como atribuição
      registerAssignment(variable);
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
            const std::size_t refPos = variable.position >= 0 ? static_cast<std::size_t>(variable.position) : 0;
            const std::string alias = resolveAlias(variable.name, refPos);
            emitScalarStore(alias, literal, refPos);
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
          const std::size_t refPos = variable.position >= 0 ? static_cast<std::size_t>(variable.position) : 0;
          const std::string alias = resolveAlias(variable.name, refPos);
          emitScalarStore(alias, literal, refPos);
        }
      }
      return;
    }

    try
    {
      std::vector<std::string> code;
      const std::size_t refPos = parsed.statementStart;
      const std::string targetAlias = resolveAlias(parsed.targetName, refPos);

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
            std::string decimalValue = convertLiteralToDecimal(expr.index->value);
            direct.push_back("LDI " + decimalValue);
          }
          else if (expr.index->kind == Expr::Kind::Variable)
          {
            const std::string name = resolveAlias(expr.index->value, refPos);
            direct.push_back("LD " + name);
          }
          if (!direct.empty())
          {
            direct.push_back("STO $indr");
            const std::string arrayName = resolveAlias(expr.value, refPos);
            direct.push_back("LDV " + arrayName);
            direct.push_back("STO " + targetAlias);
            emitAndStore(std::move(direct));
            return;
          }
        }

        if (expr.kind == Expr::Kind::Literal)
        {
          std::string decimalValue = convertLiteralToDecimal(expr.value);
          code.push_back("LDI " + decimalValue);
          code.push_back("STO " + targetAlias);
          emitAndStore(std::move(code));
          return;
        }
        if (expr.kind == Expr::Kind::Variable)
        {
          const std::string name = resolveAlias(expr.value, refPos);
          code.push_back("LD " + name);
          code.push_back("STO " + targetAlias);
          emitAndStore(std::move(code));
          return;
        }
        
        // Otimização para operações binárias simples (sem temporários desnecessários)
        if (expr.kind == Expr::Kind::Binary && expr.left && expr.right)
        {
          // Caso especial: operação binária simples (variável op literal/variável)
          bool leftIsSimple = (expr.left->kind == Expr::Kind::Variable || expr.left->kind == Expr::Kind::Literal);
          bool rightIsSimple = (expr.right->kind == Expr::Kind::Variable || expr.right->kind == Expr::Kind::Literal);
          
          if (leftIsSimple && rightIsSimple)
          {
            // Carrega operando esquerdo
            if (expr.left->kind == Expr::Kind::Literal)
            {
              std::string decimalValue = convertLiteralToDecimal(expr.left->value);
              code.push_back("LDI " + decimalValue);
            }
            else
            {
              const std::string name = resolveAlias(expr.left->value, refPos);
              code.push_back("LD " + name);
            }
            
            // Aplica operação com operando direito
            if (expr.op == "<<" || expr.op == ">>")
            {
              // Shifts
              if (expr.right->kind == Expr::Kind::Literal)
              {
                std::string decimalValue = convertLiteralToDecimal(expr.right->value);
                code.push_back(opcodeForOperator(expr.op) + " " + decimalValue);
              }
              else
              {
                const std::string name = resolveAlias(expr.right->value, refPos);
                code.push_back(opcodeForOperator(expr.op) + " " + name);
              }
            }
            else if (expr.op == "&" || expr.op == "|" || expr.op == "^")
            {
              // Operações bit a bit
              if (expr.right->kind == Expr::Kind::Literal)
              {
                std::string decimalValue = convertLiteralToDecimal(expr.right->value);
                code.push_back(opcodeForOperator(expr.op) + "I " + decimalValue);
              }
              else
              {
                const std::string name = resolveAlias(expr.right->value, refPos);
                code.push_back(opcodeForOperator(expr.op) + " " + name);
              }
            }
            else
            {
              // Operações aritméticas
              if (expr.right->kind == Expr::Kind::Literal)
              {
                std::string decimalValue = convertLiteralToDecimal(expr.right->value);
                code.push_back(opcodeForOperator(expr.op) + "I " + decimalValue);
              }
              else
              {
                const std::string name = resolveAlias(expr.right->value, refPos);
                code.push_back(opcodeForOperator(expr.op) + " " + name);
              }
            }
            
            code.push_back("STO " + targetAlias);
            emitAndStore(std::move(code));
            return;
          }
        }
        
        if (emitOptimizedAddSub(expr, targetAlias, code, refPos))
        {
          emitAndStore(std::move(code));
          return;
        }
      }

      ExpressionEmitter emitter(code, refPos);
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
          code.push_back("STOV " + targetAlias);
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
        code.push_back("STOV " + targetAlias);
      }
      else
      {
        std::string valueTemp = emitter.emit(*parsed.rhsExpr);
        code.push_back("LD " + valueTemp);
        code.push_back("STO " + targetAlias);
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
      generateReadIntoExpression(*expr, code, position);
      statementInstructions.emplace_back(position, std::move(code));
    }
    catch (const std::exception &)
    {
      // ignore leituras inválidas
    }
  }

  void registerPrintStatement(std::size_t position, const std::string &expression)
  {
    const std::string key = std::to_string(position) + ":" + expression;
    if (seenPrints.count(key))
      return;
    seenPrints.insert(key);
    try
    {
      std::vector<std::string> code;
      auto expr = parseExpressionString(expression);
      generatePrintExpression(*expr, code, position);
      auto sameBlock = [&](const std::vector<std::string> &other) {
        if (other.size() != code.size())
          return false;
        for (std::size_t i = 0; i < code.size(); ++i)
        {
          if (code[i] != other[i])
            return false;
        }
        return true;
      };
      for (const auto &entry : statementInstructions)
      {
        if (entry.first == position && sameBlock(entry.second))
        {
          return;
        }
      }
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

  void scanPrintStatements()
  {
    const std::string &src = Semantico::sourceCode;
    const std::string keyword = "print";
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
      registerPrintStatement(i, argument);
      i = close;
    }
  }

  std::string render()
  {
    generateControlFlow();
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
