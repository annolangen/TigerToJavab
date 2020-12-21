#include "ToString.h"
#include "testing/catch.h"
namespace {
SCENARIO("ToString produces strings for all our classes", "[ToString]") {

  GIVEN("A RecordType") {
    RecordType record_type({{"x", "string"}, {"y", "int"}});
    REQUIRE(ToString(record_type) == "{x: string, y: int}");
    WHEN("visited") {
      TypeVisitor visitor;
      record_type.Accept(visitor);
      THEN("ToString is unaffected") {
        REQUIRE(ToString(record_type) == "{x: string, y: int}");
      }
    }
  }
  GIVEN("An ArrayType") {
    ArrayType array_type("string");
    REQUIRE(ToString(array_type) == "array of string");
    WHEN("visited") {
      TypeVisitor visitor;
      array_type.Accept(visitor);
      THEN("ToString is unaffected") {
        REQUIRE(ToString(array_type) == "array of string");
      }
    }
  }
  GIVEN("A TypeReference") {
    TypeReference reference("string");
    REQUIRE(ToString(reference) == "string");
    WHEN("visited") {
      TypeVisitor visitor;
      reference.Accept(visitor);
      THEN("ToString is unaffected") {
        REQUIRE(ToString(reference) == "string");
      }
    }
  }
}
} // namespace
