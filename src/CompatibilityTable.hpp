#pragma once

#include "enums/AttributeResult.hpp"
#include "enums/Operator.hpp"
#include "enums/Type.hpp"

Type resultType(Type left, Type right, Operator op);
AttributeResult attributeResult(Type left, Type right);

