#include "debug_string.h"

#include "catch2/catch_test_macros.hpp"
#include "testing/testing.h"

namespace {
SCENARIO("DebugString") {
  GIVEN("Leaf Expression") {
    syntax::Expr nil = syntax::Nil{};
    REQUIRE(DebugString(nil) == "nil");
  }
  GIVEN("array type and an array variable") {
    auto expr = testing::Parse(R"(
let
	type  arrtype = array of int
	var arr1:arrtype := arrtype [10] of 0
in
	arr1
end)");
    REQUIRE(expr != nullptr);
  }
}
}  // namespace
