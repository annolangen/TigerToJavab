#pragma once
#include "BinaryOp.h"
#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// Pairs of classes to hold the abstract syntax tree and navigating
// them. Pure virtual classes Type, Declaration, and Expression each
// accept corresponding visitors, TypeVisitor, DeclarationVisitor, and
// ExpressionVisitor. Default methods in visitor classes facilitate
// navigation by calling Accept recursively. Methods Accept and Visit
// return boolean `false` to terminate navigation.

struct TypeField {
  std::string id;
  std::string type_id;
};

class TypeVisitor;

class Type {
public:
  virtual ~Type() = default;
  virtual bool Accept(TypeVisitor &visitor) const = 0;
};

class TypeVisitor {
public:
  virtual bool VisitTypeReference(const std::string &id) { return true; }
  virtual bool VisitRecordType(const std::vector<TypeField> fields) {
    return true;
  }
  virtual bool VisitArrayType(const std::string &type_id) { return true; }
};

// Reference to a named type.
class TypeReference : public Type {
public:
  TypeReference(std::string_view id) : id_(id) {}
  virtual bool Accept(TypeVisitor &visitor) const {
    return visitor.VisitTypeReference(id_);
  }

private:
  std::string id_;
};

class RecordType : public Type {
public:
  RecordType(std::vector<TypeField> &&fields) : fields_(std::move(fields)) {}
  virtual bool Accept(TypeVisitor &visitor) const {
    return visitor.VisitRecordType(fields_);
  }

private:
  std::vector<TypeField> fields_;
};

class ArrayType : public Type {
public:
  ArrayType(std::string_view type_id) : type_id_(type_id) {}
  virtual bool Accept(TypeVisitor &visitor) {
    return visitor.VisitArrayType(type_id_);
  }

private:
  std::string type_id_;
};

// Forward declaration of visitor to navigate Declaration abstract syntax tree.
class DeclarationVisitor;

// Abstract base class for Declarations
class Declaration {
public:
  virtual ~Declaration() = 0;
  virtual bool Accept(DeclarationVisitor &visitor) const = 0;
};

class Expression;

class DeclarationVisitor {
public:
  virtual bool VisitTypeDeclaration(const std::string &id, const Type &type) {
    return true;
  }
  virtual bool
  VisitVariableDeclaration(const std::string &id,
                           const std::optional<std::string> &type_id,
                           const Expression &expr) {
    return true;
  }
  virtual bool VisitFunctionDeclaration(
      const std::string &id, const std::vector<TypeField> &params,
      const std::optional<std::string> type_id, const Expression &body) {
    return true;
  }
};

class TypeDeclaration : public Declaration {
public:
  TypeDeclaration(std::string_view type_id, std::shared_ptr<Type> type)
      : type_id_(type_id), type_(type) {}
  virtual bool Accept(DeclarationVisitor &visitor) const {
    return visitor.VisitTypeDeclaration(type_id_, *type_);
  }

private:
  std::string type_id_;
  std::shared_ptr<Type> type_;
};

class VariableDeclaration : public Declaration {
public:
  VariableDeclaration(std::string_view id, std::shared_ptr<Expression> expr)
      : id_(id), expr_(expr) {}
  VariableDeclaration(std::string_view id, std::string_view type_id,
                      std::shared_ptr<Expression> expr)
      : id_(id), type_id_(type_id), expr_(expr) {}
  virtual bool Accept(DeclarationVisitor &visitor) const {
    return visitor.VisitVariableDeclaration(id_, type_id_, *expr_);
  }

private:
  std::string id_;
  std::optional<std::string> type_id_;
  std::shared_ptr<Expression> expr_;
};

class FunctionDeclaration : public Declaration {
public:
  FunctionDeclaration(std::string_view id, std::vector<TypeField> &&params,
                      std::shared_ptr<Expression> body)
      : id_(id), params_(std::move(params)), body_(body) {}
  FunctionDeclaration(std::string_view id, std::vector<TypeField> &&params,
                      std::string_view type_id,
                      std::shared_ptr<Expression> body)
      : id_(id), type_id_(type_id), params_(std::move(params)), body_(body) {}
  virtual bool Accept(DeclarationVisitor &visitor) const {
    return visitor.VisitFunctionDeclaration(id_, params_, type_id_, *body_);
  }

private:
  std::string id_;
  std::vector<TypeField> params_;
  std::optional<std::string> type_id_;
  std::shared_ptr<Expression> body_;
};

// Forward declaration of visitor to navigate abstract syntax tree Expression.
class ExpressionVisitor;

// Base class for Abstract Syntax Tree. Original source is
// http://www.cs.columbia.edu/~sedwards/classes/2002/w4115/tiger.pdf
class Expression {
public:
  virtual ~Expression() = default;
  virtual bool Accept(ExpressionVisitor &visitor) const = 0;
};

struct FieldValue {
  std::string id;
  std::unique_ptr<Expression> expr;
};

class LValue;

