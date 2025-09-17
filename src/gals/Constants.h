#ifndef CONSTANTS_H
#define CONSTANTS_H

enum TokenId 
{
    EPSILON  = 0,
    DOLLAR   = 1,
    t_KEY_PRINT = 2,
    t_KEY_READ = 3,
    t_KEY_VARIABLE = 4,
    t_KEY_DECIMAL = 5,
    t_KEY_INTEGER = 6,
    t_KEY_BINARY = 7,
    t_KEY_HEXADECIMAL = 8,
    t_KEY_STRING = 9,
    t_KEY_TYPE_INT = 10,
    t_KEY_TYPE_FLOAT = 11,
    t_KEY_TYPE_STRING = 12,
    t_KEY_TYPE_BOOLEAN = 13,
    t_KEY_TYPE_FUNC = 14,
    t_KEY_TYPE_OBJECT = 15,
    t_KEY_TYPE_VOID = 16,
    t_KEY_IF = 17,
    t_KEY_ELSE = 18,
    t_KEY_ELIF = 19,
    t_KEY_DO = 20,
    t_KEY_WHILE = 21,
    t_KEY_FOR = 22,
    t_KEY_RETURN = 23,
    t_KEY_FUNCTION = 24,
    t_KEY_CONST = 25,
    t_KEY_VAR = 26,
    t_KEY_SWITCH = 27,
    t_KEY_CASE = 28,
    t_KEY_BREAK = 29,
    t_KEY_DEFAULT = 30,
    t_KEY_TRY = 31,
    t_KEY_CATCH = 32,
    t_KEY_FINALLY = 33,
    t_KEY_THROW = 34,
    t_KEY_CONTINUE = 35,
    t_KEY_TRUE = 36,
    t_KEY_FALSE = 37,
    t_KEY_NULL = 38,
    t_KEY_COMMENT_MULT_LINE = 39,
    t_KEY_COMMENT_LINE = 40,
    t_KEY_SUM = 41,
    t_KEY_SUB = 42,
    t_KEY_MULT = 43,
    t_KEY_DIV = 44,
    t_KEY_MOD = 45,
    t_KEY_EXPO = 46,
    t_KEY_INCREMENT = 47,
    t_KEY_DECREMENT = 48,
    t_KEY_ATTR = 49,
    t_KEY_OR = 50,
    t_KEY_AND = 51,
    t_KEY_NOT = 52,
    t_KEY_GREATER = 53,
    t_KEY_LESSER = 54,
    t_KEY_GREATER_EQUAL = 55,
    t_KEY_LESSER_EQUAL = 56,
    t_KEY_EQUAL = 57,
    t_KEY_NOT_EQUAL = 58,
    t_KEY_BIT_SR = 59,
    t_KEY_BIT_SL = 60,
    t_KEY_BIT_AND = 61,
    t_KEY_BIT_OR = 62,
    t_KEY_NEGACAO = 63,
    t_KEY_BIT_XOR = 64,
    t_KEY_BIT_NOT = 65,
    t_KEY_DOT = 66,
    t_KEY_COMMA = 67,
    t_KEY_SEMICOLON = 68,
    t_KEY_COLON = 69,
    t_KEY_LPAREN = 70,
    t_KEY_RPAREN = 71,
    t_KEY_LBRACKET = 72,
    t_KEY_RBRACKET = 73,
    t_KEY_LBRACE = 74,
    t_KEY_RBRACE = 75
};

const int STATES_COUNT = 64;

extern int SCANNER_TABLE[STATES_COUNT][256];

extern int TOKEN_STATE[STATES_COUNT];

extern int SPECIAL_CASES_INDEXES[77];

extern const char *SPECIAL_CASES_KEYS[29];

extern int SPECIAL_CASES_VALUES[29];

extern const char *SCANNER_ERROR[STATES_COUNT];

const int FIRST_SEMANTIC_ACTION = 150;

const int SHIFT  = 0;
const int REDUCE = 1;
const int ACTION = 2;
const int ACCEPT = 3;
const int GO_TO  = 4;
const int ERROR  = 5;

extern const int PARSER_TABLE[266][150][2];

extern const int PRODUCTIONS[164][2];

extern const char *PARSER_ERROR[266];

#endif
