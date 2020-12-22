#include "types.h"
#include "testing/catch.h"
namespace {
using types::InferType;
SCENARIO("types functions", "[types]") {

  GIVEN("Leaf expressions") {
    auto anInt = std::make_shared<IntegerConstant>(3);
    REQUIRE(InferType(*anInt) == "int");
    auto aString = std::make_shared<StringConstant>("Hello");
    REQUIRE(InferType(*aString) == "string");
    auto aNil = std::make_shared<Nil>();
    REQUIRE(InferType(*aNil) == "none");
    auto aBreak = std::make_shared<Break>();
    REQUIRE(InferType(*aBreak) == "none");
    auto anArray = std::make_shared<Array>("IntArray", anInt, anInt);
    REQUIRE(InferType(*anArray) == "IntArray");
    std::vector<FieldValue> field_values;
    field_values.push_back(
        {"height", std::move(std::make_unique<IntegerConstant>(6))});
    field_values.push_back(
        {"weight", std::move(std::make_unique<IntegerConstant>(200))});
    auto aRecord = std::make_shared<Record>("Bulk", std::move(field_values));
    REQUIRE(InferType(*aRecord) == "Bulk");
    WHEN("composed") {
      REQUIRE(InferType(Negated(anInt)) == "int");
      REQUIRE(InferType(Binary(anInt, BinaryOp::kTimes, anInt)) == "int");
      REQUIRE(InferType(Binary(aString, BinaryOp::kPlus, aString)) == "string");
      REQUIRE(InferType(IfThen(anInt, aString)) == "none");
      REQUIRE(InferType(IfThenElse(anInt, aString, aString)) == "string");
      REQUIRE(InferType(Block({anInt, aString})) == "string");
      REQUIRE(InferType(Block({aString, anInt})) == "int");
      REQUIRE(InferType(While(anInt, aString)) == "none");
      REQUIRE(InferType(For("i", anInt, anInt, aString)) == "none");
      REQUIRE(InferType(Let({}, {})) == "none");
    }
    GIVEN("Declarations") {
      std::vector<std::unique_ptr<Declaration>> declarations;
      declarations.push_back(std::make_unique<VariableDeclaration>("n", anInt));
      declarations.push_back(
          std::make_unique<VariableDeclaration>("s", aString));
      declarations.push_back(std::make_unique<FunctionDeclaration>(
          "f", std::vector<TypeField>(), anInt));
      declarations.push_back(std::make_unique<FunctionDeclaration>(
          "f", std::vector<TypeField>(), "T", anInt));
      WHEN("n") {
        std::vector<std::unique_ptr<Expression>> body;
        body.push_back(std::make_unique<IdLValue>("n"));
        Let let(std::move(declarations), std::move(body));
	// TODO get this working
        // REQUIRE(InferType(let) == "int");
      }
    }

    // Composite expressions
    // lvalue
    // lvalue := expr
    // id ( expr-listopt )
    // let declaration-list in expr-seqopt end
  }
}
} // namespace
