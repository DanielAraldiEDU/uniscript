#ifndef SEMANTIC_TABLE_H
#define SEMANTIC_TABLE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <iomanip>
#include <sstream>

#include "gals/SemanticError.h"

using namespace std;

class SemanticTable {
public:
    enum Types { INT = 0, FLOAT, STRING, BOOLEAN, VOID };
    enum Operations { SUM = 0, SUB, MUL, DIV, REL, MOD, POT, AND, OR };
    enum Status { ERR = -1, WAR, OK };

    struct SymbolEntry {
        string name;
        Types type = INT;
        bool hasExplicitType = false;
        bool initialized = false;
        bool used = false;
        int scope = -1;
        bool isParameter = false;
        int position = -1;
        int line = -1;
        int column = -1;
        bool isArray = false;
        bool isFunction = false;
        bool isConstant = false;
    };

    struct DiagnosticEntry {
        string severity;
        string message;
        int position = -1;
        int length = 1;
    };

    struct Param {
        string name;
        Types type;
        int position;
        bool isArray = false;
        int line = -1;
        int column = -1;
    };

    SemanticTable() { enterScope(); }

    void reset() {
        scopes.clear();
        symbolTable.clear();
        diagnostics.clear();
        openFunctions.clear();
        functionScopeDepths.clear();
        pendingExpressionType = -1;
        enterScope();
    }

    void declare(const SymbolEntry &e) {
        auto &cur = scopes.back();

        if (cur.count(e.name)) {
            addError("Identificador já declarado neste escopo: '" + e.name + "'", e.position, static_cast<int>(e.name.size()));
            return;
        }

        SymbolEntry copy = e;
        copy.scope = (int)scopes.size()-1;
        int idx = (int)symbolTable.size();
        symbolTable.push_back(copy);
        cur[e.name] = idx;
    }

    void commitStatement(const SymbolEntry &instrucao) {
        if (instrucao.name.empty() || instrucao.isFunction) {
            limparTipoPendente();
            return;
        }
        const int indiceSimbolo = lookupIndex(instrucao.name);
        const int escopoAtual = scopes.empty() ? -1 : static_cast<int>(scopes.size()) - 1;
        const bool tentativaDeclaracao = instrucao.hasExplicitType || instrucao.isParameter;

        if (tentativaDeclaracao) {
            const bool mesmaDeclaracao = indiceSimbolo >= 0 && symbolTable[indiceSimbolo].scope == escopoAtual;
            if (mesmaDeclaracao) {
                addError("Identificador já declarado neste escopo: '" + instrucao.name + "'");
            } else {
                tratarNovaDeclaracao(instrucao);
            }
        } else if (indiceSimbolo < 0) {
            tratarNovaDeclaracao(instrucao);
        } else {
            tratarUsoExistente(instrucao, indiceSimbolo);
        }

        limparTipoPendente();
    }

    void beginFunction(const string& name, Types retType, const vector<Param>& params, int position = -1, int line = -1, int column = -1) {
        SymbolEntry fun;
        fun.name = name;
        fun.type = retType;
        fun.initialized = true;
        fun.isFunction = true;
        fun.hasExplicitType = true;
        fun.isConstant = true;
        fun.position = position;
        fun.line = line;
        fun.column = column;
        declare(fun);
        enterScope();
        functionScopeDepths.push_back((int)scopes.size() - 1);
        for (auto p : params) {
            SymbolEntry s;
            s.name = p.name;
            s.type = p.type;
            s.initialized = true;
            s.isParameter = true;
            s.position = p.position;
            s.line = p.line;
            s.column = p.column;
            s.hasExplicitType = true;
            s.isArray = p.isArray;
            declare(s);
        }
        openFunctions.push_back(name);
    }

    // Fecha escopo de função quando apropriado
    void maybeCloseFunction() {
        if (!openFunctions.empty()) {
            exitScope();
            openFunctions.pop_back();
        }
    }

    void enterScope() {
        scopes.emplace_back();
    }

