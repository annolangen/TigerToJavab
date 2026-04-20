#include "java_source.h"

#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <variant>
#include <vector>

#include "debug_string.h"

namespace java {

std::string Sanitize(std::string id) {
  static std::unordered_set<std::string_view> kJavaKeywords = {
      "abstract",   "assert",       "boolean",   "break",      "byte",
      "case",       "catch",        "char",      "class",      "const",
      "continue",   "default",      "do",        "double",     "else",
      "enum",       "extends",      "final",     "finally",    "float",
      "for",        "goto",         "if",        "implements", "import",
      "instanceof", "int",          "interface", "long",       "native",
      "new",        "package",      "private",   "protected",  "public",
      "return",     "short",        "static",    "strictfp",   "super",
      "switch",     "synchronized", "this",      "throw",      "throws",
      "transient",  "try",          "void",      "volatile",   "while",
  };
  return kJavaKeywords.count(id) ? "_" + id : id;
}

bool IsClassScope(const Scope* s) {
  if (!s) return false;
  for (const auto& kv : s->storage) {
    if (std::holds_alternative<const syntax::VariableDeclaration*>(kv.second))
      return true;
  }
  return false;
}

struct JavaType {
  const SymbolTable& symbols;
  const syntax::Expr& expr;

  std::string operator()(std::string_view name) {
    if (const syntax::TypeDeclaration* type = symbols.lookupType(expr, name)) {
      return std::visit(*this, type->value);
    }
    static const std::unordered_map<std::string_view, std::string> kTypeMap = {
        {"int", "int"}, {"string", "String"}, {"nil", "null"}};
    auto it = kTypeMap.find(name);
    return it != kTypeMap.end() ? it->second : "Object";
  }
  std::string operator()(const syntax::TypeFields& fields) {
    std::ostringstream out;
    out << "class {\n";
    for (const auto& field : fields) {
      out << "  " << (*this)(field.type_id) << " " << field.id << ";\n";
    }
    out << "}";
    return out.str();
  }
  std::string operator()(const syntax::ArrayType& array) {
    return (*this)(array.element_type_id) + "[]";
  }
};

struct Compiler {
  const SymbolTable& symbols;
  TypeFinder& types;
  std::ostream& out;
  std::ostream& post_body;
  std::string current_lvalue;

  const syntax::FunctionDeclaration* current_function = nullptr;
  const Scope* local_scope = nullptr;
  const Scope* constructor_scope = nullptr;
  const syntax::Expr* current_expr = nullptr;

