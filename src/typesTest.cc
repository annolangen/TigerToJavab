#include "syntax_nodes.h"
#include "testing/catch.h"
#include "testing/testing.h"
namespace {
using testing::Parse;

std::string InferTypeFromParse(const char* text) {
  std::shared_ptr<Expression> e = Parse(text);
  Expression::SetNameSpacesBelow(*e);
  Expression::SetTypesBelow(*e);
  return e->GetType();
}

const std::string& InferType(Expression& e) {
  Expression::SetNameSpacesBelow(e);
  Expression::SetTypesBelow(e);
  return e.GetType();
}

#define HasType(text, type) REQUIRE(InferTypeFromParse(text) == (type))

SCENARIO("types functions", "[types]") {
  GIVEN("Leaf expressions") {
    auto anInt = []() { return new IntegerConstant(3); };
    REQUIRE(InferType(*anInt()) == "int");
    auto aString = []() { return new StringConstant("Hello"); };
    REQUIRE(InferType(*aString()) == "string");
    auto aBreak = std::make_shared<Break>();
    REQUIRE(InferType(*aBreak) == "none");
    auto anArray = std::make_shared<Array>("IntArray", anInt(), anInt());
    REQUIRE(InferType(*anArray) == "IntArray");
    std::vector<FieldValue> field_values;
    field_values.push_back(
        {"height", std::move(std::make_unique<IntegerConstant>(6))});
    field_values.push_back(
        {"weight", std::move(std::make_unique<IntegerConstant>(200))});
    auto aRecord = std::make_shared<Record>("Bulk", std::move(field_values));
    REQUIRE(InferType(*aRecord) == "Bulk");
    GIVEN("Declarations") {
      std::vector<std::shared_ptr<Declaration>> declarations;
      declarations.push_back(
          std::make_unique<VariableDeclaration>("n", anInt()));
      declarations.push_back(
          std::make_unique<VariableDeclaration>("s", aString()));
      declarations.push_back(std::make_unique<FunctionDeclaration>(
          "f", std::vector<TypeField>(), anInt()));
      declarations.push_back(std::make_unique<FunctionDeclaration>(
          "f", std::vector<TypeField>(), "T", anInt()));
      WHEN("n") {
        std::vector<std::shared_ptr<Expression>> body;
        body.push_back(std::make_unique<IdLValue>("n"));
        Let let(std::move(declarations), std::move(body));
        //        REQUIRE(InferType(let) == "int");
      }
    }
    GIVEN("Parsed test") {
      HasType("3", "int");
      HasType("\"Hello\"", "string");
      //      HasType("nil", "none");
      HasType("break", "none");
      HasType("IntArray [3] of 0", "IntArray");
      HasType("Bulk {height=6, weight=200}", "Bulk");
      HasType("-3", "int");
      HasType("1*1", "int");
      HasType("\"hello\"+\"world\"", "string");
      HasType("if 1 then \"hello\"", "none");
      HasType("if 1 then \"you\" else \"world\"", "string");
      HasType("while 1 do \"hello\"", "none");
      HasType("for i := 1 to 3 do 6", "none");
      HasType("let var a : int := 3 in a end", "int");
      HasType("let var a := 666 in a + 3 end", "int");
    }
    GIVEN("Complex case") {
      HasType("let type T = int in let type T = string var a : T := \"Hello\" "
              "in a end end",
              "T");
      HasType("a", "???");
      HasType("let function f():int = g() function g():int = f() in f() end",
              "int");
      HasType("let function f() = g() function g() = f() in f() end", "unset");
    }

    // Composite expressions
    // lvalue
    // lvalue := expr
    // id ( expr-listopt )
    // let declaration-list in expr-seqopt end
  }
}
} // namespace
