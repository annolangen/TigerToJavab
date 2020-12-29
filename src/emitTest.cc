#include "emit.h"
#include "instruction.h"
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
    auto program = Program::JavaProgram();
    if (auto f = program->LookupLibraryFunction("print"); f) {
      auto text = program->DefineStringConstant("Hello, World!\n");
      std::ostringstream main_os;
      f->Call(main_os, {text});
      main_os.put(Instruction::_return);
      program->DefineFunction(emit::ACC_PUBLIC | emit::ACC_STATIC, "main",
                              "([Ljava/lang/String;)V", main_os.str());
    }
    std::ofstream out("/tmp/Main.class");
    program->Emit(out);
   // REQUIRE(os.str() == ReadFile("../../src/testdata/Main.class"));
  }
}
} // namespace
