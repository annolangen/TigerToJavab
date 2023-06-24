#include "syntax_insertion.h"

#include <functional>
#include <iostream>

template <class T>
struct Join {
  const std::vector<T>& v;
  std::string_view separator;
};
struct JoinBuilder {
  std::string_view separator;
  template <class T>
  Join<T> join(const std::vector<T>& v) {
    return Join<T>{v, separator};
  }
};
JoinBuilder with(std::string_view separator) { return JoinBuilder{separator}; }
template <class T>
std::ostream& MaybeDeref(std::ostream& os, const T& e) {
  return os << e;
}
template <>
std::ostream& MaybeDeref(std::ostream& os,
                         const std::unique_ptr<syntax::Expr>& e) {
  return os << *e;
}
template <class T>
std::ostream& operator<<(std::ostream& os, const Join<T>& j) {
  std::string_view sep = "";
  for (const T& e : j.v) {
    MaybeDeref(os << sep, e);
    sep = j.separator;
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const syntax::ArrayElement& a) {
  return os << *a.l_value << '[' << *a.expr << ']';
}
std::ostream& operator<<(std::ostream& os, const syntax::ArrayLiteral& a) {
  return os << a.type_id << '[' << *a.size << "] of " << *a.value;
}
std::ostream& operator<<(std::ostream& os, const syntax::ArrayType& a) {
  return os << "array of " << a.element_type_id;
}
std::ostream& operator<<(std::ostream& os, const syntax::Assignment& a) {
  return os << *a.l_value << " := " << *a.expr;
}
std::ostream& operator<<(std::ostream& os, const syntax::Binary& b) {
  return os << *b.left << " " << b.op << ' ' << *b.right;
}
std::ostream& operator<<(std::ostream& os, const syntax::Break&) {
  return os << "break";
}
std::ostream& operator<<(std::ostream& os, const syntax::Declaration& d) {
  std::visit([&os](auto&& v) { os << v; }, d);
  return os;
}
std::ostream& operator<<(std::ostream& os, const syntax::Expr& e) {
  std::visit([&os](auto&& v) { os << v; }, e);
  return os;
}
std::ostream& operator<<(std::ostream& os, const syntax::FieldAssignment& f) {
  return os << f.id << " = " << *f.expr;
}
std::ostream& operator<<(std::ostream& os, const syntax::For& f) {
  return os << "for " << f.id << " := " << *f.start << " to " << *f.end
            << " do " << *f.body;
}
std::ostream& operator<<(std::ostream& os, const syntax::FunctionCall& f) {
  return os << f.id << "(" << with(", ").join(f.arguments) << ")";
}
std::ostream& operator<<(std::ostream& os,
                         const syntax::FunctionDeclaration& f) {
  os << "function " << f.id << "(" << f.parameter << ")";
  if (f.type_id) os << ":" << *f.type_id;
  return os << " = " << *f.body;
}
std::ostream& operator<<(std::ostream& os, const syntax::IfThen& i) {
  return os << "if " << *i.condition << " then " << *i.then_expr;
}
std::ostream& operator<<(std::ostream& os, const syntax::IfThenElse& i) {
  return os << "if " << *i.condition << " then " << *i.then_expr << " else "
            << *i.else_expr;
}
std::ostream& operator<<(std::ostream& os, const syntax::Let& l) {
  os << "let\n";
  for (const auto& d : l.declaration) {
    os << *d << "\n";
  }
  return os << "in " << with("; ").join(l.body) << "end\n";
}
std::ostream& operator<<(std::ostream& os, const syntax::LValue& l) {
  std::visit([&os](auto&& v) { os << v; }, l);
  return os;
}
std::ostream& operator<<(std::ostream& os, const syntax::Negated& n) {
  return os << "-" << *n.expr;
}
std::ostream& operator<<(std::ostream& os, const syntax::Nil&) {
  return os << "nil";
}
std::ostream& operator<<(std::ostream& os, const syntax::Parenthesized& p) {
  return os << "(" << with("; ").join(p.exprs) << ")";
}
std::ostream& operator<<(std::ostream& os, const syntax::RecordField& r) {
  return os << *r.l_value << "." << r.id;
}
std::ostream& operator<<(std::ostream& os, const syntax::RecordLiteral& r) {
  return os << r.type_id << " {" << with(",\n").join(r.fields) << "}\n";
}
std::ostream& operator<<(std::ostream& os, const syntax::StringConstant& s) {
  return os << '"' << s.value << '"';
}
std::ostream& operator<<(std::ostream& os, const syntax::Type& t) {
  std::visit([&os](auto&& v) { os << v; }, t);
  return os;
}
std::ostream& operator<<(std::ostream& os, const syntax::TypeDeclaration& t) {
  return os << "type " << t.id << " = " << t.value;
}
std::ostream& operator<<(std::ostream& os, const syntax::TypeField& t) {
  return os << t.id << ": " << t.type_id;
}
std::ostream& operator<<(std::ostream& os, const syntax::TypeFields& t) {
  return os << with(", ").join(t);
}
std::ostream& operator<<(std::ostream& os,
                         const syntax::VariableDeclaration& v) {
  os << "var " << v.id;
  if (v.type_id) os << ": " << *v.type_id;
  return os << *v.value;
}
std::ostream& operator<<(std::ostream& os, const syntax::While& w) {
  return os << "while " << *w.condition << " do " << *w.body;
}

std::ostream& operator<<(std::ostream& os,
                         const std::unique_ptr<syntax::LValue>& lvp) {
  return os << *lvp;
}
