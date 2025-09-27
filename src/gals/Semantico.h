#ifndef SEMANTICO_H
#define SEMANTICO_H

#include <vector>

#include "Token.h"
#include "SemanticError.h"
#include "SymbolInfo.h"

class Semantico
{
public:
    using Table = std::vector<SymbolInfo>;

    void executeAction(int action, const Token *token);

    void clearSymbolTable();
    Table& symbolTable();
    const Table& symbolTable() const;

private:
    Table symbols;
};

#endif
