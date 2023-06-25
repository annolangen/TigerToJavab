#pragma once
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "binary_op.h"

// Types and helper functions for the abstract syntax tree from
// http://www.cs.columbia.edu/~sedwards/classes/2002/w4115/tiger.pdf.
namespace syntax {
// Abstract syntax node types are a mix of structs, std::variants, and
// type aliases.

// forward declarations
class RecordField;
class ArrayElement;
class Negated;
class Binary;
class Assignment;
class FunctionCall;
class RecordLiteral;
class ArrayLiteral;
class IfThen;
class IfThenElse;
class While;
class For;
class Break;
class Let;
class Parenthesized;

using Identifier = std::string;
using LValue = std::variant<Identifier, RecordField, ArrayElement>;

struct StringConstant {
  std::string value;
};
using IntegerConstant = int;
struct Nil {};
using Expr = std::variant<StringConstant, IntegerConstant, Nil,
                          std::unique_ptr<LValue>, Negated, Binary, Assignment,
                          FunctionCall, RecordLiteral, ArrayLiteral, IfThen,
                          IfThenElse, While, For, Break, Let, Parenthesized>;
struct RecordField {
  std::unique_ptr<LValue> l_value;
  std::string id;
};
struct ArrayElement {
  std::unique_ptr<LValue> l_value;
  std::unique_ptr<Expr> expr;
};
struct Negated {
  std::unique_ptr<Expr> expr;
};
struct Binary {
  std::unique_ptr<Expr> left;
  BinaryOp op;
  std::unique_ptr<Expr> right;
};
struct Assignment {
  std::unique_ptr<LValue> l_value;
  std::unique_ptr<Expr> expr;
};
struct Parenthesized {
  std::vector<std::unique_ptr<Expr>> exprs;
};
struct FunctionCall {
  Identifier id;
  std::vector<std::unique_ptr<Expr>> arguments;
};
struct FieldAssignment {
  std::string id;
  std::unique_ptr<Expr> expr;
};
struct RecordLiteral {
  std::string type_id;
  std::vector<FieldAssignment> fields;
};
struct ArrayLiteral {
  std::string type_id;
  std::unique_ptr<Expr> size;
  std::unique_ptr<Expr> value;
};
struct IfThen {
  std::unique_ptr<Expr> condition;
  std::unique_ptr<Expr> then_expr;
};
struct IfThenElse {
  std::unique_ptr<Expr> condition;
  std::unique_ptr<Expr> then_expr;
  std::unique_ptr<Expr> else_expr;
};
struct While {
  std::unique_ptr<Expr> condition;
  std::unique_ptr<Expr> body;
};
struct For {
  std::string id;
  std::unique_ptr<Expr> start;
  std::unique_ptr<Expr> end;
  std::unique_ptr<Expr> body;
};
struct Break {};
struct VariableDeclaration {
  std::string id;
  std::unique_ptr<Expr> value;
  std::optional<std::string> type_id;
};
struct TypeField {
  std::string id;
  std::string type_id;
};
using TypeFields = std::vector<TypeField>;
struct ArrayType {
  std::string element_type_id;
};
struct FunctionDeclaration {
  std::string id;
  TypeFields parameter;
  std::unique_ptr<Expr> body;
  std::optional<std::string> type_id;
};
using Type = std::variant<Identifier, TypeFields, ArrayType>;
struct TypeDeclaration {
  std::string id;
  Type value;
};
using Declaration =
    std::variant<TypeDeclaration, VariableDeclaration, FunctionDeclaration>;
struct Let {
  std::vector<std::unique_ptr<Declaration>> declaration;
  std::vector<std::unique_ptr<Expr>> body;
};

// Generic struct of lambdas for ad-hoc visitors.
template <class... F>
struct Overloaded : F... {
  using F::operator()...;
};
template <class... F>
Overloaded(F...) -> Overloaded<F...>;

// Overloaded template function for visiting child nodes. Returns false to
// indicate stop.
template <class F>
bool VisitChildren(const StringConstant& v, F&& f) {
  return true;
}
template <class F>
bool VisitChildren(const Identifier& v, F&& f) {
  return true;
}
template <class F>
bool VisitChildren(const IntegerConstant& v, F&& f) {
  return true;
}
template <class F>
bool VisitChildren(const Nil& v, F&& f) {
  return true;
}
template <class F>
bool VisitChildren(const Break& v, F&& f) {
  return true;
}
template <class F>
bool VisitChildren(const TypeField& v, F&& f) {
  return true;
}
template <class F>
bool VisitChildren(const RecordField& v, F&& f) {
  return std::invoke(f, *v.l_value);
}
template <class F>
bool VisitChildren(const ArrayElement& v, F&& f) {
  return std::invoke(f, *v.l_value) && std::invoke(f, *v.expr);
}
template <class F>
bool VisitChildren(const Negated& v, F&& f) {
  return std::invoke(f, *v.expr);
}
template <class F>
bool VisitChildren(const Binary& v, F&& f) {
  return std::invoke(f, *v.left) && std::invoke(f, *v.right);
}
template <class F>
bool VisitChildren(const Assignment& v, F&& f) {
  return std::invoke(f, *v.l_value) && std::invoke(f, *v.expr);
}
template <class F>
bool VisitChildren(const FunctionCall& v, F&& f) {
  for (const auto& e : v.arguments) {
    if (!std::invoke(f, *e)) return false;
  }
  return true;
}
template <class F>
bool VisitChildren(const FieldAssignment& v, F&& f) {
  return std::invoke(f, *v.expr);
}
template <class F>
bool VisitChildren(const RecordLiteral& v, F&& f) {
  for (const FieldAssignment& a : v.fields) {
    if (!std::invoke(f, a)) return false;
  }
  return true;
}
template <class F>
bool VisitChildren(const ArrayLiteral& v, F&& f) {
  return std::invoke(f, *v.size) && std::invoke(f, *v.value);
}
template <class F>
bool VisitChildren(const IfThen& v, F&& f) {
  return std::invoke(f, *v.condition) && std::invoke(f, *v.then_expr);
}
template <class F>
bool VisitChildren(const IfThenElse& v, F&& f) {
  return std::invoke(f, *v.condition) && std::invoke(f, *v.then_expr) &&
         std::invoke(f, *v.else_expr);
}
template <class F>
bool VisitChildren(const While& v, F&& f) {
  return std::invoke(f, *v.condition) && std::invoke(f, *v.body);
}
template <class F>
bool VisitChildren(const For& v, F&& f) {
  return std::invoke(f, *v.start) && std::invoke(f, *v.end) &&
         std::invoke(f, *v.body);
}
template <class F>
bool VisitChildren(const Parenthesized& v, F&& f) {
  for (const auto& e : v.exprs) {
    if (!std::invoke(f, *e)) return false;
  }
  return true;
}
template <class F>
bool VisitChildren(const LValue& v, F&& f) {
  switch (v.index()) {
    case 1:
      return VisitChildren(std::get<1>(v), f);
    case 2:
      return VisitChildren(std::get<2>(v), f);
    default:
      return true;
  }
  // return std::visit([&f](auto e) -> bool { return VisitChildren(e, f); }, v);
}
template <class F>
bool VisitChildren(const std::unique_ptr<LValue>& v, F&& f) {
  return VisitChildren(*v, f);
}
template <class F>
bool VisitChildren(const Expr& v, F&& f) {
  switch (v.index()) {
    case 3:
      return VisitChildren(*std::get<3>(v), f);
    case 4:
      return f(*std::get<4>(v).expr);
    case 5:
      return VisitChildren(std::get<5>(v), f);
    case 6:
      return VisitChildren(std::get<6>(v), f);
    case 7:
      return VisitChildren(std::get<7>(v), f);
    case 8:
      return VisitChildren(std::get<8>(v), f);
    case 9:
      return VisitChildren(std::get<9>(v), f);
    case 10:
      return VisitChildren(std::get<10>(v), f);
    case 11:
      return VisitChildren(std::get<11>(v), f);
    case 12:
      return VisitChildren(std::get<12>(v), f);
    case 13:
      return VisitChildren(std::get<13>(v), f);
    case 14:
      return VisitChildren(std::get<14>(v), f);
    case 15:
      return VisitChildren(std::get<15>(v), f);
    case 16:
      return VisitChildren(std::get<16>(v), f);
    default:
      return true;
  }
  // return std::visit([&f](auto e) -> bool { return VisitChildren(e, f); }, v);
}
template <class F>
bool VisitChildren(const Type& v, F&& f) {
  switch (v.index()) {
    case 1:
      return VisitChildren(std::get<1>(v), f);
    case 2:
      return VisitChildren(std::get<2>(v), f);
    default:
      return true;
  }
  // return std::visit([&f](auto e) -> bool { return VisitChildren(e, f); }, v);
}
template <class F>
bool VisitChildren(const Declaration& v, F&& f) {
  switch (v.index()) {
    case 0:
      return VisitChildren(std::get<0>(v), f);
    case 1:
      return VisitChildren(std::get<1>(v), f);
    case 2:
      return VisitChildren(std::get<2>(v), f);
    default:
      return true;
  }
  // return std::visit([&f](auto e) -> bool { return VisitChildren(e, f); }, v);
}
template <class F>
bool VisitChildren(const VariableDeclaration& v, F&& f) {
  return std::invoke(f, *v.value);
}
template <class F>
bool VisitChildren(const FunctionDeclaration& v, F&& f) {
  return std::invoke(f, *v.body);
}
template <class F>
bool VisitChildren(const TypeDeclaration& v, F&& f) {
  return true;
}
template <class F>
bool VisitChildren(const Let& v, F&& f) {
  for (const auto& d : v.declaration) {
    if (!std::invoke(f, *d)) return false;
  }
  for (const auto& e : v.body) {
    if (!std::invoke(f, *e)) return false;
  }
  return true;
}
}  // namespace syntax
