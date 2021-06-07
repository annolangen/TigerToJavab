#pragma once
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace syntax {
class Expr;   // forward declaration
class LValue; // forward declaration
using Identifier = std::string;
struct RecordField {
  std::unique_ptr<LValue> l_value;
  std::string id;
};
struct ArrayElement {
  std::unique_pre<LValue> l_value;
  std::unique_ptr<Expr> expr;
};
using StringConstant = std::string;
using IntegerConstant = int;
struct Nil {};
using LValue = std::variant<Identifier, RecordField, ArrayElement>;
using Negated = Expr;
struct Binary {
  std::unique_ptr<Expr> left;
  Token op;
  std::unique_ptr<Expr> right;
};
struct Assignment {
  std::unique_ptr<LValue> l_value;
  std::unique_ptr<Expr> expr;
};
using Parenthesized = std::vector<std::unique_ptr<Expr>>;
struct FunctionCall {
  std::string id;
  Parenthesized arguments;
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
  std::unique_ptr<Expr> then;
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
struct Declaration {
  std::string id;
  std::unique_ptr<Expr> value;
};
struct Let {
  std::vector<Declaration> declarations;
  std::unique_ptr<Expr> body;
};
using Expr =
    std::variant<StringConstant, IntegerConstant, Nil, LValue, Negated, Binary,
                 Assignment, FunctionCall, Parenthesized, RecordLiteral,
                 ArrayLiteral, IfThen, IfThenElse, While, For, Break, Let>;

// Returns Expression parsed using yylex and yytext.
std::unique_ptr<Expr> Parse();
} // namespace syntax
