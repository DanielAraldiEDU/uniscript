#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <string>

#include "src/gals/Lexico.h"
#include "src/gals/Sintatico.h"
#include "src/gals/Semantico.h"
#include "src/gals/LexicalError.h"
#include "src/gals/SyntacticError.h"
#include "src/gals/SemanticError.h"

using SymbolTable = Semantico::Table;

static std::string jsonEscape(const char* s) {
  std::string out;
  if (!s) return out;
  for (const unsigned char* p = (const unsigned char*)s; *p; ++p) {
    unsigned char c = *p;
    switch (c) {
      case '"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\b': out += "\\b"; break;
      case '\f': out += "\\f"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if (c < 0x20) {
          char buf[7];
          std::snprintf(buf, sizeof(buf), "\\u%04x", c);
          out += buf;
        } else {
          out += static_cast<char>(c);
        }
    }
  }
  return out;
}

static std::string symbolTableToJson(const SymbolTable& table) {
  std::string json = "[";
  bool first = true;
  for (const auto& symbol : table) {
    if (!first) {
      json += ",";
    }
    first = false;
    json += "{";
    json += "\"identifier\":\"" + jsonEscape(symbol.identifier.c_str()) + "\"";
    json += ",\"type\":\"" + jsonEscape(symbol.type.c_str()) + "\"";
    json += ",\"modality\":\"" + jsonEscape(symbol.modality.c_str()) + "\"";
    json += ",\"scope\":\"" + jsonEscape(symbol.scope.c_str()) + "\"";
    json += ",\"declaredLine\":" + std::to_string(symbol.declaredLine);
    json += ",\"initialized\":" + std::string(symbol.initialized ? "true" : "false");
    json += ",\"used\":" + std::string(symbol.used ? "true" : "false");
    json += "}";
  }
  json += "]";
  return json;
}

static std::string successResponse(const Semantico& sem) {
  std::string json = "{\"ok\":true,\"symbolTable\":";
  json += symbolTableToJson(sem.symbolTable());
  json += "}";
  return json;
}

static std::string errorResponse(const char* kind, const char* message, int pos, const Semantico& sem) {
  std::string json = "{\"ok\":false";
  if (kind) {
    json += ",\"kind\":\"";
    json += jsonEscape(kind);
    json += "\"";
  }
  if (message) {
    json += ",\"message\":\"";
    json += jsonEscape(message);
    json += "\"";
  }
  json += ",\"pos\":" + std::to_string(pos);
  json += ",\"symbolTable\":";
  json += symbolTableToJson(sem.symbolTable());
  json += "}";
  return json;
}

static char* duplicateString(const std::string& s) {
  char* out = static_cast<char*>(std::malloc(s.size() + 1));
  if (!out) {
    return nullptr;
  }
  std::memcpy(out, s.c_str(), s.size() + 1);
  return out;
}

extern "C" {
__attribute__((used))
char* uniscript_compile(const char* src) {
  Lexico lex;
  Sintatico sint;
  Semantico sem;

  lex.setInput(src);

  try {
    sint.parse(&lex, &sem);
    return duplicateString(successResponse(sem));
  } catch (const LexicalError& e) {
    return duplicateString(errorResponse("lexical", e.getMessage(), e.getPosition(), sem));
  } catch (const SyntacticError& e) {
    return duplicateString(errorResponse("syntactic", e.getMessage(), e.getPosition(), sem));
  } catch (const SemanticError& e) {
    return duplicateString(errorResponse("semantic", e.getMessage(), e.getPosition(), sem));
  } catch (...) {
    return duplicateString(errorResponse("unknown", "unknown error", -1, sem));
  }
}

}
