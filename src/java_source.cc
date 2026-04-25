#include "java_source.h"

#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <variant>
#include <vector>

#include "debug_string.h"
#include "symbol_table.h"
#include "syntax.h"
#include "type_finder.h"

namespace java {

using syntax::Overloaded;

std::string Sanitize(std::string id) {
  static std::unordered_set<std::string_view> kJavaKeywords = {"abstract",
      "assert", "boolean", "break", "byte", "case", "catch", "char", "class",
      "const", "continue", "default", "do", "double", "else", "enum", "extends",
      "final", "finally", "float", "for", "goto", "if", "implements", "import",
      "instanceof", "int", "interface", "long", "native", "new", "package",
      "private", "protected", "public", "return", "short", "static", "strictfp",
      "super", "switch", "synchronized", "this", "throw", "throws", "transient",
      "try", "void", "volatile", "while"};
  return kJavaKeywords.count(id) ? "_" + id : id;
}

std::string GetJavaType(TypeFinder& types, const SymbolTable& symbols,
    const syntax::Expr& expr, std::string_view t) {
  if (t == "int") return "int";
  if (t == "string") return "String";
  if (t.empty() || t == "NOTYPE" || t == "void") return "Object";
  if (const syntax::TypeDeclaration* decl =
          symbols.lookupUnaliasedType(expr, t)) {
    if (auto arr = std::get_if<syntax::ArrayType>(&decl->value)) {
      return GetJavaType(types, symbols, expr, arr->element_type_id) + "[]";
    }
  }
  return Sanitize(std::string(t));
}

std::string GetJavaType(
    TypeFinder& types, const SymbolTable& symbols, const syntax::Expr& expr) {
  return GetJavaType(types, symbols, expr, types(expr));
}

std::string GetJavaType(TypeFinder& /*types*/, std::string_view t) {
  if (t == "int") return "int";
  if (t == "string") return "String";
  if (t.empty() || t == "NOTYPE" || t == "void") return "Object";
  return Sanitize(std::string(t));
}

bool NeedsSemicolon(TypeFinder& types, const syntax::Expr& expr) {
  if (std::holds_alternative<syntax::IfThen>(expr) ||
      std::holds_alternative<syntax::While>(expr) ||
      std::holds_alternative<syntax::For>(expr) ||
      std::holds_alternative<syntax::Let>(expr) ||
      std::holds_alternative<syntax::Parenthesized>(expr)) {
    return false;
  }
  if (auto ite = std::get_if<syntax::IfThenElse>(&expr)) {
    std::string_view type = types(*ite->then_expr);
    return !type.empty() && type != "void" && type != "Object" &&
           type != "NOTYPE";
  }
  return true;
}

void PrintScopes(const SymbolTable& t, TypeFinder& tf, std::ostream& out) {
  for (const auto& scope_ptr : t.scopes()) {
    const Scope* scope = scope_ptr.get();
    out << "class Scope" << scope->id << " {\n";
    if (scope->parent) {
      out << "  public Scope" << scope->parent->id << " parent;\n";
    }
    for (const auto& kv : scope->storage) {
      std::visit(Overloaded{[&](const syntax::VariableDeclaration* var) {
        std::string type = var->type_id
                               ? GetJavaType(tf, t, *var->value, *var->type_id)
                               : GetJavaType(tf, t, *var->value);
        out << "  public " << type << " " << Sanitize(std::string(kv.first))
            << ";\n";
      },
                     [&](const syntax::For*) {
        out << "  public int " << Sanitize(std::string(kv.first)) << ";\n";
      },
                     [&](const syntax::TypeField* param) {
        out << "  public " << GetJavaType(tf, param->type_id) << " "
            << Sanitize(std::string(kv.first)) << ";\n";
      }, [](std::nullptr_t) {}},
          kv.second);
    }
    out << "}\n\n";
  }
}

struct Compiler {
  const SymbolTable& symbols;
  TypeFinder& types;
  std::ostream& out;
  std::ostream& post_body;
  std::string current_lvalue;

  const Scope* local_scope = nullptr;
  const syntax::Expr* current_expr = nullptr;
  const Scope* req_scope = nullptr;
  int indent_level = 2;

