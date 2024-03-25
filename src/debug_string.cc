#include "debug_string.h"

#include <sstream>

namespace {

template <class T>
struct Str {
  const T& value;
  DebugStringOptions options;
};
template <class T>
Str(const T&, DebugStringOptions) -> Str<T>;
template <class T>
std::ostream& operator<<(std::ostream& out, const Str<T>& s) {
  return AppendDebugString(s.value, out, s.options);
}

}  // namespace

std::ostream& AppendDebugString(const syntax::Declaration& d, std::ostream& out,
                                DebugStringOptions options) {
  std::visit(syntax::Overloaded{
                 [&](const syntax::FunctionDeclaration& f) {
                   out << "function " << f.id << "(";
                   const char* sep = "";
                   for (const auto& param : f.parameter) {
                     out << sep << param.id << ": " << param.type_id;
                     sep = ", ";
                   }
                   out << "): " << f.type_id.value_or("NOTYPE") << std::endl
                       << options.indent;
                   DebugStringOptions body_options = options;
                   body_options.indent += options.indent;
                   out << Str{*f.body, body_options};
                 },
                 [&](const syntax::VariableDeclaration& v) {
                   out << "var " << v.id << ": " << v.type_id.value_or("NOTYPE")
                       << " := " << Str{*v.value, options};
                 },
                 [&](const syntax::TypeDeclaration& t) {
                   out << "type " << t.id << " = " << Str{t.value, options};
                 }},
             d);

  return out;
}

std::ostream& AppendDebugString(const syntax::Expr& e, std::ostream& out,
                                DebugStringOptions options) {
  std::visit(
      syntax::Overloaded{
          [&](const syntax::StringConstant& c) {
            out << "\"" << c.value << "\"";
          },
          [&](const syntax::IntegerConstant& c) { out << c; },
          [&](const syntax::Nil&) { out << "nil"; },
          [&](const std::unique_ptr<syntax::LValue>& l_value) {
            AppendDebugString(*l_value, out, options);
          },
          [&](const syntax::Negated& n) {
            out << "-" << Str{*n.expr, options};
          },
          [&](const syntax::Binary& b) {
            out << Str{*b.left, options} << " " << b.op << " "
                << Str{*b.right, options};
          },
          [&](const syntax::Assignment& a) {
            out << Str{*a.l_value, options} << " := " << Str{*a.expr, options};
          },
          [&](const syntax::FunctionCall& f) {
            out << f.id << "(";
            const char* sep = "";
            for (const auto& arg : f.arguments) {
              AppendDebugString(*arg, out << sep, options);
              sep = ", ";
            }
            out << ")";
          },
          [&](const syntax::RecordLiteral& r) {
            out << r.type_id << " {";
            const char* sep = "";
            for (const auto& field : r.fields) {
              out << sep << field.id << " = ";
              AppendDebugString(*field.expr, out, options);
              sep = ", ";
            }
            out << "}";
          },
          [&](const syntax::ArrayLiteral& a) {
            out << a.type_id << " [" << Str{*a.size, options} << "] of "
                << Str{*a.value, options};
          },
          [&](const syntax::IfThen& i) {
            out << "if " << Str{*i.condition, options} << " then "
                << Str{*i.then_expr, options};
          },
          [&](const syntax::IfThenElse& i) {
            out << "if " << Str{*i.condition, options} << " then "
                << Str{*i.then_expr, options} << " else "
                << Str{*i.else_expr, options};
          },
          [&](const syntax::While& w) {
            out << "while (" << Str{*w.condition, options} << ") "
                << Str{*w.body, options};
          },
          [&](const syntax::For& f) {
            out << "for " << f.id << " := " << Str{*f.start, options} << " to "
                << Str{*f.end, options} << " do " << Str{*f.body, options};
          },
          [&](const syntax::Break&) { out << "break"; },
          [&](const syntax::Let& l) {
            DebugStringOptions body_options = options;
            body_options.indent += options.indent;
            out << "let" << std::endl;
            for (const auto& decl : l.declaration) {
              out << body_options.indent << Str{*decl, options} << std::endl;
            }
            out << "in" << std::endl;
            for (const auto& expr : l.body) {
              out << body_options.indent << Str{*expr, body_options}
                  << std::endl;
            }
            out << "end";
          },
          [&](const syntax::Parenthesized& p) {
            out << "(";
            const char* sep = "";
            for (const auto& expr : p.exprs) {
              out << sep << Str{*expr, options};
              sep = "; ";
            }
            out << ")";
          }},
      e);
  return out;
}

std::ostream& AppendDebugString(const syntax::LValue& v, std::ostream& out,
                                DebugStringOptions options) {
  std::visit(
      syntax::Overloaded{[&](const syntax::Identifier& id) { out << id; },
                         [&](const syntax::RecordField& f) {
                           out << Str{*f.l_value, options} << "." << f.id;
                         },
                         [&](const syntax::ArrayElement& a) {
                           out << Str{*a.l_value, options} << "["
                               << Str{*a.expr, options} << "]";
                         }},
      v);
  return out;
}

std::ostream& AppendDebugString(const syntax::Type& t, std::ostream& out,
                                DebugStringOptions options) {
  std::visit(
      syntax::Overloaded{[&](const syntax::Identifier& id) { out << id; },
                         [&](const syntax::TypeFields& f) {
                           out << " {";
                           const char* sep = "";
                           for (const auto& field : f) {
                             out << sep << field.id << ": " << field.type_id;
                             sep = ", ";
                           }
                           out << "}";
                         },
                         [&](const syntax::ArrayType& a) {
                           out << "array of " << a.element_type_id;
                         }},
      t);
  return out;
}
