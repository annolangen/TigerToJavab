#pragma once
#include <optional>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

// Scoped map inserts new bindings in a given scope. When a scope exits
// all its bindings are dropped.
template <class T> class ScopedMap {
public:
  // Pushes a new scope.
  void EnterScope() { maps_.push_back({}); }

  // Exits last scope and drops all entries bound since the last
  // EnterScope.
  void ExitScope() { maps_.pop_back(); }

  // Returns bound value from latest scope where the given key is bound.
  std::optional<T> Lookup(const std::string& key) const {
    for (auto i = maps_.rbegin(); i != maps_.rend(); ++i) {
      if (auto j = i->find(key); j != i->end()) return j->second;
    }
    return {};
  }

  // Returns true if the given key is bound in the currenr scope,
  bool IsBound(const std::string& key) {
    return maps_.rbegin()->count(key) == 1;
  }

  // Adds or overwrites a new entry in the current scope
  T& operator[](std::string key) { return (*maps_.rbegin())[key]; }

  template <class X>
  friend std::ostream& operator<<(std::ostream& os, const ScopedMap<X>& map);

private:
  std::vector<std::unordered_map<std::string, T>> maps_;
};

template <class T>
std::ostream& operator<<(std::ostream& os, const ScopedMap<T>& map) {
  os << "{";
  const char* sep = "";
  for (const auto& m : map.maps_) {
    for (const auto& p : m) {
      os << sep << p.first << ": " << p.second;
      sep = ",\n";
    }
  }
  return os << "}";
}
