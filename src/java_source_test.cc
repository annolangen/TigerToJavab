#include "java_source.h"

#include <algorithm>

#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_string.hpp"
#include "testing/testing.h"

namespace {

using Catch::Matchers::ContainsSubstring;
using Catch::Matchers::Equals;

std::string Compile(std::string_view text, std::string_view class_name = "Main") {
  std::unique_ptr<syntax::Expr> expr = testing::Parse(text);
  REQUIRE(expr != nullptr);
  std::unique_ptr<SymbolTable> st = SymbolTable::Build(*expr);
  std::vector<std::string> errors;
  TypeFinder tf(*st, errors);
  REQUIRE(errors.empty());
  std::string result = java::Compile(*expr, *st, tf, class_name);
  result.erase(std::remove(result.begin(), result.end(), '\r'), result.end());
  return result;
}

SCENARIO("JavaSource") {
  GIVEN("Leaf Expression") { REQUIRE_THAT(Compile("nil"), ContainsSubstring("null")); }
  GIVEN("array type and an array variable") {
    REQUIRE_THAT(Compile(R"(
let
  type  arrtype = array of int
  var arr1:arrtype := arrtype [10] of 0
in
  arr1
end)"),
                 Equals(R"(import java.util.Arrays;

class Scope1 {
  public Scope0 parent;
  public int[] arr1;
}

class Main {

  public static void main(String[] args) {
    Scope0 _scope0 = new Scope0();
    {
      Scope1 _scope1 = new Scope1();
      _scope1.parent = _scope0;
      _scope1.arr1 = new int[10];
      Arrays.fill(_scope1.arr1, 0);
      _scope1.arr1;
    }
  }
}
)"));
  }
  GIVEN("8 Queens") {
    REQUIRE_THAT(Compile(R"(
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
)"),
                 Equals(R"(import java.util.Arrays;

class Scope1 {
  public Scope0 parent;
  public int N;
  public int[] row;
  public int[] col;
  public int[] diag1;
  public int[] diag2;
}

class Scope2 {
  public Scope1 parent;
}

class Scope3 {
  public Scope1 parent;
  public int c;
}

class Main {

  public static void main(String[] args) {
    Scope0 _scope0 = new Scope0();
    {
      Scope1 _scope1 = new Scope1();
      _scope1.parent = _scope0;
      _scope1.N = 8;
      _scope1.row = new int[_scope1.N];
      Arrays.fill(_scope1.row, 0);
      _scope1.col = new int[_scope1.N];
      Arrays.fill(_scope1.col, 0);
      _scope1.diag1 = new int[_scope1.N + _scope1.N - 1];
      Arrays.fill(_scope1.diag1, 0);
      _scope1.diag2 = new int[_scope1.N + _scope1.N - 1];
      Arrays.fill(_scope1.diag2, 0);
      _try(_scope1, 0);
    }
  }

      static void printboard(Scope1 _scope1) {
        Scope2 _scope2 = new Scope2();
        _scope2.parent = _scope1;
        for (int i = 0; i <= _scope1.N - 1; i++) {
          for (int j = 0; j <= _scope1.N - 1; j++) {
            System.out.print((_scope1.col[i] == j ? " O" : " ."));
          }
          System.out.print("\n");
        }
        System.out.print("\n");
      }

      static void _try(Scope1 _scope1, int c) {
        Scope3 _scope3 = new Scope3();
        _scope3.parent = _scope1;
        _scope3.c = c;
        if (_scope3.c == _scope1.N) {
          printboard(_scope1);
        } else {
          for (int r = 0; r <= _scope1.N - 1; r++) {
            if (_scope1.row[r] == 0 && _scope1.diag1[r + _scope3.c] == 0 && _scope1.diag2[r + 7 - _scope3.c] == 0) {
              _scope1.row[r] = 1;
              _scope1.diag1[r + _scope3.c] = 1;
              _scope1.diag2[r + 7 - _scope3.c] = 1;
              _scope1.col[_scope3.c] = r;
              _try(_scope1, _scope3.c + 1);
              _scope1.row[r] = 0;
              _scope1.diag1[r + _scope3.c] = 0;
              _scope1.diag2[r + 7 - _scope3.c] = 0;
            }
          }
        }
      }
}
)"));
  }
}
}  // namespace
