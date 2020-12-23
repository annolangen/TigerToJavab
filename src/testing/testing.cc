#include "testing.h"
#include "../driver.h"
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
} // namespace testing
