#include "symbol_table.h"

#include <iostream>

#include "catch2/catch_test_macros.hpp"
#include "testing/testing.h"

namespace {
SCENARIO("SymbolTable", "[SymbolTable]") {
  GIVEN("Empty SymbolTable") {
    syntax::Expr nil = syntax::Nil{};
    auto t = SymbolTable::Build(nil);
    WHEN("empty") {
      REQUIRE(t->lookupFunction(nil, "foo") == nullptr);
      REQUIRE(t->lookupVariable(nil, "foo") == nullptr);
      REQUIRE(t->lookupType(nil, "foo") == nullptr);
    }
  }
  GIVEN("array type and an array variable") {
    auto expr = testing::Parse(R"(
let
	type arrtype = array of int
	var arr1:arrtype := arrtype [10] of 0
in
	arr1
end)");
    REQUIRE(expr != nullptr);
    auto t = SymbolTable::Build(*expr);
    const std::vector<std::unique_ptr<syntax::Expr>>& body =
        std::get<syntax::Let>(*expr).body;
    REQUIRE(body.size() == 1);
    const syntax::Expr& arr1 = *body[0];
    REQUIRE(t->lookupVariable(arr1, "arr1") != nullptr);
  }
}
}  // namespace
