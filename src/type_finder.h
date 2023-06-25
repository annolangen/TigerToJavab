#pragma once
#include <string_view>

#include "symbol_table.h"
#include "syntax.h"

// Type inference function for Tiger expressions. Caches results. Therefore, it
// should be created once and passed by reference. Errors detected during type
// inference, like undeclared variables, are appended to the given vector.
class TypeFinder {
 public:
  TypeFinder(const SymbolTable& symbols, std::vector<std::string>& errors)
      : errors_(errors), symbols_(symbols) {}
  TypeFinder() = delete;
  ~TypeFinder() = default;
  TypeFinder(const TypeFinder&) = delete;
  TypeFinder& operator=(const TypeFinder&) = delete;
  TypeFinder(TypeFinder&&) = delete;
  TypeFinder& operator=(TypeFinder&&) = delete;

  // Returns the type-id of the given expression, or "NOTYPE" in case of errors.
  std::string_view operator()(const syntax::Expr& id);

 private:
  std::string_view GetLValueType(const syntax::Expr& parent,
                                 const syntax::LValue& lvalue);
  const SymbolTable& symbols_;
  std::vector<std::string>& errors_;
  std::unordered_map<const syntax::Expr*, std::string_view> cache_;
};