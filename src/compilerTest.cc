#include "compiler.h"
#include "testing/catch.h"
#include "testing/testing.h"

namespace {
using testing::Parse;
using testing::RunJava;

const char* tiger = R"(print("Hello World"))";

SCENARIO("compiles to class file", "[compile]") {
  GIVEN("Hello World") {
    auto exp = Parse(tiger);
    if (exp) {
      Compile(*exp);
      REQUIRE(RunJava() == "Hello World");
    } else {
      FAIL("Parse Failed");
    }
  }
}
} // namespace
