#include "Checker.h"
#include "testing/catch.h"
#include "testing/testing.h"
#include <string_view>

namespace {

std::vector<std::string> Check(const char* text) {
  std::shared_ptr<Expression> e = testing::Parse(text);
  Expression::SetNameSpacesBelow(*e);
  Expression::SetTypesBelow(*e);
  return ListErrors(*e);
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
    REQUIRE(errors[0] == "Field counts differ for "
                         "Bulk {height: int, weight: int} and "
                         "{height: 6}");
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
}

} // namespace
