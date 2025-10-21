#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <vector>

#include "src/gals/Lexico.h"
#include "src/gals/Sintatico.h"
#include "src/gals/Semantico.h"
#include "src/gals/LexicalError.h"
#include "src/gals/SyntacticError.h"
#include "src/gals/SemanticError.h"

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

static std::string jsonEscape(const std::string& s) {
  return jsonEscape(s.c_str());
}

static std::string boolToJson(bool value) {
  return value ? "true" : "false";
}

static std::string symbolTableToJson(const std::vector<ExportedSymbol>& symbols) {
  std::string json = "[";
  bool first = true;
  for (const auto& sym : symbols) {
    if (!first) json += ",";
    first = false;

    json += "{";
    json += "\"name\":\"" + jsonEscape(sym.name) + "\"";
    json += ",\"type\":\"" + jsonEscape(sym.type) + "\"";
    json += ",\"initialized\":" + boolToJson(sym.initialized);
    json += ",\"used\":" + boolToJson(sym.used);
    json += ",\"scope\":" + std::to_string(sym.scope);
    json += ",\"isParameter\":" + boolToJson(sym.isParameter);
    json += ",\"position\":" + std::to_string(sym.position);
    json += ",\"line\":" + std::to_string(sym.line);
    json += ",\"column\":" + std::to_string(sym.column);
    json += ",\"isArray\":" + boolToJson(sym.isArray);
    json += ",\"isFunction\":" + boolToJson(sym.isFunction);
    json += ",\"isConstant\":" + boolToJson(sym.isConstant);
    json += "}";
  }
  json += "]";
  return json;
}

static std::string diagnosticsToJson(const std::vector<ExportedDiagnostic>& diagnostics) {
  std::string json = "[";
  bool first = true;
  for (const auto& d : diagnostics) {
    if (!first) json += ",";
    first = false;
    json += "{\"severity\":\"" + jsonEscape(d.severity) + "\",\"message\":\"" + jsonEscape(d.message) + "\",\"position\":" + std::to_string(d.position) + ",\"length\":" + std::to_string(d.length) + "}";
  }
  json += "]";
  return json;
}

static std::string successResponse(const std::vector<ExportedSymbol>& symbols,
                                   const std::vector<ExportedDiagnostic>& diagnostics) {
  std::string json = "{\"ok\":true";
  json += ",\"symbolTable\":" + symbolTableToJson(symbols);
  json += ",\"diagnostics\":" + diagnosticsToJson(diagnostics);
  json += "}";
  return json;
}

static std::string errorResponse(const char* kind, const char* message, int pos, int length) {
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
  json += ",\"length\":" + std::to_string(length <= 0 ? 1 : length);
  json += ",\"symbolTable\":[]";
  json += ",\"diagnostics\":[";
  if (message) {
    json += "{\"severity\":\"error\",\"message\":\"" + jsonEscape(message) + "\",\"position\":" + std::to_string(pos) + ",\"length\":" + std::to_string(length <= 0 ? 1 : length) + "}";
  }
  json += "]";
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

  sem.resetState();
  sem.setSourceCode(src);
  lex.setInput(src);

  try {
    sint.parse(&lex, &sem);
    finalizeSemanticAnalysis();
    auto symbols = snapshotSymbolTable();
    auto diagnostics = snapshotDiagnostics();
    std::string json = successResponse(symbols, diagnostics);
    sem.resetState();
    return duplicateString(json);
  } catch (const LexicalError& e) {
    sem.resetState();
    return duplicateString(errorResponse("lexical", e.getMessage(), e.getPosition(), e.getLength()));
  } catch (const SyntacticError& e) {
    sem.resetState();
    return duplicateString(errorResponse("syntactic", e.getMessage(), e.getPosition(), e.getLength()));
  } catch (const SemanticError& e) {
    sem.resetState();
    return duplicateString(errorResponse("semantic", e.getMessage(), e.getPosition(), e.getLength()));
  } catch (...) {
    sem.resetState();
    return duplicateString(errorResponse("unknown", "unknown error", -1, 1));
  }
}

}
