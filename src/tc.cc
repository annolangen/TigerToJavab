#include "Expression.h"
#include "ToString.h"
#include <iostream>

int main(int argc, char **argv) {
  std::cout << "I compile Tiger" << std::endl;

  std::cout << "Test type "
            << ToString(RecordType({{"x", "string"}, {"y", "int"}}))
            << std::endl;

  return 0;
}
