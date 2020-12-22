#pragma once
#include "Expression.h"
#include "ScopedMap.h"
#include <functional>
#include <string>

namespace types {

// Returns the type of the given expression. Returns "none", if the expression
// lacks a value.
std::string InferType(const Expression& e);

// Calls ExitScope on the referenced map, when exiting scope. Example use:
// ScopeExiter exiter{EnterScope(d, m)};
template <class T> struct ScopeExiter {
  ScopedMap<T>& map;
  ~ScopeExiter() { map.ExitScope(); }
};

// Inserts into a new scope the results of processing with the given function
// the declarations in the given vector. Returns the given scoped map for
// chaining.
template <class T>
ScopedMap<T>&
EnterScope(const std::vector<std::unique_ptr<Declaration>>& declarations,
           std::function<std::optional<T>(const Declaration&)> fn,
           ScopedMap<T>& map) {
  map.EnterScope();
  for (const auto& d : declarations) {
    if (auto t = fn(*d); t) map[d->Id()] = *t;
  }
  return map;
}

// Inserts all type declarations from the given vector into a new scope of the
// given type namespace. Returns the given type namespace.
ScopedMap<const Type*>&
EnterScope(const std::vector<std::unique_ptr<Declaration>>& declarations,
           ScopedMap<const Type*>& type_namespace);
} // namespace types
