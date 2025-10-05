#include "CompatibilityTable.hpp"

bool isNumeric(Type type) {
  return type == Type::Int || type == Type::Float;
}

Type resultType(Type left, Type right, Operator op) {
  if (left == Type::Error || right == Type::Error) {
    return Type::Error;
  }

  switch (left) {
    case Type::Int:
      if (right == Type::Float) {
        return Type::Float;
      }
      if (right == Type::Int) {
        return op == Operator::Divide ? Type::Float : Type::Int;
      }
      return Type::Error;
    case Type::Float:
      return isNumeric(right) ? Type::Float : Type::Error;
    case Type::String:
      return op == Operator::Add && right == Type::String ? Type::String : Type::Error;
    case Type::Error:
      break;
  }

  return Type::Error;
}

AttributeResult attributeResult(Type left, Type right) {
  if (left == Type::Error || right == Type::Error) {
    return AttributeResult::Error;
  }

  if (left == right) {
    return AttributeResult::Ok;
  }

  if (isNumeric(left) && isNumeric(right)) {
    return AttributeResult::Warning;
  }

  return AttributeResult::Error;
}
