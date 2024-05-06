
#include "type_finder.h"

namespace {
using namespace syntax;

// Helper function to concatenate std::string and std::string_view.
std::string operator+(std::string&& s, std::string_view v) {
  s.append(v.data(), v.size());
  return s;
}

// Returns syntax::Type* that holds either TypeFields or ArrayType for the given
// type-id. Traverses chain of alias types. Returns nullptr on error.
const Type* UnwrapTypeAliases(std::string_view type_id, const Expr& expr,
                              const SymbolTable& symbols,
                              std::vector<std::string>& errors) {
  const TypeDeclaration* td = symbols.lookupType(expr, type_id);
  while (true) {
    if (!td) {
      errors.emplace_back("Type not found: " + type_id);
      return nullptr;
    }
    const std::string* alias = std::get_if<Identifier>(&td->value);
    if (!alias) return &td->value;
    td = symbols.lookupType(expr, *alias);
  }
}

}  // namespace

std::string_view TypeFinder::operator()(const Expr& id) {
  if (auto i = cache_.find(&id); i != cache_.end()) {
    return i->second;
  }
  // Not all expressions have a value.
  // > Procedure calls, assignments, if-then, while, break, and sometimes
  // > if-then-else produce no value and may not appear where a value is
  // > expected (e.g., (a:=b)+c is illegal). A let expression with nothing
  // > between the `in` and `end` returns no value.
  std::string_view result = std::visit(
      Overloaded{
          [](const StringConstant&) -> std::string_view { return "string"; },
          [](const IntegerConstant&) -> std::string_view { return "int"; },
          [](const Negated& n) -> std::string_view { return "int"; },
          [](const RecordLiteral& rl) -> std::string_view {
            return rl.type_id;
          },
          [](const ArrayLiteral& al) -> std::string_view { return al.type_id; },
          [](const Nil&) -> std::string_view {
            // technically its type is determined from context
            return "NOTYPE";
          },
          [](const Assignment&) -> std::string_view { return "NOTYPE"; },
          [](const IfThen&) -> std::string_view { return "NOTYPE"; },
          [](const While&) -> std::string_view { return "NOTYPE"; },
          [](const For&) -> std::string_view { return "NOTYPE"; },
          [](const Break&) -> std::string_view { return "NOTYPE"; },
          [&](const std::unique_ptr<LValue>& l) {
            return GetLValueType(id, *l);
          },
          [&](const Binary& b) { return (*this)(*b.left); },
          [&](const IfThenElse& ite) { return (*this)(*ite.then_expr); },
          [&](const Let& l) -> std::string_view {
            return l.body.empty() ? "NOTYPE" : (*this)(*l.body.back());
          },
          [&](const Parenthesized& p) -> std::string_view {
            return p.exprs.empty() ? "NOTYPE" : (*this)(*p.exprs.back());
          },
          [&](const FunctionCall& fc) -> std::string_view {
            const auto* fd = symbols_.lookupFunction(id, fc.id);
            if (!fd) {
              errors_.emplace_back("Function not found: " + fc.id);
              return "NOTYPE";
            }
            // Ambiguous spec. Does a missing return type declaration
            // mean no type or inferred type? Generously assume the
            // latter.
            return fd->type_id ? *fd->type_id : (*this)(*fd->body);
          }},
      id);
  cache_.emplace(&id, result);
  return result;
}

// Returns the type-id of the given l-value, or "NOTYPE" in case of errors.
std::string_view TypeFinder::GetLValueType(const Expr& parent,
                                           const LValue& lvalue) {
  return std::visit(
      Overloaded{
          [&](const Identifier& name) -> std::string_view {
            return std::visit(
                Overloaded{
                    [&](const VariableDeclaration* vd) -> std::string_view {
                      return vd->type_id ? *vd->type_id : (*this)(*vd->value);
                    },
                    [&](const TypeField* tf) -> std::string_view {
                      return tf->type_id;
                    },
                    [&](std::nullptr_t) -> std::string_view {
                      errors_.emplace_back("Variable not found: " + name);
                      return "NOTYPE";
                    }},
                symbols_.lookupStorageLocation(parent, name));
          },
          [&](const RecordField& rf) -> std::string_view {
            // Example: `foo.bar`
            std::string_view record_type =
                this->GetLValueType(parent, *rf.l_value);
            const Type* t =
                UnwrapTypeAliases(record_type, parent, symbols_, errors_);
            if (!t) return "NOTYPE";
            const auto* tf = std::get_if<TypeFields>(t);
            if (!tf) {
              errors_.emplace_back("Record type expected: " + record_type);
              return "NOTYPE";
            }
            // Find field named rf.id in the record type.
            auto it =
                std::find_if(tf->begin(), tf->end(),
                             [&](const TypeField& f) { return f.id == rf.id; });
            if (it == tf->end()) {
              errors_.emplace_back("Record field not found: " + rf.id);
              return "NOTYPE";
            }
            return it->type_id;
          },
          [&](const ArrayElement& ae) -> std::string_view {
            // Example: `foo[7]`
            std::string_view array_type =
                this->GetLValueType(parent, *ae.l_value);
            const Type* t =
                UnwrapTypeAliases(array_type, parent, symbols_, errors_);
            if (!t) return "NOTYPE";
            const auto* at = std::get_if<ArrayType>(t);
            if (!at) {
              errors_.emplace_back("Array type expected: " + array_type);
              return "NOTYPE";
            }
            return at->element_type_id;
          }},
      lvalue);
}
