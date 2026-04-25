#pragma once

#include <memory>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include "syntax.h"

// Return value for lookupStorageLocation. Returns variable declaration,
// function parameter, or nullptr to indicate not found.
// NOTE: For loop variables are returned as `const syntax::For*`.
// These are implicit Read-Only variables within the loop scope.
using StorageLocation = std::variant<const syntax::VariableDeclaration*,
    const syntax::TypeField*, const syntax::For*, std::nullptr_t>;

// Map of symbols to declarations. Scopes correspond to for-loopss and bodies of
// functions and let.
struct Scope {
  Scope() : id(0) {}
  explicit Scope(int id, const Scope* p) : id(id), parent(p) {}

  int id;
  const Scope* parent = nullptr;
  std::unordered_map<std::string_view, const syntax::FunctionDeclaration*>
      function;
  std::unordered_map<std::string_view, StorageLocation> storage;
  std::unordered_map<std::string_view, const syntax::TypeDeclaration*> type;
};

// Table to look up symbols in the AST starting from a given expression. Returns
// null if not found.
class SymbolTable {
 public:
  static std::unique_ptr<SymbolTable> Build(const syntax::Expr& root);
  virtual ~SymbolTable() = default;

  // Returns function declarations with given name visible in given expression.
  virtual const syntax::FunctionDeclaration* lookupFunction(
      const syntax::Expr& expr, std::string_view name) const = 0;

  // Prevents implicit conversion for better error messages
  template <typename T>
  const syntax::FunctionDeclaration* lookupFunction(
      const T&, std::string_view) const = delete;

  // Returns variable declarations with given name visible in given expression.
  virtual const syntax::VariableDeclaration* lookupVariable(
      const syntax::Expr& expr, std::string_view name) const = 0;

  // Prevents implicit conversion for better error messages
  template <typename T>
  const syntax::VariableDeclaration* lookupVariable(
      const T&, std::string_view) const = delete;

  // Returns type declarations with given name visible in given expression.
  // NOTE: This returns the direct definition of the type. If the type is an
  // alias (e.g. type a = b), this returns the TypeDeclaration for 'a' which
  // contains an Identifier 'b'. To get the underlying structural type, use
  // lookupUnaliasedType or manually unwind aliases.
  virtual const syntax::TypeDeclaration* lookupType(
      const syntax::Expr& expr, std::string_view name) const = 0;

  template <typename T>
  const syntax::TypeDeclaration* lookupType(
      const T&, std::string_view) const = delete;

  // Returns type declaration with given name visible in given expression,
  // traversing any type aliases.
  // NOTE: This unwinds aliases until a Record, Array, or Builtin type is found.
  // Use this when checking structural constraints (e.g. "is this a record?").
  virtual const syntax::TypeDeclaration* lookupUnaliasedType(
      const syntax::Expr& expr, std::string_view name) const = 0;

  template <typename T>
  const syntax::TypeDeclaration* lookupUnaliasedType(
      const T&, std::string_view) const = delete;

  virtual StorageLocation lookupStorageLocation(
      const syntax::Expr& expr, std::string_view name) const = 0;

  template <typename T>
  StorageLocation lookupStorageLocation(
      const T&, std::string_view) const = delete;

  // Returns the immediate scope for a given expression.
  virtual const Scope* getScope(const syntax::Expr& expr) const = 0;

  template <typename T>
  const Scope* getScope(const T&) const = delete;

  // Returns the scope where the given variable name is stored, starting from
  // the given expression.
  virtual const Scope* getDefiningScope(
      const syntax::Expr& expr, std::string_view name) const = 0;

  template <typename T>
  const Scope* getDefiningScope(const T&, std::string_view) const = delete;

  // Returns all scopes encountered thus far.
  virtual const std::vector<std::unique_ptr<Scope>>& scopes() const = 0;

  // Returns string for debugging.
  virtual std::string toString() const = 0;
};
