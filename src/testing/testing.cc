#define _POSIX_C_SOURCE 2
#include "testing.h"

#include <cstdio>
#include <fstream>
#include <iostream>

#include "../driver.h"
#include "../syntax_insertion.h"

extern FILE* yyin;

namespace testing {
std::unique_ptr<syntax::Expr> Parse(const std::string& text) {
  std::ofstream myfile;
  std::string file_name = "/tmp/testing.tig";
  myfile.open(file_name);
  myfile << text;
  myfile.close();
  return ParseFile(file_name);
}

std::unique_ptr<syntax::Expr> ParseFile(const std::string& file_name) {
  yyin = fopen(file_name.c_str(), "r");
  if (!yyin) {
    std::perror("File opening failed");
    return nullptr;
  }
  Driver driver;
  driver.parse(file_name);
  // std::fclose(yyin);
  return std::move(driver.result);
}

std::string RunJava() {
  std::string result;
  // Command that works on Cygwin and Linux by avoiding path separator in the
  // Java classpath.
  const char* cmd =
      "test -f /tmp/Std.class || cp -f ../../src/Std.class /tmp;"
      "cd /tmp; cp Main.class $(date +%N).class; java Main";
  std::array<char, 128> buffer;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
  if (pipe) {
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
      result += buffer.data();
    }
  }
  return result;
}
}  // namespace testing