    void exitScope() {
        if (scopes.empty()) return;

        auto &cur = scopes.back();

        for (auto &kv : cur) {
            auto &sym = symbolTable[kv.second];
            if (!sym.used) {
                addWarning("Identificador declarado e não usado: '" + sym.name + "' (escopo " + to_string(sym.scope) + ")", sym.position, static_cast<int>(sym.name.size()));
            }
        }

        if (!functionScopeDepths.empty() && functionScopeDepths.back() == (int)scopes.size() - 1) {
            functionScopeDepths.pop_back();
        }

        scopes.pop_back();
    }

    void closeAllScopes() {
        while (scopes.size() > 1) exitScope();
        if (scopes.size() == 1) exitScope();
    }

    void markUseIfDeclared(const string &name, int position = -1, int length = 1, bool requireArray = false) {
        int idx = lookupIndex(name);
        if (idx < 0) {
            addError("Uso de identificador não declarado: '" + name + "'", position, length);
            return;
        }
        if (!functionScopeDepths.empty()) {
            int functionScopeDepth = functionScopeDepths.back();
            const auto &sym = symbolTable[idx];
            if (!sym.isFunction && sym.scope < functionScopeDepth) {
                addError("Identificador não declarado neste escopo: '" + name + "'", position, length);
                return;
            }
        }
        if (requireArray && !symbolTable[idx].isArray) {
            addError("Identificador não é um vetor: '" + name + "'", position, length);
            return;
        }
        symbolTable[idx].used = true;
        if (!symbolTable[idx].initialized && !symbolTable[idx].isFunction) {
            addWarning("Possível uso sem inicialização: '" + name + "'", position, length);
        }
    }

    void noteExprType(Types t) { pendingExpressionType = (int)t; }
    void discardPendingExpression() { limparTipoPendente(); }
    Types getSymbolType(const string &name) const {
        int idx = lookupIndex(name);
        if (idx < 0) {
            return INT;
        }
        return symbolTable[idx].type;
    }
    bool isArraySymbol(const string &name) const {
        int idx = lookupIndex(name);
        if (idx < 0) {
            return false;
        }
        return symbolTable[idx].isArray;
    }
    bool hasSymbol(const string &name) const {
        return lookupIndex(name) >= 0;
    }

    const vector<SymbolEntry> &getSymbolTable() const {
        return symbolTable;
    }

    const vector<DiagnosticEntry> &getDiagnostics() const {
        return diagnostics;
    }

    static string typeToStr(Types t);
    void printTable(std::ostream& os) const {
        os << "\n==== TABELA DE SÍMBOLOS ====\n";
        os << left << setw(18) << "Nome"
           << setw(8)  << "Tipo"
           << setw(12) << "Mutab."
           << setw(12) << "Inicializada"
           << setw(6)  << "Usada"
           << setw(6)  << "Escopo"
           << setw(10) << "Posição"
           << setw(10) << "Parâmetro"
           << setw(6)  << "Vetor"
           << setw(6)  << "Função"
           << "\n";
        os << string(102, '-') << "\n";
        for (auto &sym : symbolTable) {
            std::ostringstream pos;
            if (sym.line >= 0) {
                pos << sym.line << ':' << (sym.column > 0 ? sym.column : 1);
            } else {
                pos << '-';
            }
            os << left << setw(18) << sym.name
               << setw(8)  << typeToStr(sym.type)
               << setw(12) << (sym.isConstant ? "const" : "var")
               << setw(12) << (sym.initialized ? "sim" : "nao")
               << setw(6)  << (sym.used ? "sim" : "nao")
               << setw(6)  << sym.scope
               << setw(10) << pos.str()
               << setw(10) << (sym.isParameter ? "sim" : "nao")
               << setw(6)  << (sym.isArray ? "sim" : "nao")
               << setw(6)  << (sym.isFunction ? "sim" : "nao")
               << "\n";
        }
        os << string(102, '-') << "\n";
    }

