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
    t_KEY_TYPE_VOID = 14,
    t_KEY_IF = 15,
    t_KEY_ELSE = 16,
    t_KEY_ELIF = 17,
    t_KEY_DO = 18,
    t_KEY_WHILE = 19,
    t_KEY_FOR = 20,
    t_KEY_RETURN = 21,
    t_KEY_FUNCTION = 22,
    t_KEY_CONST = 23,
    t_KEY_VAR = 24,
    t_KEY_SWITCH = 25,
    t_KEY_CASE = 26,
    t_KEY_BREAK = 27,
    t_KEY_DEFAULT = 28,
    t_KEY_THROW = 29,
    t_KEY_TRUE = 30,
    t_KEY_FALSE = 31,
    t_KEY_COMMENT_MULT_LINE = 32,
    t_KEY_COMMENT_LINE = 33,
    t_KEY_SUM = 34,
    t_KEY_SUB = 35,
    t_KEY_MULT = 36,
    t_KEY_DIV = 37,
    t_KEY_MOD = 38,
    t_KEY_EXPO = 39,
    t_KEY_INCREMENT = 40,
    t_KEY_DECREMENT = 41,
    t_KEY_ATTR = 42,
    t_KEY_OR = 43,
    t_KEY_AND = 44,
    t_KEY_NOT = 45,
    t_KEY_GREATER = 46,
    t_KEY_LESSER = 47,
    t_KEY_GREATER_EQUAL = 48,
    t_KEY_LESSER_EQUAL = 49,
    t_KEY_EQUAL = 50,
    t_KEY_NOT_EQUAL = 51,
    t_KEY_BIT_SR = 52,
    t_KEY_BIT_SL = 53,
    t_KEY_BIT_AND = 54,
    t_KEY_BIT_OR = 55,
    t_KEY_NEGACAO = 56,
    t_KEY_BIT_XOR = 57,
    t_KEY_BIT_NOT = 58,
    t_KEY_DOT = 59,
    t_KEY_COMMA = 60,
    t_KEY_SEMICOLON = 61,
    t_KEY_COLON = 62,
    t_KEY_LPAREN = 63,
    t_KEY_RPAREN = 64,
    t_KEY_LBRACKET = 65,
    t_KEY_RBRACKET = 66,
    t_KEY_LBRACE = 67,
    t_KEY_RBRACE = 68
};

const int STATES_COUNT = 64;

extern int SCANNER_TABLE[STATES_COUNT][256];

extern int TOKEN_STATE[STATES_COUNT];

extern int SPECIAL_CASES_INDEXES[70];

extern const char *SPECIAL_CASES_KEYS[22];

extern int SPECIAL_CASES_VALUES[22];

extern const char *SCANNER_ERROR[STATES_COUNT];

const int FIRST_SEMANTIC_ACTION = 147;

const int SHIFT  = 0;
const int REDUCE = 1;
const int ACTION = 2;
const int ACCEPT = 3;
const int GO_TO  = 4;
const int ERROR  = 5;

extern const int PARSER_TABLE[359][190][2];

extern const int PRODUCTIONS[174][2];

extern const char *PARSER_ERROR[359];

#endif
