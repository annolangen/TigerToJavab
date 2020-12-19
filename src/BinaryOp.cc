#include "BinaryOp.h"

void AppendDebugString(BinaryOp op, std::string &out) {
  switch (op) {
  case kNone:
    out += "<NONE>";
    return;
#define DEF_BINARY_OPERATOR(c, n)                                              \
  case c:                                                                      \
    out += n;                                                                  \
    return;
#include "binary_operator.defs"
#undef DEF_BINARY_OPERATOR
  }
}