  void operator()(const syntax::Expr& expr) {
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
    if (constructor_scope && constructor_scope == def_scope) {
      out << Sanitize(expr);
      return;
    }
    if (IsClassScope(def_scope)) {
      out << "_scope." << Sanitize(expr);
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
    (*this)(*expr.expr);
    out << "]";
  }
  void operator()(const syntax::Negated& expr) {
    out << "-";
    (*this)(*expr.expr);
  }
  void operator()(const syntax::Binary& expr) {
    (*this)(*expr.left);
    out << " " << expr.op << " ";
    (*this)(*expr.right);
  }
  void operator()(const syntax::Assignment& expr) {
    std::ostringstream lval_out;
    Compiler lval_compiler{
        symbols,          types,       lval_out,         post_body, "",
        current_function, local_scope, constructor_scope};
    lval_compiler(*expr.l_value);
    current_lvalue = lval_out.str();

    out << current_lvalue;
    out << " = ";
    (*this)(*expr.expr);
    current_lvalue = "";
  }
  void operator()(const syntax::FunctionCall& expr) {
    const syntax::FunctionDeclaration* fn =
        current_expr ? symbols.lookupFunction(*current_expr, expr.id) : nullptr;
    if (fn) out << "fn";
    std::string printFn = Sanitize(expr.id);
    if (printFn == "print") printFn = "System.out.print";
    out << printFn << "(";
    const char* sep = "";

    // standard library functions don't have scopes
    if (fn) {
      const Scope* fn_scope = fn->body ? symbols.getScope(*fn->body) : nullptr;
      const Scope* req_scope = fn_scope ? fn_scope->parent : nullptr;
      if (IsClassScope(req_scope)) {
        if (!current_function) {
          out << "new Scope" << req_scope->id << "()";
        } else {
          out << "_scope";
        }
        sep = ", ";
      }
    }

    for (auto& arg : expr.arguments) {
      out << sep;
      (*this)(*arg);
      sep = ", ";
    }
    out << ")";
  }
  void operator()(const syntax::RecordLiteral& expr) {
    out << "new " << Sanitize(expr.type_id) << "()";
  }
  void operator()(const syntax::ArrayLiteral& expr) {
    JavaType java_type = {symbols, *expr.value};
    out << "new " << java_type(types(*expr.value)) << "[";
    (*this)(*expr.size);
    out << "];\n";
    if (current_lvalue.empty()) {
      std::cerr << "l_value is null\n";
      return;
    }
    out << "    Arrays.fill(";
    out << current_lvalue;
    out << ", ";
    (*this)(*expr.value);
    out << ")";
  }
  void operator()(const syntax::IfThen& expr) {
    out << "if (";
    (*this)(*expr.condition);
    out << ") {\n      ";
    (*this)(*expr.then_expr);
    out << ";\n    }";
  }
  void operator()(const syntax::IfThenElse& expr) {
    std::string_view type = types(*expr.then_expr);
    if (type.empty() || type == "void" || type == "Object") {
      out << "if (";
      (*this)(*expr.condition);
      out << ") {\n      ";
      (*this)(*expr.then_expr);
      out << ";\n    } else {\n      ";
      (*this)(*expr.else_expr);
      out << ";\n    }";
    } else {
      (*this)(*expr.condition);
      out << " ? ";
      (*this)(*expr.then_expr);
      out << " : ";
      (*this)(*expr.else_expr);
    }
  }
  void operator()(const syntax::While& expr) {
    out << "while (";
    (*this)(*expr.condition);
    out << ") {\n";
    (*this)(*expr.body);
    out << ";\n}";
  }
  void operator()(const syntax::For& expr) {
    out << "for (int " << expr.id << " = ";
    (*this)(*expr.start);
    out << "; " << expr.id << " <= ";
    (*this)(*expr.end);
    out << "; " << expr.id << "++) {\n      ";
    (*this)(*expr.body);
    out << ";\n    }";
  }
  void operator()(const syntax::Break&) { out << "break"; }
  void operator()(const syntax::FunctionDeclaration& expr) {
    const Scope* fn_scope = expr.body ? symbols.getScope(*expr.body) : nullptr;
    const Scope* req_scope = fn_scope ? fn_scope->parent : nullptr;

    std::ostringstream fn_out;
    fn_out << "\n  static " << expr.type_id.value_or("void") << " "
           << Sanitize(expr.id) << "(";
    const char* sep = "";
    if (IsClassScope(req_scope)) {
      fn_out << "Scope" << req_scope->id << " _scope";
      sep = ", ";
    }
    for (const auto& arg : expr.parameter) {
      fn_out << sep << "int" /* TODO types(arg) */ << " " << Sanitize(arg.id);
      sep = ", ";
    }
    fn_out << ") {\n    ";
    Compiler sub_compiler{symbols, types,   fn_out,  post_body, "",
                          &expr,   nullptr, nullptr, nullptr};
    // if body is let, it prints statements
    sub_compiler(*expr.body);
    // Don't add a concluding semicolon if it's a Let body or IfThen block that
    // already does it Wait, let's just not add semicolon because it might break
    // block parsing.
    fn_out << "\n  }\n";
    post_body << fn_out.str();
  }
  void operator()(const syntax::VariableDeclaration&) {}
  void operator()(const syntax::TypeDeclaration&) {}
  void operator()(const syntax::ArrayType&) { out << "TODO ArrayType"; }
  void operator()(const syntax::TypeFields& expr) {
    for (auto& field : expr) {
      out << field.type_id << " " << field.id << ";\n";
    }
  }
  void operator()(const syntax::Let& expr) {
    for (const auto& decl : expr.declaration) {
      if (auto fn = std::get_if<syntax::FunctionDeclaration>(decl.get())) {
        (*this)(*fn);
      }
    }
    const char* sep = "";
    for (const auto& stmt : expr.body) {
      out << sep;
      (*this)(*stmt);
      if (std::holds_alternative<syntax::IfThen>(*stmt) ||
          std::holds_alternative<syntax::IfThenElse>(*stmt)) {
        // blocks don't need semicolons
      } else {
        out << ";\n    ";
      }
    }
  }
  void operator()(const syntax::Parenthesized& expr) {
    const char* sep = "";
    for (auto& e : expr.exprs) {
      out << sep;
      (*this)(*e);
      sep = ";\n      ";
    }
  }
};

std::string Compile(const syntax::Expr& expr, const SymbolTable& t,
                    TypeFinder& tf, std::string_view class_name) {
  std::ostringstream head;
  std::ostringstream body;
  std::ostringstream post_body;

  head << "import java.util.Arrays;\n\n";

  body << "class " << class_name << " {\n\n";
  body << "  public static void main(String[] args) {\n    ";

  Compiler compile{t,       tf,      body,    post_body, "",
                   nullptr, nullptr, nullptr, nullptr};
  compile(expr);

  body << "\n  }\n";

  return head.str() + body.str() + post_body.str() + "}\n";
}

} // namespace java
