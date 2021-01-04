#include "emit.h"
#include "instruction.h"
#include "testing/catch.h"
#include "testing/testing.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <streambuf>
#include <string>

namespace {
using emit::Program;
using emit::Pushable;
using testing::RunJava;

std::string ReadFile(const char* path) {
  std::ifstream t(path);
  return {std::istreambuf_iterator<char>(t), std::istreambuf_iterator<char>()};
}

// Finishes the given instructions for the method body of "main" with a return
// statement, defines the "main" method using these instructions, and outputs
// the resulting program as class file /tmp/Main.class.
void EmitAsMain(std::ostringstream& main_instructions, Program& program) {
  main_instructions.put(Instruction::_return);
  program.DefineFunction(emit::ACC_PUBLIC | emit::ACC_STATIC, "main",
                         "([Ljava/lang/String;)V", main_instructions.str());
  std::ofstream out("/tmp/Main.class");
  program.Emit(out);
}

SCENARIO("emits class file", "[emit]") {
  GIVEN("Hello World") {
    const char* msg = "Hello, World!\n";
    auto program = Program::JavaProgram();
    if (auto f = program->LookupLibraryFunction("print"); f) {
      const Pushable* text = program->DefineStringConstant(msg);
      std::ostringstream main_instructions;
      f->Call(main_instructions, {text});
      EmitAsMain(main_instructions, *program);
    } else {
      FAIL("Library function print not found");
    }
    REQUIRE(RunJava() == msg);
  }

  GIVEN("printint") {
    auto program = Program::JavaProgram();
    if (auto f = program->LookupLibraryFunction("printi"); f) {
      const Pushable* int_constant = program->DefineIntegerConstant(20202020);
      std::ostringstream main_instructions;
      f->Call(main_instructions, {int_constant});
      EmitAsMain(main_instructions, *program);
    } else {
      FAIL("Library function printi not found");
    }
    REQUIRE(RunJava() == "20202020");
  }
}
} // namespace
