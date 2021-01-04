#include "Checker.h"
#include "testing/catch.h"
#include "testing/testing.h"
namespace {

std::vector<std::string> Check(const char* text) {
  std::shared_ptr<Expression> e = testing::Parse(text);
  Expression::SetNameSpacesBelow(*e);
  Expression::SetTypesBelow(*e);
  return ListErrors(*e);
}
SCENARIO("Static checking", "[checker]") {
  GIVEN("Valid Record literal") {
    std::vector<std::string> errors =
        Check("let type Bulk = {height:int, weight:int} in "
              "Bulk {height=6, weight=200} end");
    REQUIRE(errors.size() == 0);
  }
}

} // namespace
