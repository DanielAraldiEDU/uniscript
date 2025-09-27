#pragma once

#include <string>

struct SymbolInfo {
    std::string identifier;
    std::string type;
    std::string modality;
    std::string scope;
    int declaredLine = -1;
    bool initialized = false;
    bool used = false;
};
