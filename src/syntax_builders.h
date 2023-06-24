#pragma once
#include <memory>
#include <vector>

#include "syntax.h"

namespace syntax {
template <class T, class V = std::vector<std::unique_ptr<T>>>
V Add(V v, std::unique_ptr<T> e) {
  v.emplace_back(std::move(e));
  return std::move(v);
}

std::vector<FieldAssignment> Add(std::vector<FieldAssignment> v, std::string id,
                                 std::unique_ptr<Expr> expr) {
  v.emplace_back(FieldAssignment{std::move(id), std::move(expr)});
  return std::move(v);
}
}  // namespace syntax
