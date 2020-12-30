#include "testing.h"
#include "../driver.h"
#include <cstdio>
#include <fstream>
#include <iostream>

namespace testing {
std::shared_ptr<Expression> Parse(const std::string& text) {
  std::ofstream myfile;
  std::string file_name = "/tmp/testing.tig";
  myfile.open(file_name);
  myfile << text;
  myfile.close();
  return ParseFile(file_name);
}

std::shared_ptr<Expression> ParseFile(const std::string& file_name) {
  Driver driver;
  if (driver.parse(file_name) == 0) return driver.result;
  return std::make_shared<Nil>();
}

std::string RunJava() {
  std::string result;
  // Command that works on Cygwin and Linux by avoiding path separator in the
  // Java classpath.
  const char* cmd = "test -f /tmp/Std.class || cp -f ../../src/Std.class /tmp;"
                    "cd /tmp; java Main";
  std::array<char, 128> buffer;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
  if (pipe) {
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
      result += buffer.data();
    }
  }
  return result;
}
} // namespace testing
