#include "BinaryOp.h"

std::ostream& operator<<(std::ostream& os, BinaryOp op) {
  switch (op) {
  case kNone:
    return os << "<NONE>";
#define DEF_BINARY_OPERATOR(c, n)                                              \
  case c:                                                                      \
    return os << (n);
#include "binary_operator.defs"
#undef DEF_BINARY_OPERATOR
  }
  return os;
}
