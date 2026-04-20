#include "checker.h"

#include <string_view>

#include "catch2/catch_test_macros.hpp"
#include "testing/testing.h"

namespace {

std::vector<std::string> Check(const char* text) {
  std::unique_ptr<syntax::Expr> e = testing::Parse(text);
  REQUIRE(e != nullptr);
  auto st = SymbolTable::Build(*e);
  std::vector<std::string> errors;
  TypeFinder tf(*st, errors);
  std::vector<std::string> checker_errors = ListErrors(*e, *st, tf);
  errors.insert(errors.end(), checker_errors.begin(), checker_errors.end());
  return errors;
}

bool StartsWith(std::string_view text, std::string_view prefix) {
  return text.substr(0, prefix.size()) == prefix;
}

SCENARIO("Static checking", "[checker]") {
  GIVEN("Valid Record literal") {
    std::vector<std::string> errors =
        Check("let type Bulk = {height:int, weight:int} in "
              "Bulk {height=6, weight=200} end");
    REQUIRE(errors.size() == 0);
  }
  GIVEN("Missing field") {
    std::vector<std::string> errors =
        Check("let type Bulk = {height:int, weight:int} in "
              "Bulk {height=6} end");
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0] == "Type Bulk has 2 fields and literal has 1");
  }
  GIVEN("Fields out of order") {
    std::vector<std::string> errors =
        Check("let type Bulk = {height:int, weight:int} in "
              "Bulk {weight=200, height=6} end");
    REQUIRE(errors.size() == 2);
    // Require that both errors complain about different names, but don't insist
    // on a particular order.
    REQUIRE(StartsWith(errors[0], "Different names"));
    REQUIRE(StartsWith(errors[1], "Different names"));
  }
  GIVEN("Wrong field type") {
    std::vector<std::string> errors =
        Check("let type Bulk = {height:int, weight:int} in "
              "Bulk {height=\"6 feet\", weight=200} end");
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0] ==
            "Different types string and int for field #1 of record Bulk");
  }
  GIVEN("Wrong type") {
    std::vector<std::string> errors =
        Check("let type Bulk = {height:int, weight:int} in "
              "Heft {height=6, weight=200} end");
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0] == "Unknown record type Heft");
  }
  GIVEN("Non record type") {
    std::vector<std::string> errors =
        Check("let type Bulk = int in Bulk {height=6, weight=200} end");
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0] == "Type Bulk is not a record");
  }
  GIVEN("Good comparison") {
    std::vector<std::string> errors = Check("\"Hell\" < \"Hello\"");
    REQUIRE(errors.size() == 0);
    errors = Check("7 < (8 + 9)");
    REQUIRE(errors.size() == 0);
  }
  GIVEN("Mismatched operands") {
    std::vector<std::string> errors = Check("666 < \"Hello\"");
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0] == "Types of < should match, but got int and string");
  }
  GIVEN("Non-int") {
    std::vector<std::string> and_errors = Check("\"foo\" & \"bar\"");
    REQUIRE(and_errors.size() == 2);
    REQUIRE(and_errors[0] == "Operand type for & must be int, but got string");
    REQUIRE(and_errors[1] == "Operand type for & must be int, but got string");

    std::vector<std::string> or_errors = Check("\"foo\" | \"bar\"");
    REQUIRE(or_errors.size() == 2);
    REQUIRE(or_errors[0] == "Operand type for | must be int, but got string");
    REQUIRE(or_errors[1] == "Operand type for | must be int, but got string");
  }
  GIVEN("int condition") {
    auto errors = Check("if 1 < 2 then printi(1)");
    REQUIRE(errors.size() == 0);
  }
  GIVEN("string condition") {
    for (auto e : {"if \"Hello\" then printi(6)", "if \"Hello\" then 7 else 8",
                   "while \"Hello\" do break"}) {
      auto errors = Check(e);
      REQUIRE(errors.size() == 1);
      REQUIRE(errors[0] == "Conditions must be int, but got string");
    }
  }
  GIVEN("nil comparison") {
    WHEN("comparing with a record type") {
      auto errors = Check(R"(
let
  type myrec = { a: int }
  var r: myrec := nil
in
  r = nil
end)");
      REQUIRE(errors.empty());
    }

    WHEN("comparing with an int") {
      auto errors = Check(R"(
let
  var i := 0
in
  i = nil
end)");
      REQUIRE(errors.size() == 1);
      REQUIRE(errors[0] == "Type int is not a record type");
    }

    WHEN("comparing with a string") {
      auto errors = Check(R"(
let
  var s := ""
in
  nil = s
end)");
      REQUIRE(errors.size() == 1);
      REQUIRE(errors[0] == "Type string is not a record type");
    }

    WHEN("comparing with a record type alias") {
      auto errors = Check(R"(
let
  type myrec = { a: int }
  type rec_alias = myrec
  var r: rec_alias := nil
in
  r <> nil
end)");
      REQUIRE(errors.empty());
    }

    WHEN("comparing with a non-record type alias") {
      auto errors = Check(R"(
let
  type myint = int
  var i: myint := 0
in
  i = nil
end)");
      REQUIRE(errors.size() == 1);
      REQUIRE(errors[0] == "Type myint is not a record type");
    }
  }

  GIVEN("nil assignment") {
    WHEN("assigning to a record type") {
      auto errors = Check(R"(
let
  type myrec = { a: int }
  var r: myrec := nil
in
  r := nil
end)");
      REQUIRE(errors.empty());
    }

    WHEN("assigning to an int") {
      auto errors = Check(R"(
let
  var i := 0
in
  i := nil
end)");
      REQUIRE(errors.size() == 1);
      REQUIRE(errors[0] == "Type int is not a record type");
    }

    WHEN("assigning to a record type alias") {
      auto errors = Check(R"(
let
  type myrec = { a: int }
  type rec_alias = myrec
  var r: rec_alias := nil
in
  r := nil
end)");
      REQUIRE(errors.empty());
    }

    WHEN("assigning to a non-record type alias") {
      auto errors = Check(
          "let type notrecord = int var a:notrecord := 0 in a := nil end");
      REQUIRE(errors.size() == 1);
      REQUIRE(errors[0] == "Type notrecord is not a record type");
    }
  }

  GIVEN("Structure checks") {
    WHEN("Break outside loop") {
      auto errors = Check("break");
      REQUIRE(errors.size() == 1);
      REQUIRE(errors[0] == "Break must be inside a loop");
    }
    WHEN("Break inside loop") {
      auto errors = Check("while 1 do break");
      REQUIRE(errors.empty());
    }
    WHEN("Loop returning value") {
      auto errors = Check("while 1 do 5");
      REQUIRE(errors.size() == 1);
      REQUIRE(errors[0] == "Loop body must not return a value");
    }
    WHEN("For loop returning value") {
      auto errors = Check("for i := 0 to 10 do 5");
      REQUIRE(errors.size() == 1);
      REQUIRE(errors[0] == "Loop body must not return a value");
    }
    WHEN("For loop var assignment") {
      auto errors = Check("for i := 0 to 10 do i := i + 1");
      REQUIRE(errors.size() == 1);
      REQUIRE(errors[0] == "For loop variable i may not be assigned to");
    }
    WHEN("If-then-else mismatch") {
      auto errors = Check("if 1 then 5 else \"s\"");
      REQUIRE(errors.size() == 1);
      REQUIRE(errors[0] == "If-then-else branches must have same type");
    }
    WHEN("If-then-else void mismatch") {
      auto errors = Check("if 1 then 5 else ()");
      REQUIRE(errors.size() == 1);
      REQUIRE(errors[0] == "If-then-else branches must have same type");
    }
  }

  GIVEN("Declaration checks") {
    WHEN("Mutually recursive types cycle failure") {
      auto errors = Check("let type a = b type b = a in 0 end");
      REQUIRE(errors.size() >= 1);
      REQUIRE(StartsWith(errors[0], "Illegal mutually recursive type cycle"));
    }
    WHEN("Mutually recursive types valid") {
      auto errors = Check("let type a = b type b = {x:a} in 0 end");
      REQUIRE(errors.empty());
    }
    WHEN("Duplicate type definition") {
      auto errors = Check("let type a = int type a = string in 0 end");
      REQUIRE(errors.size() == 1);
      REQUIRE(errors[0] == "Duplicate type definition: a");
    }
    WHEN("Duplicate function definition") {
      auto errors =
          Check("let function f():int=0 function f():string=\"s\" in 0 end");
      REQUIRE(errors.size() == 1);
      REQUIRE(errors[0] == "Duplicate function definition: f");
    }
    WHEN("Variable init mismatch") {
      auto errors = Check("let var x:int := \"s\" in 0 end");
      REQUIRE(errors.size() == 1);
      REQUIRE(errors[0] ==
              "Variable x declared type int but initialized with string");
    }
    WHEN("Function return mismatch") {
      auto errors = Check("let function f():int = \"s\" in 0 end");
      REQUIRE(errors.size() == 1);
      REQUIRE(errors[0] ==
              "Function f declared to return int but body returns string");
    }
    WHEN("Procedure return value") {
      auto errors = Check("let function f() = 5 in 0 end");
      REQUIRE(errors.size() == 1);
      REQUIRE(errors[0] == "Procedure f body must not return a value");
    }
    WHEN("Variable no value") {
      // () is void/NOTYPE.
      auto errors = Check("let var x:int := () in 0 end");
      REQUIRE(errors.size() == 1);
      REQUIRE(errors[0] == "Variable x initialized with no value");
    }
  }

  GIVEN("Function call checks") {
    WHEN("Valid function call") {
      auto errors = Check(
          "let function f(a:int, b:string):int = a in f(5, \"hello\") end");
      REQUIRE(errors.empty());
    }
    WHEN("Argument count mismatch") {
      auto errors = Check("let function f(a:int):int = a in f(5, 6) end");
      REQUIRE(errors.size() == 1);
      REQUIRE(errors[0] == "Function f expects 1 arguments, but got 2");
    }
    WHEN("Argument type mismatch") {
      auto errors = Check("let function f(a:int):int = a in f(\"hello\") end");
      REQUIRE(errors.size() == 1);
      REQUIRE(errors[0] ==
              "Argument 1 of function f expects type int but got string");
    }
    WHEN("Argument type alias") {
      auto errors = Check("let type myint = int function f(a:myint):int = 5 "
                          "var x := 5 in f(x) end");
      REQUIRE(errors.empty());
    }
    WHEN("Argument nil for record") {
      auto errors =
          Check("let type r = {x:int} function f(a:r):int = 0 in f(nil) end");
      REQUIRE(errors.empty());
    }
    WHEN("Argument nil for int") {
      auto errors = Check("let function f(a:int):int = a in f(nil) end");
      REQUIRE(errors.size() == 1);
      // It errors on Type int is not a record type
      REQUIRE(errors[0] == "Type int is not a record type");
    }
  }
}

} // namespace