  std::string indent() const { return std::string(indent_level * 2, ' '); }

  std::string GetScopePath(const Scope* target_scope) {
    if (!target_scope || !local_scope) return "";
    bool is_local = false;
    for (const Scope* s = local_scope; s && s != req_scope; s = s->parent) {
      if (s == target_scope) {
        is_local = true;
        break;
      }
    }
    if (is_local) {
      return "_scope" + std::to_string(target_scope->id);
    }
    if (!req_scope) return "_scope" + std::to_string(target_scope->id);
    std::string path = "_scope" + std::to_string(req_scope->id);
    for (const Scope* s = req_scope; s && s != target_scope; s = s->parent) {
      path += ".parent";
    }
    return path;
  }

  void Compile(const syntax::Expr& expr) {
    const syntax::Expr* old_expr = current_expr;
    current_expr = &expr;
    std::visit(*this, expr);
    current_expr = old_expr;
  }
  void operator()(const syntax::StringConstant& expr) {
    out << '"' << expr.value << '"';
  }
  void operator()(const syntax::IntegerConstant& expr) { out << expr; }
  void operator()(const syntax::Nil&) { out << "null"; }
  void operator()(const std::unique_ptr<syntax::LValue>& expr) {
    (*this)(*expr);
  }
  void operator()(const syntax::LValue& expr) { std::visit(*this, expr); }

