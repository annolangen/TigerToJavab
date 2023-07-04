#include "driver.h"

#include "parser.hh"

Driver::~Driver() = default;

int Driver::parse(const std::string& f) {
  file = f;
  scan_begin();
  yy::Parser parser(*this);
  parser.set_debug_level(options_.trace_parsing);
  int res = parser.parse();
  scan_end();
  return res;
}

void Driver::error(const yy::location& l, const std::string& m) {
  std::cerr << l << ": " << m << std::endl;
}

void Driver::error(const std::string& m) { std::cerr << m << std::endl; }
