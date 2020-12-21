#pragma once
#include "Expression.h"
#include <string>
namespace types {
std::string InferType(const Expression &e);
}
