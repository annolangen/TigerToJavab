#include "java_source.h"

#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <variant>
#include <vector>

#include "debug_string.h"

namespace java {
namespace {

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

struct Compiler;

struct JavaFunction {
  const syntax::FunctionDeclaration* decl;
  std::vector<syntax::VariableDeclaration*> outer_scope;

  std::ostream& AppendDefinition(std::ostream& os, TypeFinder& types,
                                 Compiler& compiler);
};

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
    // TODO: Need class name. maybe from outer alias.
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
  std::ostringstream& out;
  std::ostringstream& post_body;
  // The current l-value being processed. Used for Array.fill() .
  syntax::LValue* l_value = nullptr;

  void operator()(const syntax::Expr& expr) { std::visit(*this, expr); }
  void operator()(const syntax::StringConstant& expr) {
    out << '"' << expr.value << '"';
  }
  void operator()(const syntax::IntegerConstant& expr) { out << expr; }
  void operator()(const syntax::Nil& expr) { out << "null"; }
  void operator()(const std::unique_ptr<syntax::LValue>& expr) {
    (*this)(*expr);
  }
  void operator()(const syntax::LValue& expr) { std::visit(*this, expr); }
  void operator()(const syntax::Identifier& expr) { out << expr; }
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
    (*this)(*expr.l_value);
    l_value = expr.l_value.get();
    out << " = ";
    (*this)(*expr.expr);
    l_value = nullptr;
  }
  void operator()(const syntax::FunctionCall& expr) {
    out << Sanitize(expr.id) << ".run(";
    const char* sep = "";
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
    if (l_value == nullptr) {
      std::cerr << "l_value is null\n";
      return;
    }
    out << "Arrays.fill(";
    (*this)(*l_value);
    out << ", ";
    (*this)(*expr.value);
    out << ")";
  }
  void operator()(const syntax::IfThen& expr) {
    out << "if (";
    (*this)(*expr.condition);
    out << ") {\n";
    (*this)(*expr.then_expr);
    out << "\n}";
  }
  void operator()(const syntax::IfThenElse& expr) {
    out << "if (";
    (*this)(*expr.condition);
    out << ") {\n";
    (*this)(*expr.then_expr);
    out << "\n} else {\n";
    (*this)(*expr.else_expr);
    out << "\n}";
  }
  void operator()(const syntax::While& expr) {
    out << "while (";
    (*this)(*expr.condition);
    out << ") {\n";
    (*this)(*expr.body);
    out << "\n}";
  }
  void operator()(const syntax::For& expr) {
    out << "for (int " << expr.id << " = ";
    (*this)(*expr.start);
    out << "; " << expr.id << " < ";
    (*this)(*expr.end);
    out << "; " << expr.id << "++) {\n";
    (*this)(*expr.body);
    out << "\n}";
  }
  void operator()(const syntax::Break& expr) { out << "break"; }
  void operator()(const syntax::FunctionDeclaration& expr) {
    JavaFunction f{&expr};
    f.AppendDefinition(out, types, *this);
  }
  void operator()(const syntax::VariableDeclaration& decl) {
    if (decl.value) {
      JavaType java_type = {symbols, *decl.value};
      out << java_type(types(decl)) << " " << decl.id;
      out << " = ";
      syntax::LValue id = decl.id;
      auto* old_lvalue = l_value;
      l_value = &id;
      (*this)(*decl.value);
      l_value = old_lvalue;
    }
    out << ";\n";
  }
  void operator()(const syntax::TypeDeclaration& expr) {}
  void operator()(const syntax::ArrayType& expr) { out << "TODO ArrayType"; }
  void operator()(const syntax::TypeFields& expr) {
    for (auto& field : expr) {
      out << field.type_id << " " << field.id << ";\n";
    }
  }
  void operator()(const syntax::Let& expr) {
    for (auto& decl : expr.declaration) {
      std::visit(*this, *decl);
    }
    for (auto& stmt : expr.body) {
      (*this)(*stmt);
      out << ";\n";
    }
  }
  void operator()(const syntax::Parenthesized& expr) {
    const char* sep = "\n";
    for (auto& e : expr.exprs) {
      out << sep;
      (*this)(*e);
      sep = ";\n";
    }
  }
};

std::ostream& JavaFunction::AppendDefinition(std::ostream& os,
                                             TypeFinder& types,
                                             Compiler& compiler) {
  os << "  static " << decl->type_id.value_or("void") << " " << decl->id << "(";
  const char* sep = "";
  for (const auto& arg : decl->parameter) {
    os << sep << arg.type_id << " " << arg.id;
    sep = ", ";
  }
  for (const auto* arg : outer_scope) {
    os << sep << types(*arg) << " " << arg->id;
    sep = ", ";
  }
  os << ") {\n";
  compiler(*decl->body);
  return os << "\n  }\n";
}
}  // namespace

std::string Compile(const syntax::Expr& expr, const SymbolTable& t,
                    TypeFinder& tf, std::string_view class_name) {
  std::ostringstream body;
  std::ostringstream post_body;
  body << "import java.utils.Arrays;"
       << "\n\n";
  body << "class " << class_name << " {\n\n";
  body << "  public static void main(String[] args) {\n";
  post_body << "}\n}\n";

  Compiler compile{t, tf, body, post_body};
  compile(expr);

  return body.str() + post_body.str();
}
}  // namespace java
