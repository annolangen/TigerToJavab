#include "java_source.h"

#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_string.hpp"
#include "testing/testing.h"

namespace {

using Catch::Matchers::ContainsSubstring;
using Catch::Matchers::Equals;

std::string Compile(std::string_view text,
                    std::string_view class_name = "Main") {
  std::unique_ptr<syntax::Expr> expr = testing::Parse(text);
  REQUIRE(expr != nullptr);
  std::unique_ptr<SymbolTable> st = SymbolTable::Build(*expr);
  std::vector<std::string> errors;
  TypeFinder tf(*st, errors);
  REQUIRE(errors.empty());
  return java::Compile(*expr, *st, tf, class_name);
}

SCENARIO("JavaSource") {
  GIVEN("Leaf Expression") {
    REQUIRE_THAT(Compile("nil"), ContainsSubstring("null"));
  }
  GIVEN("array type and an array variable") {
    REQUIRE_THAT(Compile(R"(
let
	type  arrtype = array of int
	var arr1:arrtype := arrtype [10] of 0
in
	arr1
end)"),
                 Equals(R"(import java.utils.Arrays;

class Main {

  public static void main(String[] args) {
int[] arr1 = new int[10];
Arrays.fill(arr1, 0);
arr1;
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
                 Equals(R"(import java.utils.Arrays;

class Main {

  public static void main(String[] args) {
int N = 8;
int[] row = new int[N];
Arrays.fill(row, 0);
int[] col = new int[N];
Arrays.fill(col, 0);
int[] diag1 = new int[N + N - 1];
Arrays.fill(diag1, 0);
int[] diag2 = new int[N + N - 1];
Arrays.fill(diag2, 0);
  static void printboard() {

for (int i = 0; i < N - 1; i++) {

for (int j = 0; j < N - 1; j++) {
print.run(if (col[i] = j) {
" O"
} else {
" ."
})
};
print.run("\n")
};
print.run("\n")
  }
  static void try(int c) {
if (c = N) {
printboard.run()
} else {
for (int r = 0; r < N - 1; r++) {
if (row[r] = 0 & diag1[r + c] = 0 & diag2[r + 7 - c] = 0) {

row[r] = 1;
diag1[r + c] = 1;
diag2[r + 7 - c] = 1;
col[c] = r;
_try.run(c + 1);
row[r] = 0;
diag1[r + c] = 0;
diag2[r + 7 - c] = 0
}
}
}
  }
_try.run(0);
}
}
)"));
  }
}
}  // namespace
