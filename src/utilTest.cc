#include "util.h"
#include "ToString.h"
#include "testing/catch.h"
#include <map>
#include <vector>

namespace {
int GetSecond(const std::pair<int, int>& p) { return p.second; }

SCENARIO("util functions", "[util]") {
  GIVEN("A simple vector") {
    std::vector<int> v = {1, 2, 3};
    REQUIRE(ToString(v | util::join("+")) == "1+2+3");
  }
  GIVEN("A map") {
    std::map<int, int> big_by_small = {{5, 100}, {6, 200}};
    REQUIRE(ToString(big_by_small |
                     util::map([](const auto& p) { return p.first; }) |
                     util::join("+")) == "5+6");
    REQUIRE(ToString(big_by_small | util::map(GetSecond) | util::join(", ")) ==
            "100, 200");
  }
}
} // namespace
