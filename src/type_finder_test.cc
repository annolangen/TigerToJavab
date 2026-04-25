#include "type_finder.h"

#include "catch2/catch_test_macros.hpp"
#include "testing/testing.h"

namespace {
SCENARIO("TypeFinder") {
  GIVEN("Empty TypeFinder") {
    syntax::Expr nil = syntax::Nil{};
    std::vector<std::string> errors;
    auto symbols = SymbolTable::Build(nil);
    TypeFinder tf(*symbols, errors);
    REQUIRE(tf(nil) == "NOTYPE");
    REQUIRE(errors.empty());
  }
  GIVEN("array type and an array variable") {
    auto expr = testing::Parse(R"(
let
  type Foo = array of int
  var arr1:Foo := Foo [10] of 0
in
  arr1[7]
end)");
    REQUIRE(expr != nullptr);
    std::vector<std::string> errors;
    auto symbols = SymbolTable::Build(*expr);
    TypeFinder tf(*symbols, errors);
    REQUIRE(tf(*expr) == "int");
    REQUIRE(errors.empty());
  }
  GIVEN("Alias chain") {
    auto expr = testing::Parse(R"(
let
  type Foo = array of int
  type Bar = Foo
  var a:Bar := Bar [10] of 0
in a[7] end)");
    REQUIRE(expr != nullptr);
    std::vector<std::string> errors;
    auto symbols = SymbolTable::Build(*expr);
    TypeFinder tf(*symbols, errors);
    REQUIRE(tf(*expr) == "int");
    REQUIRE(errors.empty());
  }
  GIVEN("undeclared variable") {
    auto expr = testing::Parse(R"(let var x := 1 in y end)");
    REQUIRE(expr != nullptr);
    std::vector<std::string> errors;
    auto symbols = SymbolTable::Build(*expr);
    TypeFinder tf(*symbols, errors);
    REQUIRE(tf(*expr) == "NOTYPE");
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0] == "Variable not found: y");
  }
  GIVEN("undeclared array type") {
    auto expr = testing::Parse(R"(
let
  var arr1:Foo := Foo [10] of 0
in
  arr1[7]
end)");
    REQUIRE(expr != nullptr);
    std::vector<std::string> errors;
    auto symbols = SymbolTable::Build(*expr);
    TypeFinder tf(*symbols, errors);
    REQUIRE(tf(*expr) == "NOTYPE");
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0] == "Type not found: Foo");
  }
  GIVEN("undeclared record type type") {
    auto expr = testing::Parse(R"(
let
  var rec:Foo := nil
in
  rec.name
end)");
    REQUIRE(expr != nullptr);
    std::vector<std::string> errors;
    auto symbols = SymbolTable::Build(*expr);
    TypeFinder tf(*symbols, errors);
    REQUIRE(tf(*expr) == "NOTYPE");
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0] == "Type not found: Foo");
  }
}
}  // namespace
