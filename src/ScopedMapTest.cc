#include "ScopedMap.h"
#include "testing/catch.h"

SCENARIO("ScopedMap handles binding in scopes", "[ScopedMap]") {

  GIVEN("A ScopedMap<int>") {
    ScopedMap<int> map;
    WHEN("empty") {
      THEN("Lookup works, but returns empty optional") {
        REQUIRE(!map.Lookup("foo"));
      }
    }
    WHEN("Two scopes") {
      map.EnterScope();
      map["foo"] = 7;
      map.EnterScope();
      map["foo"] = 666;
      THEN("New binding shadows old") {
        auto v = map.Lookup("foo");
        REQUIRE(v);
        REQUIRE(**v == 666);
      }
      THEN("Old value is restored on scope exit") {
        map.ExitScope();
        auto v = map.Lookup("foo");
        REQUIRE(v);
        REQUIRE(**v == 7);
      }
    }
  }
}
