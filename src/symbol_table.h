#pragma once

#include <memory>
#include <string_view>

#include "syntax.h"

// Table to look up symbols in the AST starting from a given expression. Returns
// null if not found.
class SymbolTable {
 public:
  static std::unique_ptr<SymbolTable> Build(const syntax::Expr& root);
  virtual ~SymbolTable() = default;

  // Returns function declarations iwht given name visible in given expression.
  virtual const syntax::FunctionDeclaration* lookupFunction(
      const syntax::Expr& expr, std::string_view name) const = 0;

  // Returns variable declarations iwht given name visible in given expression.
  virtual const syntax::VariableDeclaration* lookupVariable(
      const syntax::Expr& expr, std::string_view name) const = 0;

  // Returns type declarations iwht given name visible in given expression.
  virtual const syntax::TypeDeclaration* lookupType(
      const syntax::Expr& expr, std::string_view name) const = 0;

  // Returns string for debugging.
  virtual std::string toString() const = 0;
};
