#ifndef SEMANTIC_TABLE_H
#define SEMANTIC_TABLE_H

#include <string>
#include <vector>

using namespace std;

class SemanticTable {
public:
    enum Types { INT = 0, FLO, STR, BOO };
    enum Operations { SUM = 0, SUB, MUL, DIV, REL, MOD, POT, AND, OR };
    enum Status { ERR = -1, WAR, OK };

    struct SymbolEntry {
        string name;
        Types type;
        bool initialized;
        bool used;
        int scope;
        bool isParameter;
        int position;
        bool isArray;
        bool isFunction;
        bool isConstant;
    };

    static int const expTable[4][4][9];
    static int const atribTable[4][4];

    static vector<SymbolEntry> symbolTable;

    static const vector<SymbolEntry> &getSymbolTable() {
        return symbolTable;
    }

    // Adiciona um símbolo à tabela
    void addSymbol(const SymbolEntry &entry) {
        symbolTable.push_back(entry);
    }

    // Busca um símbolo na tabela, se não encontrar retorna false
    bool searchSymbol(const string &name) {
        for (const auto &entry : symbolTable) {
            if (entry.name == name) {
                return true;
            }
        }
        return false;
    }

    void setSymbolUsed(const string &name) {
        for (auto &entry : symbolTable) {
            if (entry.name == name) {
                entry.used = true;
                return;
            }
        }
    }

    void setSymbolInitialized(const string &name) {
        for (auto &entry : symbolTable) {
            if (entry.name == name) {
                entry.initialized = true;
                return;
            }
        }
    }

    static int resultType(int TP1, int TP2, int OP) {
        if (TP1 < 0 || TP2 < 0 || OP < 0) {
            return ERR;
        }
        return expTable[TP1][TP2][OP];
    }

    static int atribType(int TP1, int TP2) {
        if (TP1 < 0 || TP2 < 0) {
            return ERR;
        }
        return atribTable[TP1][TP2];
    }
};

// expTable[Tipo1][Tipo2][Operação]
// Resultado do tipo da expressão (ou ERR)
int const SemanticTable::expTable[4][4][9] =
              /*            OPERACOES               */
              /* SUM,SUB,MUL,DIV,REL,MOD,POT,AND,OR */
{
/*INT   */  {
              {INT,INT,INT,FLO,BOO,INT,INT,INT,INT},   // INT op INT
              {FLO,FLO,FLO,FLO,BOO,ERR,FLO,INT,INT},   // INT op FLOAT
              {ERR,ERR,ERR,ERR,BOO,ERR,ERR,INT,INT},   // INT op STRING
              {ERR,ERR,ERR,ERR,BOO,ERR,ERR,INT,INT}    // INT op BOOL
            },
/*FLOAT */  {
              {FLO,FLO,FLO,FLO,BOO,ERR,FLO,FLO,FLO},   // FLOAT op INT
              {FLO,FLO,FLO,FLO,BOO,ERR,FLO,FLO,FLO},   // FLOAT op FLOAT
              {ERR,ERR,ERR,ERR,BOO,ERR,ERR,FLO,FLO},   // FLOAT op STRING
              {ERR,ERR,ERR,ERR,BOO,ERR,ERR,FLO,FLO}    // FLOAT op BOOL
            },
/*STRING*/  {
              {ERR,ERR,ERR,ERR,BOO,ERR,ERR,STR,STR},   // STRING op INT
              {ERR,ERR,ERR,ERR,BOO,ERR,ERR,STR,STR},   // STRING op FLOAT
              {STR,ERR,ERR,ERR,BOO,ERR,ERR,STR,STR},   // STRING op STRING (+ concatenação, REL comparação)
              {ERR,ERR,ERR,ERR,BOO,ERR,ERR,STR,STR}    // STRING op BOOL
            },
/*BOOL  */  {
              {ERR,ERR,ERR,ERR,BOO,ERR,ERR,BOO,BOO},   // BOOL op INT
              {ERR,ERR,ERR,ERR,BOO,ERR,ERR,BOO,BOO},   // BOOL op FLOAT
              {ERR,ERR,ERR,ERR,BOO,ERR,ERR,BOO,BOO},   // BOOL op STRING
              {ERR,ERR,ERR,ERR,BOO,ERR,ERR,BOO,BOO}    // BOOL op BOOL (REL, AND, OR)
            }
};

// atribTable[ESQ][DIR]
// Retorna OK, WAR ou ERR conforme compatibilidade
int const SemanticTable::atribTable[4][4] = {
    /*        INT                FLO                STR                BOO  */
    /*INT*/  { SemanticTable::OK,  SemanticTable::WAR,  SemanticTable::ERR,  SemanticTable::ERR },
    /*FLO*/  { SemanticTable::WAR,  SemanticTable::OK,   SemanticTable::ERR,  SemanticTable::ERR },
    /*STR*/  { SemanticTable::ERR, SemanticTable::ERR,  SemanticTable::OK,   SemanticTable::ERR },
    /*BOO*/  { SemanticTable::ERR, SemanticTable::ERR,  SemanticTable::ERR,  SemanticTable::OK  }
};

#endif // SEMANTIC_TABLE_H
