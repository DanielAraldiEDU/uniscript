#include "BipGenerator.h"

#include <cctype>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace
{
  struct Entry
  {
    std::string name;
    bool isArray = false;
    std::size_t elementCount = 1;
    bool hasInitializer = false;
    std::vector<std::string> literalValues;
  };

  std::vector<Entry> entries;
  std::string cachedCode;
  constexpr const char *OUTPUT_FILE = "output.bip";

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

  std::string buildCode()
  {
    std::ostringstream out;
    out << ".data\n";
    for (const auto &entry : entries)
    {
      out << "  " << entry.name << ": ";
      if (entry.isArray)
      {
        if (entry.hasInitializer && !entry.literalValues.empty())
        {
          for (std::size_t idx = 0; idx < entry.literalValues.size(); ++idx)
          {
            if (idx > 0)
            {
              out << ",";
            }
            out << entry.literalValues[idx];
          }
        }
        else
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
    out << ".text\n HLT\n";
    return out.str();
  }
}

namespace BipGenerator
{

  void reset()
  {
    entries.clear();
    cachedCode.clear();
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
  }

  std::string render()
  {
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
