#include "types.h"
#include "ScopedMap.h"
#include <iostream>

namespace {
using types::InferType;

// Visitor that binds the type of a declared value to the declaration ID.
class ValueTypeDeclarationVisitor : public DeclarationVisitor {
public:
  ValueTypeDeclarationVisitor(ScopedMap<std::string> &env) : env_(env) {}
  virtual bool
  VisitVariableDeclaration(const std::string &id,
                           const std::optional<std::string> &type_id,
                           const Expression &expr) {
    env_[id] = type_id ? *type_id : InferType(expr);
    std::cout << "Inserted " << id << "->" << env_[id] << std::endl;
    return true;
  }
  virtual bool VisitFunctionDeclaration(
      const std::string &id, const std::vector<TypeField> &params,
      const std::optional<std::string> type_id, const Expression &body) {
    env_[id] = type_id ? *type_id : InferType(body);
    return true;
  }

private:
  ScopedMap<std::string> &env_;
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
      ValueTypeDeclarationVisitor visitor(type_by_name_);
      d->Accept(visitor);
    }
    type_ = (*body.rbegin())->Accept(*this);
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
