#include "emit.h"
#include "testing/catch.h"
#include <fstream>
#include <sstream>
#include <streambuf>
#include <string>
namespace {
using emit::Program;
std::string ReadFile(const char* path) {
  std::ifstream t(path);
  return {std::istreambuf_iterator<char>(t), std::istreambuf_iterator<char>()};
}
SCENARIO("emits class file", "[emit]") {
  GIVEN("Hello World") {
    Program program;
    auto text = program.DefineStringConstant("Hello, World!\n");
    if (auto f = program.LookupLibraryFunction("print"); f) {
      f->Call(*program.GetMainCodeBlock(), {text.get()});
    }
    std::ostringstream os;
    program.Emit(os);
    REQUIRE(os.str() == ReadFile("../../src/testdata/Main.class"));
  }
}
} // namespace
