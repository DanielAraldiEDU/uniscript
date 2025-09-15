#include <cstring>
#include <cstdlib>
#include <cstdio>

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

extern "C" {
__attribute__((used))
char* uniscript_compile(const char* src) {
  try {
    Lexico lex;
    Sintatico sint;
    Semantico sem;

    lex.setInput(src);
    sint.parse(&lex, &sem);

    const char* ok = "{\"ok\":true}";
    char* out = (char*)std::malloc(std::strlen(ok) + 1);
    std::strcpy(out, ok);
    return out;
  } catch (const LexicalError& e) {
    const std::string msg = jsonEscape(e.getMessage());
    const std::string json = std::string("{\"ok\":false,\"kind\":\"lexical\",\"message\":\"") + msg + "\",\"pos\":" + std::to_string(e.getPosition()) + "}";
    char* out = (char*)std::malloc(json.size() + 1);
    std::strcpy(out, json.c_str());
    return out;
  } catch (const SyntacticError& e) {
    const std::string msg = jsonEscape(e.getMessage());
    const std::string json = std::string("{\"ok\":false,\"kind\":\"syntactic\",\"message\":\"") + msg + "\",\"pos\":" + std::to_string(e.getPosition()) + "}";
    char* out = (char*)std::malloc(json.size() + 1);
    std::strcpy(out, json.c_str());
    return out;
  } catch (const SemanticError& e) {
    const std::string msg = jsonEscape(e.getMessage());
    const std::string json = std::string("{\"ok\":false,\"kind\":\"semantic\",\"message\":\"") + msg + "\",\"pos\":" + std::to_string(e.getPosition()) + "}";
    char* out = (char*)std::malloc(json.size() + 1);
    std::strcpy(out, json.c_str());
    return out;
  } catch (...) {
    const char* unk = "{\"ok\":false,\"kind\":\"unknown\",\"message\":\"unknown error\"}";
    char* out = (char*)std::malloc(std::strlen(unk) + 1);
    std::strcpy(out, unk);
    return out;
  }
}

}
