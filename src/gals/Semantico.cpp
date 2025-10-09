#include "Semantico.h"
#include "Constants.h"

#include <iostream>
#include <string>

using namespace std;

void Semantico::executeAction(int action, const Token *token)
{
  string currentValue;

  switch (action) {
    case 1:
      currentValue = token->getLexeme();
      break;
    case 2:
      // OR
      break;
    case 3:
      // AND
      break;
    case 4:
      // BIT OR
      break;
    case 5:
      // EXPO
      break;
    case 6:
      // BIT AND
      break;
    case 7:
      // OP REL
      break;
    case 8:
      // OP BITWISE
      break;
    case 9:
      // ARIT LOWER
      break;
    case 10:
      // ARIT UPPER
      break;
    case 11:
      // NEG
      break;
    case 12:
      // LEFT PARENTHESIS
      break;
    case 13:
      // RIGHT PARENTHESIS
      break;
    case 14:
      // FUNCTION CALL
      break;
    case 15:
      // INDEXED VALUE
    case 16:
      // COMMENTS
      break;
    case 17:
      // PRINT
      cout << currentValue;
      break;
    case 18:
      // READ
      break;
    case 19:
      // TYPE
      break;
    case 20:
      // OPEN BRACKET
      break;
    case 21:
      // CLOSE BRACKET
      break;
    case 22:
      // ATTRIBUTION
      break;
    case 23:
      // FUNCTION DECLARATION
      break;
    case 24:
      // INCREMENT/DECREMENT
      break;
    case 25:
      // CONST/VAR
      break;
    case 26:
      // MULTI VAR DECLARATION
      break;
    case 27:
      // ATTRIBUTION INCREMENT/DECREMENT  
      break;
    case 28:
      // OPEN BRACKET INDEX
      break;
    case 29:
      // CLOSE BRACKET INDEX
      break;
    case 30:
      // FUNCTION TYPE
      break;
    case 31:
      // RETURN
      break;
    case 32:
      // BREAK
      break;
    case 33:
      // THROW
      break;
    case 34:
      // IF/ELIF/ELSE
      break;
    case 35:
      // DO
      break;
    case 36:
      // WHILE
      break;
    case 37:
      // FOR
      break;
    case 38:
      // FOR VARIABLE
      break;
    case 39:
      // FOR VARIABLE TYPE
      break;
    case 40:
      // FOR VALUE
      break;
    case 41:
      // SWITCH/CASE/DEFAULT
      break;
    case 99:
      // FINAL CODE
      break;
    default:
      cout << "Unknown semantic action" << endl;
      break;
  }
}