    void printDiagnostics(std::ostream& os) const {
        os << "\n==== DIAGNÓSTICOS ====\n";
        for (auto &d : diagnostics) {
            const char* tag = d.severity == "error" ? "[ERRO] " : "[AVISO] ";
            os << tag << d.message << "\n";
        }
        os << string(64, '-') << "\n";
    }

    static int const expTable[4][4][9];
    static int const atribTable[4][4];

    static int resultType(int TP1, int TP2, int OP) {
        if (TP1 < 0 || TP1 >= 4 || TP2 < 0 || TP2 >= 4 || OP < 0) {
            return ERR;
        }
        return expTable[TP1][TP2][OP];
    }

    static int atribType(int TP1, int TP2) {
        if (TP1 < 0 || TP1 >= 4 || TP2 < 0 || TP2 >= 4) {
            return ERR;
        }
        return atribTable[TP1][TP2];
    }

private:
    vector<unordered_map<string,int>> scopes;
    vector<SymbolEntry> symbolTable;
    vector<DiagnosticEntry> diagnostics;
    vector<string> openFunctions;
    vector<int> functionScopeDepths;
    int pendingExpressionType = -1;

    void tratarNovaDeclaracao(const SymbolEntry &instrucao) {
        if (!instrucao.hasExplicitType) {
            addError("Uso de identificador não declarado: '" + instrucao.name + "'", instrucao.position, static_cast<int>(instrucao.name.size()));
            return;
        }

        declare(instrucao);

        int novoIndice = lookupIndex(instrucao.name);
        if (novoIndice < 0) return;

        if (pendingExpressionType >= 0) {
            int resultado = atribType((int)symbolTable[novoIndice].type, pendingExpressionType);
            if (resultado == ERR) {
                addError("Tipos incompatíveis na inicialização de '" + instrucao.name + "'", instrucao.position, static_cast<int>(instrucao.name.size()));
            } else {
                symbolTable[novoIndice].initialized = true;
                if (resultado == WAR) {
                    addWarning("Conversão implícita na inicialização de '" + instrucao.name + "'", instrucao.position, static_cast<int>(instrucao.name.size()));
                }
            }
        } else if (instrucao.initialized) {
            symbolTable[novoIndice].initialized = true;
        }
    }

    void tratarUsoExistente(const SymbolEntry &instrucao, int indiceSimbolo) {
        auto &simbolo = symbolTable[indiceSimbolo];
        simbolo.used = true;

        const bool tentativaAtribuicao = instrucao.initialized || pendingExpressionType >= 0;
        if (tentativaAtribuicao && simbolo.isConstant) {
            addError("Não é permitido modificar constante: '" + instrucao.name + "'", instrucao.position, static_cast<int>(instrucao.name.size()));
            return;
        }

        if (pendingExpressionType >= 0) {
            int resultado = atribType((int)simbolo.type, pendingExpressionType);
            if (resultado == ERR) {
                addError("Tipos incompatíveis na atribuição para '" + instrucao.name + "'", instrucao.position, static_cast<int>(instrucao.name.size()));
            } else {
                simbolo.initialized = true;
                if (resultado == WAR) {
                    addWarning("Possível perda de precisão na atribuição para '" + instrucao.name + "'", instrucao.position, static_cast<int>(instrucao.name.size()));
                }
            }
        } else if (instrucao.initialized) {
            simbolo.initialized = true;
        } else if (!simbolo.initialized && !simbolo.isFunction) {
            addWarning("Possível uso sem inicialização: '" + instrucao.name + "'", instrucao.position, static_cast<int>(instrucao.name.size()));
        }
    }

    void limparTipoPendente() {
        pendingExpressionType = -1;
    }

    int lookupIndex(const string &name) const {
        for (int i=(int)scopes.size()-1; i>=0; --i) {
            auto it = scopes[i].find(name);
            if (it != scopes[i].end()) return it->second;
        }
        return -1;
    }

    void addError(const string& message, int position = -1, int length = 1){
        diagnostics.push_back({"error", message, position, length});
        throw SemanticError(message, position, length);
    }
    void addWarning(const string& message, int position = -1, int length = 1){
        diagnostics.push_back({"warning", message, position, length});
    }
};

