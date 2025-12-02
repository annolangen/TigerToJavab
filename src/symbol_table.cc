#include "symbol_table.h"

#include <iostream>
#include <sstream>
#include <string_view>
#include <unordered_map>

namespace {
using namespace syntax;

template <class T, class K>
const T* Lookup(const std::unordered_map<K, const T*>& map, K key) {
  auto it = map.find(key);
  return it != map.end() ? it->second : nullptr;
}

struct Scope {
  const Scope* parent;
  std::unordered_map<std::string_view, const FunctionDeclaration*> function;
  std::unordered_map<std::string_view, StorageLocation> storage;
  std::unordered_map<std::string_view, const TypeDeclaration*> type;
};

// Owns all Scopes and a map pointing each expression (by its address) to its
// scope. Each lookup member function consists of seaching each scope in the
// parent chain for the symbol.
class St : public SymbolTable {
 public:
  St(std::vector<std::unique_ptr<Scope>>&& scopes,
     std::unordered_map<const Expr*, const Scope*>&& scope_by_expr)
      : scopes_(std::move(scopes)), scope_by_expr_(std::move(scope_by_expr)) {}

  const FunctionDeclaration* lookupFunction(
      const Expr& expr, std::string_view name) const override {
    for (const Scope* s = Lookup(scope_by_expr_, &expr); s; s = s->parent) {
      if (const auto* d = Lookup(s->function, name); d) return d;
    }
    return nullptr;
  }

  StorageLocation lookupStorageLocation(const Expr& expr,
                                        std::string_view name) const override {
    for (const Scope* s = Lookup(scope_by_expr_, &expr); s; s = s->parent) {
      if (auto it = s->storage.find(name); it != s->storage.end()) {
        return it->second;
      }
    }
    return nullptr;
  }

  const VariableDeclaration* lookupVariable(
      const Expr& expr, std::string_view name) const override {
    StorageLocation d = lookupStorageLocation(expr, name);
    if (auto v = std::get_if<const VariableDeclaration*>(&d); v) return *v;
    return nullptr;
  }

  const TypeDeclaration* lookupType(const Expr& expr,
                                    std::string_view name) const override {
    for (const Scope* s = Lookup(scope_by_expr_, &expr); s; s = s->parent) {
      if (const auto* d = Lookup(s->type, name); d) return d;
    }
    return nullptr;
  }

  const TypeDeclaration* lookupUnaliasedType(
      const Expr& expr, std::string_view name) const override {
    const TypeDeclaration* decl = lookupType(expr, name);
    while (decl) {
      const auto* alias = std::get_if<Identifier>(&decl->value);
      if (!alias) return decl;
      decl = lookupType(expr, *alias);
    }
    return nullptr;
  }

  std::string toString() const override {
    std::ostringstream out;
    out << "SymbolTable{ #scopes: " << scopes_.size()
        << ", #expr: " << scope_by_expr_.size() << " }";
    return out.str();
  }

 private:
  std::vector<std::unique_ptr<Scope>> scopes_;
  std::unordered_map<const Expr*, const Scope*> scope_by_expr_;
};

// A visitor that creates and populates all the Scopes. FunctionDeclaration and
// Let create a new Scope. The `Build` function transfers ownership of the
// Scopes and the map of Expr to Scope to the returned SymbolTable.
struct StBuilder {
  template <class T>
  bool operator()(const T& v) {
    return VisitChildren(v, *this);
  }

  bool operator()(const Expr& v) {
    scope_by_expr[&v] = current;
    return VisitChildren(v, *this);
  }

  bool operator()(const Declaration& d) { return std::visit(*this, d); }

  bool operator()(const FunctionDeclaration& v) {
    current->function[v.id] = &v;
    Scope* prev = current;
    scopes.emplace_back(std::make_unique<Scope>(Scope{.parent = current}));
    current = scopes.back().get();
    for (const TypeField& p : v.parameter) {
      current->storage[p.id] = &p;
    }
    if (!VisitChildren(v, *this)) return false;
    current = prev;
    return true;
  }

  bool operator()(const VariableDeclaration& v) {
    current->storage[v.id] = &v;
    return VisitChildren(v, *this);
  }

  bool operator()(const TypeDeclaration& v) {
    current->type[v.id] = &v;
    return true;
  }

  bool operator()(const Let& v) {
    Scope* prev = current;
    scopes.emplace_back(std::make_unique<Scope>(Scope{.parent = current}));
    current = scopes.back().get();
    // First pass: add all declarations to the new scope.
    for (const auto& decl : v.declaration) {
      std::visit(Overloaded{[&](const TypeDeclaration& d) {
                              current->type[d.id] = &d;
                            },
                            [&](const FunctionDeclaration& d) {
                              current->function[d.id] = &d;
                            },
                            [&](const VariableDeclaration& d) {
                              current->storage[d.id] = &d;
                            }},
                 *decl);
    }
    // Second pass: visit children to handle nested scopes and expressions.
    if (!VisitChildren(v, *this)) return false;
    current = prev;
    return true;
  }

  St Build() { return St(std::move(scopes), std::move(scope_by_expr)); }

  std::vector<std::unique_ptr<Scope>> scopes;
  std::unordered_map<const Expr*, const Scope*> scope_by_expr;
  Scope* current =
      (scopes.emplace_back(std::make_unique<Scope>(Scope{.parent = nullptr})),
       scopes[0].get());
};
}  // namespace

std::unique_ptr<SymbolTable> SymbolTable::Build(const Expr& root) {
  StBuilder builder;
  std::visit(builder, root);
  return std::make_unique<St>(builder.Build());
}
