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
    t_KEY_TYPE_VOID = 15,
    t_KEY_IF = 16,
    t_KEY_ELSE = 17,
    t_KEY_ELIF = 18,
    t_KEY_DO = 19,
    t_KEY_WHILE = 20,
    t_KEY_FOR = 21,
    t_KEY_RETURN = 22,
    t_KEY_FUNCTION = 23,
    t_KEY_CONST = 24,
    t_KEY_VAR = 25,
    t_KEY_SWITCH = 26,
    t_KEY_CASE = 27,
    t_KEY_BREAK = 28,
    t_KEY_DEFAULT = 29,
    t_KEY_THROW = 30,
    t_KEY_CONTINUE = 31,
    t_KEY_TRUE = 32,
    t_KEY_FALSE = 33,
    t_KEY_NULL = 34,
    t_KEY_COMMENT_MULT_LINE = 35,
    t_KEY_COMMENT_LINE = 36,
    t_KEY_SUM = 37,
    t_KEY_SUB = 38,
    t_KEY_MULT = 39,
    t_KEY_DIV = 40,
    t_KEY_MOD = 41,
    t_KEY_EXPO = 42,
    t_KEY_INCREMENT = 43,
    t_KEY_DECREMENT = 44,
    t_KEY_ATTR = 45,
    t_KEY_OR = 46,
    t_KEY_AND = 47,
    t_KEY_NOT = 48,
    t_KEY_GREATER = 49,
    t_KEY_LESSER = 50,
    t_KEY_GREATER_EQUAL = 51,
    t_KEY_LESSER_EQUAL = 52,
    t_KEY_EQUAL = 53,
    t_KEY_NOT_EQUAL = 54,
    t_KEY_BIT_SR = 55,
    t_KEY_BIT_SL = 56,
    t_KEY_BIT_AND = 57,
    t_KEY_BIT_OR = 58,
    t_KEY_NEGACAO = 59,
    t_KEY_BIT_XOR = 60,
    t_KEY_BIT_NOT = 61,
    t_KEY_DOT = 62,
    t_KEY_COMMA = 63,
    t_KEY_SEMICOLON = 64,
    t_KEY_COLON = 65,
    t_KEY_LPAREN = 66,
    t_KEY_RPAREN = 67,
    t_KEY_LBRACKET = 68,
    t_KEY_RBRACKET = 69,
    t_KEY_LBRACE = 70,
    t_KEY_RBRACE = 71
};

const int STATES_COUNT = 64;

extern int SCANNER_TABLE[STATES_COUNT][256];

extern int TOKEN_STATE[STATES_COUNT];

extern int SPECIAL_CASES_INDEXES[73];

extern const char *SPECIAL_CASES_KEYS[25];

extern int SPECIAL_CASES_VALUES[25];

extern const char *SCANNER_ERROR[STATES_COUNT];

const int FIRST_SEMANTIC_ACTION = 150;

const int SHIFT  = 0;
const int REDUCE = 1;
const int ACTION = 2;
const int ACCEPT = 3;
const int GO_TO  = 4;
const int ERROR  = 5;

extern const int PARSER_TABLE[303][165][2];

extern const int PRODUCTIONS[175][2];

extern const char *PARSER_ERROR[303];

#endif
