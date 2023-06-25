#pragma once

#include <map>
#include <string>
#include <unordered_set>

#include "parser.hh"
#include "syntax.h"

// Tell Flex the lexer's prototype ...
#define YY_DECL yy::Parser::symbol_type yylex(Driver& driver)
// ... and declare it for the parser's sake.
YY_DECL;

// Conducting the whole scanning and parsing of Tiger compiler.
class Driver {
 public:
  Driver();
  virtual ~Driver();

  static std::unordered_set<std::string> kTypeIds;
  std::map<std::string, int> variables;

  std::unique_ptr<syntax::Expr> result;
  // Handling the scanner.
  void scan_begin();
  void scan_end();
  bool trace_scanning;
  // Run the parser on file F.
  // Return 0 on success.
  int parse(const std::string& f);
  // The name of the file being parsed.
  // Used later to pass the file name to the location tracker.
  std::string file;
  // Whether parser traces should be generated.
  bool trace_parsing;
  // Error handling.
  void error(const yy::location& l, const std::string& m);
  void error(const std::string& m);
};
