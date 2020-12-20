#include "ToString.h"
#include "testing/catch.h"

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
}
