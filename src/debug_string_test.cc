#include "debug_string.h"

#include <iostream>

#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_string.hpp"
#include "testing/testing.h"

namespace {

using Catch::Matchers::Equals;

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
    REQUIRE_THAT(DebugString(*expr), Equals(R"(let
    type arrtype = array of int
    var arr1: arrtype := arrtype [10] of 0
in
    arr1
end)"));
  }
  GIVEN("8 Queens") {
    auto expr = testing::Parse(R"(
let
  var N := 8
  type intArray = array of int
  var row := intArray [ N ] of 0
  var col := intArray [ N ] of 0
  var diag1 := intArray [ N+N-1 ] of 0
  var diag2 := intArray [ N+N-1 ] of 0
  function printboard() =
    (for i := 0 to N-1 do 
      (for j := 0 to N-1 do print(if col[i]=j then " O" else " .");
       print("\n"));
     print("\n"))
  function try(c:int) =
    if c=N then printboard()
    else for r := 0 to N-1 do 
       if row[r]=0 & diag1[r+c]=0 & diag2[r+7-c]=0
       then (row[r] := 1; diag1[r+c] := 1;
             diag2[r+7-c] := 1; col[c] := r;
             try(c+1);
             row[r] := 0; diag1[r+c] := 0;
             diag2[r+7-c] := 0)
in try(0) end
)");
    REQUIRE(expr != nullptr);
    REQUIRE_THAT(DebugString(*expr), Equals(R"(let
    var N: NOTYPE := 8
    type intArray = array of int
    var row: NOTYPE := intArray [N] of 0
    var col: NOTYPE := intArray [N] of 0
    var diag1: NOTYPE := intArray [N + N - 1] of 0
    var diag2: NOTYPE := intArray [N + N - 1] of 0
    function printboard(): NOTYPE
  (for i := 0 to N - 1 do (for j := 0 to N - 1 do print(if col[i] = j then " O" else " ."); print("\n")); print("\n"))
    function try(c: int): NOTYPE
  if c = N then printboard() else for r := 0 to N - 1 do if row[r] = 0 & diag1[r + c] = 0 & diag2[r + 7 - c] = 0 then (row[r] := 1; diag1[r + c] := 1; diag2[r + 7 - c] := 1; col[c] := r; try(c + 1); row[r] := 0; diag1[r + c] := 0; diag2[r + 7 - c] := 0)
in
    try(0)
end)"));
  }
}
}  // namespace
