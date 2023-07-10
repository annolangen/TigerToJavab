#include "java_source.h"

#include <sstream>
#include <string>
#include <unordered_set>
#include <variant>
#include <vector>

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
  return kJavaKeywords.count(id) ? "__" + id + "__" : id;
}

struct ClassBuilder {
  std::string name;
  std::ostringstream fields;
  std::ostringstream constructor;
  std::string run_method_type;
  std:string run_method_name;
  std::vector<std::string> run_method_args;
  std::ostringstream run_method_body;

  std::ostream& operator<<(std::ostream& os) {
    os << "class " << name << " {\n";
    os << fields.str();
    os << "  public " << name << "() {\n";
    os << constructor.str();
    os << "  }\n";
    os << "  " << run_method_type << " "<<run_method_name<<"(";
    const char* sep = "";
    for (auto& arg : run_method_args) {
      os << sep << arg;
      sep = ", ";
    }
    os << ") {\n";
    os << run_method_body.str();
    os << "  }\n";
    os << "}\n";
    return os;
  }
};

struct Compiler {
  const SymbolTable& symbols;
  TypeFinder& types;
  std::ostringstream& body;
  int next_id = 0;

  void operator()(const syntax::Expr& expr) { std::visit(*this, expr); }
  void operator()(const syntax::StringConstant& expr) {
    body << '"' << expr.value << '"';
  }
  void operator()(const syntax::IntegerConstant& expr) { body << expr; }
  void operator()(const syntax::Nil& expr) { body << "null"; }
  void operator()(const std::unique_ptr<syntax::LValue>& expr) {
    (*this)(*expr);
  }
  void operator()(const syntax::LValue& expr) { std::visit(*this, expr); }
  void operator()(const syntax::Identifier& expr) { body << expr; }
  void operator()(const syntax::RecordField& expr) {
    (*this)(*expr.l_value);
    body << "." << expr.id;
  }
  void operator()(const syntax::ArrayElement& expr) {
    (*this)(*expr.l_value);
    body << "[";
    (*this)(*expr.expr);
    body << "]";
  }
  void operator()(const syntax::Negated& expr) {
    body << "-";
    (*this)(*expr.expr);
  }
  void operator()(const syntax::Binary& expr) {
    (*this)(*expr.left);
    body << " " << expr.op << " ";
    (*this)(*expr.right);
  }
  void operator()(const syntax::Assignment& expr) {
    (*this)(*expr.l_value);
    body << " = ";
    (*this)(*expr.expr);
  }
  void operator()(const syntax::FunctionCall& expr) {
    body << Sanitize(expr.id) << ".run(";
    const char* sep = "";
    for (auto& arg : expr.arguments) {
      body << sep;
      (*this)(*arg);
      sep = ", ";
    }
    body << ")";
  }
  void operator()(const syntax::Parenthesized& expr) {
    const char* sep = "\n";
    for (auto& e : expr.exprs) {
      body << sep;
      (*this)(*e);
      sep = ";\n";
    }
  }
};

}  // namespace

std::string Compile(const syntax::Expr& expr, const SymbolTable& t,
                    TypeFinder& tf, std::string class_name) {
  std::ostringstream prelude;
  std::ostringstream decls;
  std::ostringstream body;
  std::ostringstream postlude;
  prelude << "import java.utils.Arrays;"
          << "\n\n";
  prelude << "class " << class_name << " {\n";
  prelude << "static void print(String s) {\n  System.out.print(s);\n}\n";
  prelude << "static void print(int i) {\n  System.out.print(i);\n}\n";
  body << "  void run() {\n";
  postlude << "  }\n";
  postlude << "  public static void main(String[] args) {\n    new "
           << class_name << "().run();\n  }\n";
  postlude << "}\n";

  Compiler compile{t, tf, decls, body};
  compile(expr);

  return prelude.str() + decls.str() + body.str() + postlude.str();
}
}  // namespace java