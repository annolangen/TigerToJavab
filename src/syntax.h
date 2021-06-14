#pragma once
#include "Token.h"
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace syntax {
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

using StringConstant = std::string;
using IntegerConstant = int;
struct Nil {};
using Expr =
    std::variant<StringConstant, IntegerConstant, Nil, LValue, Negated, Binary,
                 Assignment, FunctionCall, RecordLiteral, ArrayLiteral, IfThen,
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
  Token op;
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
  std::vector<Declaration> declaration;
  std::vector<std::unique_ptr<Expr>> body;
};

// Returns Expression parsed using yylex and yytext.
std::unique_ptr<Expr> Parse();
} // namespace syntax
