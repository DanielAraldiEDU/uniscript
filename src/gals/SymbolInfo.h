#pragma once

#include <string>

struct SymbolInfo {
    std::string identifier;
    std::string type;
    bool isConstant = false;
    bool initialized = false;
    bool used = false;
    int scope = -1;
    int line = -1;
    int column = -1;
    bool isParameter = false;
    bool isArray = false;
    bool isFunction = false;
};