  void operator()(const syntax::Identifier& expr) {
    const Scope* def_scope =
        current_expr ? symbols.getDefiningScope(*current_expr, expr) : nullptr;
    if (def_scope && local_scope) {
      out << GetScopePath(def_scope) << "." << Sanitize(expr);
    } else {
      out << Sanitize(expr);
    }
  }
  void operator()(const syntax::RecordField& expr) {
    (*this)(*expr.l_value);
    out << "." << expr.id;
  }
  void operator()(const syntax::ArrayElement& expr) {
    (*this)(*expr.l_value);
    out << "[";
    Compile(*expr.expr);
    out << "]";
  }
  void operator()(const syntax::Negated& expr) {
    out << "-";
    Compile(*expr.expr);
  }
  void operator()(const syntax::Binary& expr) {
    std::string op;
    switch (expr.op) {
      case kPlus:
        op = "+";
        break;
      case kMinus:
        op = "-";
        break;
      case kTimes:
        op = "*";
        break;
      case kDivide:
        op = "/";
        break;
      case kEqual:
        op = "==";
        break;
      case kUnequal:
        op = "!=";
        break;
      case kLessThan:
        op = "<";
        break;
      case kNotGreaterThan:
        op = "<=";
        break;
      case kGreaterThan:
        op = ">";
        break;
      case kNotLessThan:
        op = ">=";
        break;
      case kAnd:
        op = "&&";
        break;
      case kOr:
        op = "||";
        break;
      default:
        break;
    }
    Compile(*expr.left);
    out << " " << op << " ";
    Compile(*expr.right);
  }
  void operator()(const syntax::Assignment& expr) {
    std::ostringstream lval_out;
    Compiler lval_compiler{symbols, types, lval_out, post_body, "", local_scope,
        current_expr, req_scope, indent_level};
    lval_compiler(*expr.l_value);
    current_lvalue = lval_out.str();

    out << current_lvalue << " = ";
    Compile(*expr.expr);
    current_lvalue = "";
  }
  void operator()(const syntax::FunctionCall& expr) {
    const syntax::FunctionDeclaration* fn =
        current_expr ? symbols.lookupFunction(*current_expr, expr.id) : nullptr;

    std::string printFn = Sanitize(expr.id);
    if (printFn == "print") printFn = "System.out.print";
    if (printFn == "flush") printFn = "System.out.flush";

    out << printFn << "(";
    const char* sep = "";

    if (fn) {
      const Scope* fn_scope = fn->body ? symbols.getScope(*fn->body) : nullptr;
      const Scope* fn_req_scope = fn_scope ? fn_scope->parent : nullptr;
      if (fn_req_scope && local_scope) {
        out << GetScopePath(fn_req_scope);
        sep = ", ";
      }
    }

    for (auto& arg : expr.arguments) {
      out << sep;
      Compile(*arg);
      sep = ", ";
    }
    out << ")";
  }
  void operator()(const syntax::RecordLiteral& expr) {
    out << "new " << Sanitize(expr.type_id) << "()";
  }
  void operator()(const syntax::ArrayLiteral& expr) {
    out << "new " << GetJavaType(types, symbols, *expr.value) << "[";
    Compile(*expr.size);
    out << "];\n";
    if (current_lvalue.empty()) return;
    out << indent() << "Arrays.fill(" << current_lvalue << ", ";
    Compile(*expr.value);
    out << ")";
  }
  void operator()(const syntax::IfThen& expr) {
    out << "if (";
    Compile(*expr.condition);
    out << ") {\n";
    indent_level++;
    out << indent();
    Compile(*expr.then_expr);
    if (NeedsSemicolon(types, *expr.then_expr)) out << ";";
    indent_level--;
    out << "\n" << indent() << "}";
  }
  void operator()(const syntax::IfThenElse& expr) {
    std::string_view type = types(*expr.then_expr);
    if (type.empty() || type == "void" || type == "Object" ||
        type == "NOTYPE") {
      out << "if (";
      Compile(*expr.condition);
      out << ") {\n";
      indent_level++;
      out << indent();
      Compile(*expr.then_expr);
      if (NeedsSemicolon(types, *expr.then_expr)) out << ";";
      indent_level--;
      out << "\n" << indent() << "} else {\n";
      indent_level++;
      out << indent();
      Compile(*expr.else_expr);
      if (NeedsSemicolon(types, *expr.else_expr)) out << ";";
      indent_level--;
      out << "\n" << indent() << "}";
    } else {
      out << "(";
      Compile(*expr.condition);
      out << " ? ";
      Compile(*expr.then_expr);
      out << " : ";
      Compile(*expr.else_expr);
      out << ")";
    }
  }
  void operator()(const syntax::While& expr) {
    out << "while (";
    Compile(*expr.condition);
    out << ") {\n";
    indent_level++;
    out << indent();
    Compile(*expr.body);
    if (NeedsSemicolon(types, *expr.body)) out << ";";
    indent_level--;
    out << "\n" << indent() << "}";
  }
  void operator()(const syntax::For& expr) {
    out << "for (int " << Sanitize(expr.id) << " = ";
    Compile(*expr.start);
    out << "; " << Sanitize(expr.id) << " <= ";
    Compile(*expr.end);
    out << "; " << Sanitize(expr.id) << "++) {\n";
    indent_level++;
    out << indent();
    Compile(*expr.body);
    if (NeedsSemicolon(types, *expr.body)) out << ";";
    indent_level--;
    out << "\n" << indent() << "}";
  }
  void operator()(const syntax::Break&) { out << "break"; }
  void operator()(const syntax::FunctionDeclaration& expr) {
    const Scope* fn_scope = expr.body ? symbols.getScope(*expr.body) : nullptr;
    const Scope* req_scope = fn_scope ? fn_scope->parent : nullptr;

    std::ostringstream fn_out;
    fn_out << "\n"
           << indent() << "static "
           << (expr.type_id ? GetJavaType(
                                  types, symbols, *current_expr, *expr.type_id)
                            : "void")
           << " " << Sanitize(expr.id) << "(";
    const char* sep = "";
    if (req_scope) {
      fn_out << "Scope" << req_scope->id << " _scope" << req_scope->id;
      sep = ", ";
    }
    for (const auto& arg : expr.parameter) {
      fn_out << sep << GetJavaType(types, arg.type_id) << " "
             << Sanitize(arg.id);
      sep = ", ";
    }
    fn_out << ") {\n";
    Compiler sub_compiler{symbols, types, fn_out, post_body, "", fn_scope,
        current_expr, req_scope, indent_level + 1};
    if (fn_scope) {
      fn_out << sub_compiler.indent() << "Scope" << fn_scope->id << " _scope"
             << fn_scope->id << " = new Scope" << fn_scope->id << "();\n";
      if (req_scope) {
        fn_out << sub_compiler.indent() << "_scope" << fn_scope->id
               << ".parent = _scope" << req_scope->id << ";\n";
      }
      for (const auto& arg : expr.parameter) {
        fn_out << sub_compiler.indent() << "_scope" << fn_scope->id << "."
               << Sanitize(arg.id) << " = " << Sanitize(arg.id) << ";\n";
      }
    }

    if (expr.body) {
      fn_out << sub_compiler.indent();
      sub_compiler.Compile(*expr.body);
      if (NeedsSemicolon(types, *expr.body)) fn_out << ";";
    }
    fn_out << "\n" << indent() << "}\n";
    post_body << fn_out.str();
  }
  void operator()(const syntax::VariableDeclaration&) {}
  void operator()(const syntax::TypeDeclaration&) {}
  void operator()(const syntax::ArrayType&) {}
  void operator()(const syntax::TypeFields& expr) {
    for (auto& field : expr) {
      out << Sanitize(field.type_id) << " " << Sanitize(field.id) << ";\n";
    }
  }
  void operator()(const syntax::Let& expr) {
    const Scope* let_scope =
        expr.body.empty() ? nullptr : symbols.getScope(*expr.body[0]);
    if (let_scope) {
      out << "{\n";
      indent_level++;
      out << indent() << "Scope" << let_scope->id << " _scope" << let_scope->id
          << " = new Scope" << let_scope->id << "();\n";
      if (let_scope->parent && local_scope) {
        out << indent() << "_scope" << let_scope->id << ".parent = _scope"
            << local_scope->id << ";\n";
      }
      Compiler sub_compiler{symbols, types, out, post_body, "", let_scope,
          current_expr, req_scope, indent_level};

      for (const auto& decl : expr.declaration) {
        std::visit(Overloaded{[&](const syntax::FunctionDeclaration& fn) {
          sub_compiler(fn);
        },
                       [&](const syntax::TypeDeclaration& type) {
          if (auto fields = std::get_if<syntax::TypeFields>(&type.value)) {
            post_body << "class " << Sanitize(type.id) << " {\n";
            for (auto& field : *fields) {
              post_body << "  public " << GetJavaType(types, field.type_id)
                        << " " << Sanitize(field.id) << ";\n";
            }
            post_body << "}\n\n";
          }
        },
                       [&](const syntax::VariableDeclaration& var) {
          std::string lval =
              "_scope" + std::to_string(let_scope->id) + "." + Sanitize(var.id);
          out << indent() << lval << " = ";
          sub_compiler.current_lvalue = lval;
          if (var.value) sub_compiler.Compile(*var.value);
          sub_compiler.current_lvalue = "";
          out << ";\n";
        }},
            *decl);
      }

      bool first = true;
      for (const auto& stmt : expr.body) {
        if (!first) {
          out << "\n" << indent();
        } else {
          out << indent();
        }
        first = false;
        sub_compiler.Compile(*stmt);
        if (NeedsSemicolon(types, *stmt)) {
          out << ";";
        }
      }
      indent_level--;
      out << "\n" << indent() << "}";
    }
  }
  void operator()(const syntax::Parenthesized& expr) {
    bool first = true;
    for (auto& e : expr.exprs) {
      if (!first) {
        out << "\n" << indent();
      }
      first = false;
      Compile(*e);
      if (NeedsSemicolon(types, *e)) {
        out << ";";
      }
    }
  }
};

std::string Compile(const syntax::Expr& expr, const SymbolTable& t,
    TypeFinder& tf, std::string_view class_name) {
  std::ostringstream head;
  std::ostringstream body;
  std::ostringstream post_body;

  head << "import java.util.Arrays;\n\n";

  PrintScopes(t, tf, head);

  body << "class " << class_name << " {\n\n";
  body << "  public static void main(String[] args) {\n";

  const Scope* main_scope = t.scopes().empty() ? nullptr : t.scopes()[0].get();
  Compiler compile{t, tf, body, post_body, "", main_scope, nullptr, nullptr, 2};
  if (main_scope) {
    body << compile.indent() << "Scope" << main_scope->id << " _scope"
         << main_scope->id << " = new Scope" << main_scope->id << "();\n";
  }

  body << compile.indent();
  compile.Compile(expr);

  body << "\n  }\n";

  return head.str() + body.str() + post_body.str() + "}\n";
}

}  // namespace java
