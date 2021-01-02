#pragma once
#include "BinaryOp.h"
#include "DeclarationVisitor.h"
#include "NameSpace.h"
#include "SyntaxTreeVisitor.h"
#include "TypeVisitor.h"
#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// Pairs of classes implement the abstract syntax tree from
// http://www.cs.columbia.edu/~sedwards/classes/2002/w4115/tiger.pdf:
// nodes and visitors. Read more about this pattern at
// https://en.wikipedia.org/wiki/Visitor_pattern.  Classes for every
// node derive from pure virtual base classes Type, Declaration, or
// Expression. The node classes manage memory and implement the Accept
// method to support visitors. Visitor base classes TypeVisitor,
// DeclarationVisitor, and ExpressionVisitor, implement their member
// methods so that full traversal of child nodes is the default. As a
// special twist, value `false` stops traversal early when returned
// from Accept or a visit method.

// Abstract base class for Type nodes.
class Type {
public:
  virtual ~Type() = default;
  virtual bool Accept(TypeVisitor& visitor) const = 0;

  // Returns element type ID for an array type.
  virtual std::optional<const std::string*> GetElementType() const {
    return {};
  }

  // Returns type ID for an field for a record type.
  virtual std::optional<const std::string*>
  GetFieldType(const std::string& field_id) const {
    return {};
  }
};

// Abstract base class for Declaration nodes.
class Declaration {
public:
  Declaration(std::string_view id) : id_(id) {}
  virtual ~Declaration() = default;
  virtual bool Accept(DeclarationVisitor& visitor) const = 0;
  virtual bool Accept(SyntaxTreeVisitor& visitor) { return true; };
  // Returns the type of a type declaration
  virtual std::optional<const Type*> GetType() const { return {}; }

  // Returns type of bound variable, parameter, or function return value.
  virtual std::optional<const std::string*> GetValueType() const { return {}; }

  // Return special name space including parameters, if the declaration is a
  // function declaration.
  virtual std::optional<const NameSpace*> GetFunctionNameSpace() const {
    return {};
  }

  const std::string& Id() const { return id_; }
  virtual void SetNameSpaces(const NameSpace& types,
                             const NameSpace& non_types) {}

private:
  std::string id_;
};

// Forward declaration of visitor to navigate abstract syntax tree Expression.
class ExpressionVisitor;

// Base class for Expression nodes.
class Expression {
public:
  Expression();
  virtual ~Expression() = default;
  virtual bool Accept(ExpressionVisitor& visitor) const = 0;

  // Calls VisitNode on all children.
  virtual bool Accept(SyntaxTreeVisitor& visitor) { return true; }

  // Returns type of this expression. Undefined behavior until SetTypesBelow has
  // been called on the root.
  const std::string& GetType() const { return *type_; }

  // Sets type and non_types name spaces for every expression in the tree with
  // the given root.
  static void SetNameSpacesBelow(Expression& root);

  // Sets types in every expression in the tree with the given
  // root. Undefined behavior until SetNameSpacesBelow has been
  // called.
  static void SetTypesBelow(Expression& root);

protected:
  // Sets name spaces in this expression and its children. Default
  // implementation sets the given name spaces and nodes that establish new
  // scopes override to set different ones.
  virtual void SetNameSpaces(const NameSpace& types,
                             const NameSpace& non_types);
  friend class FunctionDeclaration;
  friend class Let;
  friend class TreeNameSpaceSetter;
  const NameSpace* types_ = nullptr;
  const NameSpace* non_types_ = nullptr;

  friend class TypeSetter;
  mutable const std::string* type_ = nullptr;
};

struct FieldValue {
  std::string id;
  std::shared_ptr<Expression> expr;
};

class LValue;

// Visit methods return false to stop. Default implementations return true after
// successfully visiting all child Expressions.
class ExpressionVisitor {
public:
  virtual bool VisitStringConstant(const std::string& text) { return true; }
  virtual bool VisitIntegerConstant(int value) { return true; }
  virtual bool VisitNil() { return true; }
  virtual bool VisitLValue(const LValue& value) { return true; }
  virtual bool VisitNegated(const Expression& value) {
    return value.Accept(*this);
  }
  virtual bool VisitBinary(const Expression& left, BinaryOp op,
                           const Expression& right) {
    return left.Accept(*this) && right.Accept(*this);
  }
  virtual bool VisitAssignment(const LValue& value, const Expression& expr) {
    return expr.Accept(*this);
  }
  virtual bool
  VisitFunctionCall(const std::string& id,
                    const std::vector<std::shared_ptr<Expression>>& args) {
    return std::all_of(args.begin(), args.end(),
                       [this](const auto& arg) { return arg->Accept(*this); });
  }
  virtual bool
  VisitBlock(const std::vector<std::shared_ptr<Expression>>& exprs) {
    return std::all_of(exprs.begin(), exprs.end(),
                       [this](const auto& arg) { return arg->Accept(*this); });
  }
  virtual bool VisitRecord(const std::string& type_id,
                           const std::vector<FieldValue>& field_values) {
    return true;
  }
  virtual bool VisitArray(const std::string& type_id, const Expression& size,
                          const Expression& value) {
    return size.Accept(*this) && value.Accept(*this);
  }
  virtual bool VisitIfThen(const Expression& condition,
                           const Expression& expr) {
    return condition.Accept(*this) && expr.Accept(*this);
  }
  virtual bool VisitIfThenElse(const Expression& condition,
                               const Expression& then_expr,
                               const Expression& else_expr) {
    return condition.Accept(*this) && then_expr.Accept(*this) &&
           else_expr.Accept(*this);
  }
  virtual bool VisitWhile(const Expression& condition, const Expression& body) {
    return condition.Accept(*this) && body.Accept(*this);
  }
  virtual bool VisitFor(const std::string& id, const Expression& first,
                        const Expression& last, const Expression& body) {
    return first.Accept(*this) && last.Accept(*this) && body.Accept(*this);
  }
  virtual bool VisitBreak() { return true; }
  virtual bool
  VisitLet(const std::vector<std::shared_ptr<Declaration>>& declarations,
           const std::vector<std::shared_ptr<Expression>>& body) {
    return std::all_of(body.begin(), body.end(),
                       [this](const auto& arg) { return arg->Accept(*this); });
  }
};

class LValueVisitor;

class LValue : public Expression {
public:
  bool Accept(ExpressionVisitor& visitor) const override {
    return visitor.VisitLValue(*this);
  }
  virtual bool Accept(LValueVisitor& visitor) const = 0;
  // Returns ID, if this L-value is an IdLValue.
  virtual std::optional<std::string> GetId() const { return {}; }
  // Returns field ID, if this L-value is an field.
  virtual std::optional<std::string> GetField() const { return {}; }
  // Returns index value, if this L-value is an IndexLValue.
  virtual std::optional<const Expression*> GetIndexValue() const { return {}; }
  // Returns child LValue for FieldLValue or IndexLValue
  virtual std::optional<const LValue*> GetChild() const { return {}; }
};

class LValueVisitor {
public:
  virtual bool VisitId(const std::string& id) { return true; }
  virtual bool VisitField(const LValue& value, const std::string& id) {
    return value.Accept(*this);
  }
  virtual bool VisitIndex(const LValue& value, const Expression& expr) {
    return value.Accept(*this);
  }
};
