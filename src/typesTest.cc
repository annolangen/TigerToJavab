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
      REQUIRE(InferType(Binary(aString, BinaryOp::kPlus, aString)) ==
              "string");
    }
    // Composite expressions
    // lvalue
    // - expr
    // expr binary-operator expr
    // lvalue := expr
    // id ( expr-listopt )
    // ( expr-seqopt )
    // if expr then expr
    // if expr then expr else expr
    // while expr do expr
    // for id := expr to expr do expr
    // let declaration-list in expr-seqopt end
  }
  GIVEN("Declarations") {}
}
} // namespace
