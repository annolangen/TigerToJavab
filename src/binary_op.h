#pragma once
#include <ostream>
#include <string>

enum BinaryOp {
  kNone
#define DEF_BINARY_OPERATOR(c, n) , c
#include "binary_operator.defs"
#undef DEF_BINARY_OPERATOR
};

std::ostream& operator<<(std::ostream& os, BinaryOp op);
