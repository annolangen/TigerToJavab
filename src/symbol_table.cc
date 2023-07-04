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
  std::unordered_map<std::string_view, const VariableDeclaration*> variable;
  std::unordered_map<std::string_view, const TypeDeclaration*> type;
};

class St : public SymbolTable {
 public:
  St(std::vector<Scope>&& scopes,
     std::unordered_map<const Expr*, const Scope*>&& scope_by_expr)
      : scopes_(std::move(scopes)), scope_by_expr_(std::move(scope_by_expr)) {}

  const FunctionDeclaration* lookupFunction(
      const Expr& expr, std::string_view name) const override {
    const Scope* s = Lookup(scope_by_expr_, &expr);
    return s ? Lookup(s->function, name) : nullptr;
  }

  const VariableDeclaration* lookupVariable(
      const Expr& expr, std::string_view name) const override {
    const Scope* s = Lookup(scope_by_expr_, &expr);
    return s ? Lookup(s->variable, name) : nullptr;
  }

  const TypeDeclaration* lookupType(const Expr& expr,
                                    std::string_view name) const override {
    const Scope* s = Lookup(scope_by_expr_, &expr);
    return s ? Lookup(s->type, name) : nullptr;
  }

  std::string toString() const override {
    std::ostringstream out;
    out << "SymbolTable{ #scopes: " << scopes_.size()
        << ", #expr: " << scope_by_expr_.size() << " }";
    return out.str();
  }

 private:
  std::vector<Scope> scopes_;
  std::unordered_map<const Expr*, const Scope*> scope_by_expr_;
};

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
    scopes.emplace_back(Scope{current});
    current = &scopes.back();
    if (!VisitChildren(v, *this)) return false;
    current = prev;
    return true;
  }

  bool operator()(const VariableDeclaration& v) {
    current->variable[v.id] = &v;
    return VisitChildren(v, *this);
  }

  bool operator()(const TypeDeclaration& v) {
    current->type[v.id] = &v;
    return true;
  }

  bool operator()(const Let& v) {
    Scope* prev = current;
    scopes.emplace_back(Scope{current});
    current = &scopes.back();
    if (!VisitChildren(v, *this)) return false;
    current = prev;
    return true;
  }

  St Build() { return St(std::move(scopes), std::move(scope_by_expr)); }

  std::vector<Scope> scopes = {{nullptr}};
  std::unordered_map<const Expr*, const Scope*> scope_by_expr;
  Scope* current = &scopes[0];
};
}  // namespace

std::unique_ptr<SymbolTable> SymbolTable::Build(const Expr& root) {
  StBuilder builder;
  std::visit(builder, root);
  return std::make_unique<St>(builder.Build());
}
