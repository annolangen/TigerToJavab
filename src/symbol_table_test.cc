#include "symbol_table.h"

#include <iostream>

#include "catch2/catch_test_macros.hpp"
#include "testing/testing.h"

namespace {

using namespace syntax;

SCENARIO("SymbolTable", "[SymbolTable]") {
  GIVEN("Empty SymbolTable") {
    Expr nil = Nil{};
    auto t = SymbolTable::Build(nil);
    WHEN("empty") {
      REQUIRE(t->lookupFunction(nil, "foo") == nullptr);
      REQUIRE(std::holds_alternative<std::nullptr_t>(
          t->lookupStorageLocation(nil, "foo")));
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
    const std::vector<std::unique_ptr<Expr>>& body = std::get<Let>(*expr).body;
    REQUIRE(body.size() == 1);
    StorageLocation var = t->lookupStorageLocation(*body[0], "arr1");
    REQUIRE(std::holds_alternative<const VariableDeclaration*>(var));
  }

  GIVEN("given parameter") {
    auto expr = testing::Parse(R"(
let
	type arrtype = array of int
  function first(a:arrtype):int = a[0]
  var arr1:arrtype := arrtype [10] of 0
in
	first(arr1)
end)");
    REQUIRE(expr != nullptr);
    auto t = SymbolTable::Build(*expr);
    const Declaration& decl = *std::get<Let>(*expr).declaration[1];
    REQUIRE(std::holds_alternative<FunctionDeclaration>(decl));
    const FunctionDeclaration& fdecl = std::get<FunctionDeclaration>(decl);
    StorageLocation a = t->lookupStorageLocation(*fdecl.body, "a");
    REQUIRE(std::holds_alternative<const TypeField*>(a));
    const TypeField* a_ptr = std::get<const TypeField*>(a);
    REQUIRE(a_ptr != nullptr);
    REQUIRE(a_ptr->id == "a");
    REQUIRE(a_ptr->type_id == "arrtype");
    const std::vector<std::unique_ptr<Expr>>& body = std::get<Let>(*expr).body;
    REQUIRE(body.size() == 1);
    const FunctionDeclaration* f = t->lookupFunction(*body[0], "first");
    REQUIRE(f != nullptr);
    REQUIRE(f->id == "first");
    REQUIRE(f->parameter.size() == 1);
    REQUIRE(f->parameter[0].id == "a");
    REQUIRE(f->parameter[0].type_id == "arrtype");
    REQUIRE(f->type_id == "int");
  }

  GIVEN("mutually recursive functions") {
    auto expr = testing::Parse(R"(
let
  function f(a:int):int = if a > 0 then g(a-1) else 0
  function g(a:int):int = if a > 0 then f(a-1) else 1
in
  f(5)
end)");
    REQUIRE(expr != nullptr);
    auto t = SymbolTable::Build(*expr);

    // Check that `g` is visible from within `f`
    const auto& f_decl =
        std::get<FunctionDeclaration>(*std::get<Let>(*expr).declaration[0]);
    const FunctionDeclaration* g_lookup_from_f =
        t->lookupFunction(*f_decl.body, "g");
    REQUIRE(g_lookup_from_f != nullptr);
    REQUIRE(g_lookup_from_f->id == "g");

    // Check that `f` is visible from within `g`
    const auto& g_decl =
        std::get<FunctionDeclaration>(*std::get<Let>(*expr).declaration[1]);
    const FunctionDeclaration* f_lookup_from_g =
        t->lookupFunction(*g_decl.body, "f");
    REQUIRE(f_lookup_from_g != nullptr);
    REQUIRE(f_lookup_from_g->id == "f");
  }
}
}  // namespace