// Visit methods return false to stop. Default implementations return true after
// successfully visiting all child Expressions.
class ExpressionVisitor {
public:
  virtual bool VisitStringConstant(const std::string &text) { return true; }
  virtual bool VisitIntegerConstant(int value) { return true; }
  virtual bool VisitNil() { return true; }
  virtual bool VisitLValue(const LValue &value) { return true; }
  virtual bool VisitNegated(const Expression &value) {
    return value.Accept(*this);
  }
  virtual bool VisitBinary(const Expression &left, BinaryOp op,
                           const Expression &right) {
    return left.Accept(*this) && right.Accept(*this);
  }
  virtual bool VisitAssignment(const LValue &value, const Expression &expr) {
    return expr.Accept(*this);
  }
  virtual bool
  VisitFunctionCall(const std::string &id,
                    const std::vector<std::shared_ptr<Expression>> &args) {
    return std::all_of(args.begin(), args.end(),
                       [this](const auto &arg) { return arg->Accept(*this); });
  }
  virtual bool
  VisitBlock(const std::vector<std::shared_ptr<Expression>> &exprs) {
    return std::all_of(exprs.begin(), exprs.end(),
                       [this](const auto &arg) { return arg->Accept(*this); });
  }
  virtual bool VisitRecord(const std::string &type_id,
                           const std::vector<FieldValue> &field_values) {
    return true;
  }
  virtual bool VisitArray(const std::string &type_id, const Expression &size,
                          const Expression &value) {
    return size.Accept(*this) && value.Accept(*this);
  }
  virtual bool VisitIfThen(const Expression &condition,
                           const Expression &expr) {
    return condition.Accept(*this) && expr.Accept(*this);
  }
  virtual bool VisitIfThenElse(const Expression &condition,
                               const Expression &then_expr,
                               const Expression &else_expr) {
    return condition.Accept(*this) && then_expr.Accept(*this) &&
           else_expr.Accept(*this);
  }
  virtual bool VisitWhile(const Expression &condition, const Expression &body) {
    return condition.Accept(*this) && body.Accept(*this);
  }
  virtual bool VisitFor(const std::string &id, const Expression &first,
                        const Expression &last, const Expression &body) {
    return first.Accept(*this) && last.Accept(*this) && body.Accept(*this);
  }
  virtual bool VisitBreak() { return true; }
  virtual bool
  VisitLet(const std::vector<std::unique_ptr<Declaration>> &declarations) {
    return true;
  }
};

class StringConstant : public Expression {
public:
  StringConstant(std::string_view text) : text_(text) {}
  virtual bool Accept(ExpressionVisitor &visitor) const {
    return visitor.VisitStringConstant(text_);
  }

private:
  std::string text_;
};

class IntegerConstant : public Expression {
public:
  IntegerConstant(int value) : value_(value) {}
  virtual bool Accept(ExpressionVisitor &visitor) const {
    return visitor.VisitIntegerConstant(value_);
  }

private:
  int value_;
};

class Nil : public Expression {
public:
  Nil() {}
  virtual bool Accept(ExpressionVisitor &visitor) const {
    return visitor.VisitNil();
  }
};

class LValueVisitor;

class LValue : public Expression {
public:
  virtual bool Accept(ExpressionVisitor &visitor) const {
    return visitor.VisitLValue(*this);
  }
  virtual bool Accept(LValueVisitor &visitor) const = 0;
};

class LValueVisitor {
public:
  virtual bool VisitId(const std::string &id) { return true; }
  virtual bool VisitField(const LValue &value, const std::string &id) {
    return value.Accept(*this);
  }
  virtual bool VisitIndex(const LValue &value, const Expression &expr) {
    return value.Accept(*this);
  }
};

class IdLValue : public LValue {
public:
  IdLValue(std::string_view id) : id_(id) {}
  virtual bool Accept(LValueVisitor &visitor) const {
    return visitor.VisitId(id_);
  }

private:
  std::string id_;
};

class FieldLValue : public LValue {
public:
  FieldLValue(std::shared_ptr<LValue> value, std::string_view id)
      : value_(value), id_(id) {}
  virtual bool Accept(LValueVisitor &visitor) const {
    return visitor.VisitField(*value_, id_);
  }

private:
  std::shared_ptr<LValue> value_;
  std::string id_;
};

class IndexLValue : public LValue {
public:
  IndexLValue(std::shared_ptr<LValue> value, std::shared_ptr<Expression> expr)
      : value_(value), expr_(expr) {}
  virtual bool Accept(LValueVisitor &visitor) const {
    return visitor.VisitIndex(*value_, *expr_);
  }

private:
  std::shared_ptr<LValue> value_;
  std::shared_ptr<Expression> expr_;
};

class Negated : public Expression {
public:
  Negated(std::shared_ptr<Expression> expr) : expr_(expr) {}
  virtual bool Accept(ExpressionVisitor &visitor) const {
    return visitor.VisitNegated(*expr_);
  }

private:
  std::shared_ptr<Expression> expr_;
};

