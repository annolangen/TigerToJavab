#pragma once
#include <string>

enum BinaryOp {
  kNone
#define DEF_BINARY_OPERATOR(c, n) , c
#include "binary_operator.defs"
#undef DEF_BINARY_OPERATOR
};

void AppendDebugString(BinaryOp op, std::string& out);
