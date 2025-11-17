#include <iostream>
#include <string>
#include <vector>

#include "debug_string.h"
#include "driver.h"

int main(int argc, char** argv) {
  std::vector<std::string> args(argv + 1, argv + argc);
  bool print_ast = false;
  std::string filename;

  for (const auto& arg : args) {
    if (arg == "--print-ast") {
      print_ast = true;
    } else if (arg.starts_with("--")) {
      std::cerr << "Error: Unknown flag '" << arg << "'." << std::endl;
      return 1;
    } else {
      if (!filename.empty()) {
        std::cerr << "Error: Multiple input files specified." << std::endl;
        return 1;
      }
      filename = arg;
    }
  }

  if (filename.empty()) {
    std::cerr << "Error: No input file specified." << std::endl;
    return 1;
  }

  Driver driver;
  if (driver.parse(filename) != 0) {
    std::cerr << "Parsing failed." << std::endl;
    return 1;
  }

  if (print_ast) {
    std::cout << DebugString(*driver.result) << std::endl;
  }

  return 0;
}
