#pragma once
#include <ostream>
#include <string_view>

enum BinaryOp {
  kNone
#define DEF_BINARY_OPERATOR(c, n) , c
#include "binary_operator.defs"
#undef DEF_BINARY_OPERATOR
};

inline constexpr std::string_view kBinaryOpNames[] = {"<NONE>"
#define DEF_BINARY_OPERATOR(c, n) , n
#include "binary_operator.defs"
#undef DEF_BINARY_OPERATOR
};

inline std::ostream& operator<<(std::ostream& os, BinaryOp op) {
  return os << kBinaryOpNames[static_cast<size_t>(op)];
}
