#include "DebugString.h"
#include "syntax_nodes.h"
#include "testing/catch.h"
#include "testing/testing.h"
namespace {
using testing::Parse;

SCENARIO("DebugString produces strings for all our classes", "[DebugString]") {
  GIVEN("A RecordType") {
    RecordType record_type({{"x", "string"}, {"y", "int"}});
    REQUIRE(DebugString(record_type) ==
            "RecordType{TypeField{id: x type_id: string} TypeField{id: y "
            "type_id: int}}");
  }
  GIVEN("An ArrayType") {
    ArrayType array_type("string");
    REQUIRE(DebugString(array_type) == "ArrayType{type_id: string}");
  }
  GIVEN("A TypeReference") {
    TypeReference reference("string");
    REQUIRE(DebugString(reference) == "TypeReference{id: string}");
  }
  GIVEN("Text inputs") {
    REQUIRE(DebugString(*Parse("33")) == "Int{33}");
    REQUIRE(DebugString(*Parse("\"Hello\"")) == "String{Hello}");
    REQUIRE(DebugString(*Parse("nil")) == "Nil");
    REQUIRE(DebugString(*Parse("foo")) == "Id{id: foo}");
    REQUIRE(DebugString(*Parse("foo.bar")) ==
            "Field{l_value: Id{id: foo}  id: bar}");
    REQUIRE(
        DebugString(*Parse("foo.bar[33]")) ==
        "Index{l_value: Field{l_value: Id{id: foo}  id: bar} expr: Int{33}}");
    REQUIRE(DebugString(*Parse("-33")) == "Negated{value: Int{33}}");
    REQUIRE(DebugString(*Parse("2|2")) ==
            "Binary{left: Int{2} op: | right: Int{2}}");

       
        REQUIRE(DebugString(*Parse("1+2*3")) == "Binary{left: Int{1} op: + right: Binary{left: Int{2} op: * right: Int{3}}}");
        REQUIRE(DebugString(*Parse("1*2+3")) == "Binary{left: Binary{left: Int{1} op: * right: Int{2}} op: + right: Int{3}}");






    REQUIRE(DebugString(*Parse("a:=2")) ==
            "Assign{l_value: Id{id: a} expr: Int{2}}");
    REQUIRE(DebugString(*Parse("foo()")) == "FunctionCall{id: foo }");
    REQUIRE(DebugString(*Parse("sin(3)")) ==
            "FunctionCall{id: sin arg: Int{3}}");
    REQUIRE(DebugString(*Parse("bar(3, 1)")) ==
            "FunctionCall{id: bar arg: Int{3} arg: Int{1}}");
    REQUIRE(DebugString(*Parse("(3; 1)")) == "Block{expr: Int{3} expr: Int{1}}");
  }
}
} // namespace
