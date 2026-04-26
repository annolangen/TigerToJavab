#include <cctype>
#include <iostream>
#include <string>
#include <vector>

#include "debug_string.h"
#include "driver.h"
#include "java_source.h"
#include "symbol_table.h"
#include "type_finder.h"

int main(int argc, char** argv) {
  std::vector<std::string> args(argv + 1, argv + argc);
  bool print_ast = false;
  bool print_java = false;
  std::string filename;

  for (const auto& arg : args) {
    if (arg == "--print-ast") {
      print_ast = true;
    } else if (arg == "--print-java") {
      print_java = true;
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
  if (print_java) {
    std::unique_ptr<SymbolTable> symbols = SymbolTable::Build(*driver.result);
    std::vector<std::string> errors;
    TypeFinder types(*symbols, errors);

    // Extract class name from filename
    std::string class_name = filename;
    size_t last_slash = class_name.find_last_of("/\\");
    if (last_slash != std::string::npos) {
      class_name = class_name.substr(last_slash + 1);
    }
    size_t last_dot = class_name.find_last_of('.');
    if (last_dot != std::string::npos) {
      class_name = class_name.substr(0, last_dot);
    }
    if (!class_name.empty()) {
      class_name[0] = std::toupper(class_name[0]);
    } else {
      class_name = "Main";
    }

    std::cout << java::Compile(*driver.result, *symbols, types, class_name) << std::endl;
  }

  return 0;
}
