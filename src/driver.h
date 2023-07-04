#pragma once

#include <map>
#include <string>

#include "parser.hh"
#include "syntax.h"

// Tell Flex the lexer's prototype ...
#define YY_DECL yy::Parser::symbol_type yylex(Driver& driver)
// ... and declare it for the parser's sake.
YY_DECL;

struct DriverOptions {
  bool trace_scanning = false;
  bool trace_parsing = false;
};
// Conducting the whole scanning and parsing of Tiger compiler.
class Driver {
 public:
  Driver(DriverOptions options = {}) : options_(options) {}
  virtual ~Driver();
  
  std::unique_ptr<syntax::Expr> result;

  // Handling the scanner.
  void scan_begin();
  void scan_end();

  // Run the parser on file F.
  // Return 0 on success.
  int parse(const std::string& f);

  // The name of the file being parsed.
  // Used later to pass the file name to the location tracker.
  std::string file;

  // Error handling.
  void error(const yy::location& l, const std::string& m);
  void error(const std::string& m);

  private:
  DriverOptions options_;
};
