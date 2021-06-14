#include "ToString.h"
#include "testing/catch.h"
#include "testing/testing.h"
namespace syntax {
namespace {
using testing::Parse;

SCENARIO("ToString produces strings for all our classes", "[ToString]") {
  GIVEN("A RecordType") {
    TypeFields record_type({{"x", "string"}, {"y", "int"}});
    REQUIRE(ToString(record_type) == "x: string, y: int");
  }
  GIVEN("An ArrayType") {
    ArrayType array_type{"string"};
    REQUIRE(ToString(array_type) == "array of string");
  }
  GIVEN("Text inputs") {
    REQUIRE(ToString(*Parse("33")) == "33");
    REQUIRE(ToString(*Parse("\"Hello\"")) == R"("Hello")");
    REQUIRE(ToString(*Parse("nil")) == "nil");
    REQUIRE(ToString(*Parse("foo")) == "foo");
    REQUIRE(ToString(*Parse("foo.bar")) == "foo.bar");
    REQUIRE(ToString(*Parse("-33")) == "-33");
    REQUIRE(ToString(*Parse("2|2")) == "2 | 2");
    REQUIRE(ToString(*Parse("1+2-3*4/5=6<>7<8>9<=a>=b&c|d")) ==
            "1 + 2 - 3 * 4 / 5 = 6 <> 7 < 8 > 9 <= a >= b & c | d");
    REQUIRE(ToString(*Parse("a:=2")) == "a := 2");
    REQUIRE(ToString(*Parse("foo()")) == "foo()");
    REQUIRE(ToString(*Parse("sin(3)")) == "sin(3)");
    REQUIRE(ToString(*Parse("bar(3, 1)")) == "bar(3, 1)");
    REQUIRE(ToString(*Parse("(3; 1)")) == "(3; 1)");
    // Mystery: If this test is not last, the next test reads EOF!?!
    REQUIRE(ToString(*Parse("foo.bar[33]")) == "foo.bar[33]");
  }
}
} // namespace
} // namespace syntax