class Binary : public Expression {
public:
  Binary(std::shared_ptr<Expression> left, BinaryOp op,
         std::shared_ptr<Expression> right)
      : left_(left), op_(op), right_(right) {}
  virtual bool Accept(ExpressionVisitor &visitor) const {
    return visitor.VisitBinary(*left_, op_, *right_);
  }

private:
  std::shared_ptr<Expression> left_;
  BinaryOp op_;
  std::shared_ptr<Expression> right_;
};

class Assignment : public Expression {
public:
  Assignment(std::shared_ptr<LValue> value, std::shared_ptr<Expression> expr)
      : value_(value), expr_(expr) {}
  virtual bool Accept(ExpressionVisitor &visitor) const {
    return visitor.VisitAssignment(*value_, *expr_);
  }

private:
  std::shared_ptr<LValue> value_;
  std::shared_ptr<Expression> expr_;
};

class FunctionCall : public Expression {
  FunctionCall(std::string_view id,
               std::vector<std::shared_ptr<Expression>> &&args)
      : id_(id), args_(std::move(args)) {}
  virtual bool Accept(ExpressionVisitor &visitor) const {
    return visitor.VisitFunctionCall(id_, args_);
  }

private:
  std::string id_;
  std::vector<std::shared_ptr<Expression>> args_;
};

// From syntax for bracketed sequence `( expr-seq_opt )`.
class Block : public Expression {
  Block(std::vector<std::shared_ptr<Expression>> &&exprs)
      : exprs_(std::move(exprs)) {}
  virtual bool Accept(ExpressionVisitor &visitor) const {
    return visitor.VisitBlock(exprs_);
  }

private:
  std::vector<std::shared_ptr<Expression>> exprs_;
};

// Record literal
class Record : public Expression {
public:
  Record(std::string_view type_id, std::vector<FieldValue> &&field_values)
      : type_id_(type_id), field_values_(std::move(field_values)) {}
  virtual bool Accept(ExpressionVisitor &visitor) const {
    return visitor.VisitRecord(type_id_, field_values_);
  }

private:
  std::string type_id_;
  std::vector<FieldValue> field_values_;
};

// Array literal
class Array : public Expression {
public:
  Array(std::string_view type_id, std::shared_ptr<Expression> size,
        std::shared_ptr<Expression> value)
      : type_id_(type_id), size_(size), value_(value) {}
  virtual bool Accept(ExpressionVisitor &visitor) const {
    return visitor.VisitArray(type_id_, *size_, *value_);
  }

private:
  std::string type_id_;
  std::shared_ptr<Expression> size_;
  std::shared_ptr<Expression> value_;
};

class IfThen : public Expression {
public:
  IfThen(std::shared_ptr<Expression> condition,
         std::shared_ptr<Expression> expr)
      : condition_(condition), expr_(expr) {}
  virtual bool Accept(ExpressionVisitor &visitor) const {
    return visitor.VisitIfThen(*condition_, *expr_);
  }

private:
  std::shared_ptr<Expression> condition_;
  std::shared_ptr<Expression> expr_;
};

class IfThenElse : public Expression {
public:
  IfThenElse(std::shared_ptr<Expression> condition,
             std::shared_ptr<Expression> then_expr,
             std::shared_ptr<Expression> else_expr)
      : condition_(condition), then_expr_(then_expr), else_expr_(else_expr) {}
  virtual bool Accept(ExpressionVisitor &visitor) const {
    return visitor.VisitIfThenElse(*condition_, *then_expr_, *else_expr_);
  }

private:
  std::shared_ptr<Expression> condition_;
  std::shared_ptr<Expression> then_expr_;
  std::shared_ptr<Expression> else_expr_;
};

class While : public Expression {
public:
  While(std::shared_ptr<Expression> condition, std::shared_ptr<Expression> body)
      : condition_(condition), body_(body) {}
  virtual bool Accept(ExpressionVisitor &visitor) const {
    return visitor.VisitWhile(*condition_, *body_);
  }

private:
  std::shared_ptr<Expression> condition_;
  std::shared_ptr<Expression> body_;
};

class For : public Expression {
public:
  For(std::string_view id, std::shared_ptr<Expression> first,
      std::shared_ptr<Expression> last, std::shared_ptr<Expression> body)
      : id_(id), first_(first), last_(last), body_(body) {}
  virtual bool Accept(ExpressionVisitor &visitor) const {
    return visitor.VisitFor(id_, *first_, *last_, *body_);
  }

private:
  std::string id_;
  std::shared_ptr<Expression> first_;
  std::shared_ptr<Expression> last_;
  std::shared_ptr<Expression> body_;
};

class Break : public Expression {
public:
  Break() {}
  virtual bool Accept(ExpressionVisitor &visitor) const {
    return visitor.VisitBreak();
  }
};

class Let : public Expression {
public:
  Let(std::vector<std::unique_ptr<Declaration>> &&declarations)
      : declarations_(std::move(declarations)) {}
  virtual bool Accept(ExpressionVisitor &visitor) const {
    return visitor.VisitLet(declarations_);
  }

private:
  std::vector<std::unique_ptr<Declaration>> declarations_;
};
