#include "types.h"
#include "ScopedMap.h"

namespace {
using types::InferType;

// TODO remove this
// Visitor that infers the type of an L-value in the context of
// declarations. Writes the inferred type to string `type`.
//
class InferLValueVisitor : public LValueVisitor {
public:
  InferLValueVisitor(const ScopedMap<std::string> &inferred_type_by_name,
                     const ScopedMap<const Type *> &declared_type_by_name,
                     std::string &type)
      : inferred_type_by_name_(inferred_type_by_name),
        declared_type_by_name_(declared_type_by_name), type_(type) {}
  virtual bool VisitId(const std::string &id) {
    auto found = inferred_type_by_name_.Lookup(id);
    type_ = found ? **found : "none";
    return true;
  }
  virtual bool VisitField(const LValue &value, const std::string &id) {

    return value.Accept(*this);
  }
  virtual bool VisitIndex(const LValue &value, const Expression &expr) {
    return value.Accept(*this);
  }

private:
  const ScopedMap<std::string> &inferred_type_by_name_;
  const ScopedMap<const Type *> &declared_type_by_name_;
  std::string &type_;
}; // namespace

// Visitor that binds the type of a declared value to the declaration ID.
class InferDeclarationVisitor : public DeclarationVisitor {
public:
  InferDeclarationVisitor(ScopedMap<std::string> &inferred_type_by_name)
      : inferred_type_by_name_(inferred_type_by_name) {}
  virtual bool
  VisitVariableDeclaration(const std::string &id,
                           const std::optional<std::string> &type_id,
                           const Expression &expr) {
    inferred_type_by_name_[id] = type_id ? *type_id : InferType(expr);
    return true;
  }
  virtual bool VisitFunctionDeclaration(
      const std::string &id, const std::vector<TypeField> &params,
      const std::optional<std::string> type_id, const Expression &body) {
    inferred_type_by_name_[id] = type_id ? *type_id : InferType(body);
    return true;
  }

private:
  ScopedMap<std::string> &inferred_type_by_name_;
};

class InferExpressionVisitor : public ExpressionVisitor {
public:
  InferExpressionVisitor(std::string &type) : type_(type) {}
  virtual bool VisitStringConstant(const std::string &text) {
    return SetType("string");
  }
  virtual bool VisitIntegerConstant(int value) { return SetType("int"); }
  virtual bool VisitNil() { return SetType("none"); }
  virtual bool VisitLValue(const LValue &value) { return SetType("none"); }
  virtual bool VisitNegated(const Expression &value) { return SetType("int"); }
  virtual bool VisitBinary(const Expression &left, BinaryOp op,
                           const Expression &right) {
    return right.Accept(*this);
  }
  virtual bool VisitAssignment(const LValue &value, const Expression &expr) {
    return SetType("none");
  }
  virtual bool
  VisitFunctionCall(const std::string &id,
                    const std::vector<std::shared_ptr<Expression>> &args) {
    auto found = type_by_name_.Lookup(id);
    return SetType(found ? **found : "none");
  }
  virtual bool
  VisitBlock(const std::vector<std::shared_ptr<Expression>> &exprs) {
    return (*exprs.rbegin())->Accept(*this);
  }
  virtual bool VisitRecord(const std::string &type_id,
                           const std::vector<FieldValue> &field_values) {
    return SetType(type_id);
  }
  virtual bool VisitArray(const std::string &type_id, const Expression &size,
                          const Expression &value) {
    return SetType(type_id);
  }
  virtual bool VisitIfThen(const Expression &condition,
                           const Expression &expr) {
    return SetType("none");
  }
  virtual bool VisitIfThenElse(const Expression &condition,
                               const Expression &then_expr,
                               const Expression &else_expr) {
    return else_expr.Accept(*this);
  }
  virtual bool VisitWhile(const Expression &condition, const Expression &body) {
    return SetType("none");
  }
  virtual bool VisitFor(const std::string &id, const Expression &first,
                        const Expression &last, const Expression &body) {
    return SetType("none");
  }
  virtual bool VisitBreak() { return SetType("none"); }
  virtual bool
  VisitLet(const std::vector<std::unique_ptr<Declaration>> &declarations,
           const std::vector<std::unique_ptr<Expression>> &body) {
    if (body.empty()) {
      return SetType("none");
    }
    type_by_name_.EnterScope();
    for (const auto &d : declarations) {
      InferDeclarationVisitor visitor(type_by_name_);
      d->Accept(visitor);
    }
    (*body.rbegin())->Accept(*this);
    type_by_name_.ExitScope();
    return true;
  }
  bool SetType(const std::string &type) {
    type_ = type;
    return true;
  }
  std::string &type_;
  ScopedMap<std::string> type_by_name_;
};
} // namespace
namespace types {
std::string InferType(const Expression &e) {
  std::string result;
  InferExpressionVisitor visitor(result);
  e.Accept(visitor);
  return result;
}
} // namespace types
