#pragma once

#include <memory>
#include <string_view>

#include "syntax.h"

// Return value for lookupStorageLocation. Returns variable declaration,
// function parameter, or nullptr to indicate not found.
using StorageLocation = std::variant<const syntax::VariableDeclaration*,
                                     const syntax::TypeField*, std::nullptr_t>;

// Table to look up symbols in the AST starting from a given expression. Returns
// null if not found.
class SymbolTable {
 public:
  static std::unique_ptr<SymbolTable> Build(const syntax::Expr& root);
  virtual ~SymbolTable() = default;

  // Returns function declarations with given name visible in given expression.
  virtual const syntax::FunctionDeclaration* lookupFunction(
      const syntax::Expr& expr, std::string_view name) const = 0;

  // Returns variable declarations with given name visible in given expression.
  virtual const syntax::VariableDeclaration* lookupVariable(
      const syntax::Expr& expr, std::string_view name) const = 0;

  // Returns type declarations with given name visible in given expression.
  virtual const syntax::TypeDeclaration* lookupType(
      const syntax::Expr& expr, std::string_view name) const = 0;

  // Returns type declaration with given name visible in given expression,
  // traversing any type aliases.
  virtual const syntax::TypeDeclaration* lookupUnaliasedType(
      const syntax::Expr& expr, std::string_view name) const = 0;

  virtual StorageLocation lookupStorageLocation(
      const syntax::Expr& expr, std::string_view name) const = 0;

  // Returns string for debugging.
  virtual std::string toString() const = 0;
};
