#include "compiler.h"
#include "testing/catch.h"
#include "testing/testing.h"

namespace {

std::string CompileAndRun(const char* program) {
  if (auto exp = testing::Parse(program); !exp) {
    return "Parse Failed";
  } else {
    Expression::SetNameSpacesBelow(*exp);
    Expression::SetTypesBelow(*exp);
    Compile(*exp);
    return testing::RunJava();
  }
}

SCENARIO("compiles to class file", "[compile]") {
  GIVEN("Hello World") {
    REQUIRE(CompileAndRun("print(\"Hello World\")") == "Hello World");
  }
  GIVEN("printi") { REQUIRE(CompileAndRun("printi(666)") == "666"); }
}
} // namespace