string SemanticTable::typeToStr(Types t) {
    switch (t) {
        case INT: return "int";
        case FLOAT: return "float";
        case STRING: return "string";
        case BOOLEAN: return "bool";
        case VOID: return "void";
    }
    return "?";
}

// expTable[Tipo1][Tipo2][Operação]
// Resultado do tipo da expressão (ou ERR)
int const SemanticTable::expTable[4][4][9] =
              /*            OPERACOES               */
              /* SUM,SUB,MUL,DIV,REL,MOD,POT,AND,OR */
{
/*INT   */  {
              {INT,INT,INT,FLOAT,BOOLEAN,INT,INT,INT,INT},   // INT op INT
              {FLOAT,FLOAT,FLOAT,FLOAT,BOOLEAN,ERR,FLOAT,INT,INT},   // INT op FLOAT
              {ERR,ERR,ERR,ERR,BOOLEAN,ERR,ERR,INT,INT},   // INT op STRING
              {ERR,ERR,ERR,ERR,BOOLEAN,ERR,ERR,INT,INT}    // INT op BOOLEAN
            },
/*FLOAT */  {
              {FLOAT,FLOAT,FLOAT,FLOAT,BOOLEAN,ERR,FLOAT,FLOAT,FLOAT},   // FLOAT op INT
              {FLOAT,FLOAT,FLOAT,FLOAT,BOOLEAN,ERR,FLOAT,FLOAT,FLOAT},   // FLOAT op FLOAT
              {ERR,ERR,ERR,ERR,BOOLEAN,ERR,ERR,FLOAT,FLOAT},   // FLOAT op STRING
              {ERR,ERR,ERR,ERR,BOOLEAN,ERR,ERR,FLOAT,FLOAT}    // FLOAT op BOOLEAN
            },
/*STRING*/  {
              {ERR,ERR,ERR,ERR,BOOLEAN,ERR,ERR,STRING,STRING},   // STRING op INT
              {ERR,ERR,ERR,ERR,BOOLEAN,ERR,ERR,STRING,STRING},   // STRING op FLOAT
              {STRING,ERR,ERR,ERR,BOOLEAN,ERR,ERR,STRING,STRING},   // STRING op STRING (+ concatenação)
              {ERR,ERR,ERR,ERR,BOOLEAN,ERR,ERR,STRING,STRING}    // STRING op BOOLEAN
            },
/*BOOLEAN*/ {
              {ERR,ERR,ERR,ERR,BOOLEAN,ERR,ERR,BOOLEAN,BOOLEAN},   // BOOLEAN op INT
              {ERR,ERR,ERR,ERR,BOOLEAN,ERR,ERR,BOOLEAN,BOOLEAN},   // BOOLEAN op FLOAT
              {ERR,ERR,ERR,ERR,BOOLEAN,ERR,ERR,BOOLEAN,BOOLEAN},   // BOOLEAN op STRING
              {ERR,ERR,ERR,ERR,BOOLEAN,ERR,ERR,BOOLEAN,BOOLEAN}    // BOOLEAN op BOOLEAN
            }
};

// atribTable[ESQ][DIR]
// Retorna OK, WAR ou ERR conforme compatibilidade
int const SemanticTable::atribTable[4][4] = {
    /*        INT                  FLOAT                STRING               BOOLEAN  */
    /*INT*/    { SemanticTable::OK,  SemanticTable::WAR,  SemanticTable::ERR,  SemanticTable::ERR },
    /*FLOAT*/  { SemanticTable::WAR, SemanticTable::OK,   SemanticTable::ERR,  SemanticTable::ERR },
    /*STRING*/ { SemanticTable::ERR, SemanticTable::ERR,  SemanticTable::OK,   SemanticTable::ERR },
    /*BOOLEAN*/{ SemanticTable::ERR, SemanticTable::ERR,  SemanticTable::ERR,  SemanticTable::OK  }
};

#endif // SEMANTIC_TABLE_H
