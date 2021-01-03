#pragma once
#include "Expression.h"

// Expression visitor which returns false from all Visit methods. This is only
// useful as a base class.
class StoppingExpressionVisitor : public ExpressionVisitor {
public:
  virtual bool VisitStringConstant(const std::string& text) { return false; }
  virtual bool VisitIntegerConstant(int value) { return false; }
  virtual bool VisitNil() { return false; }
  virtual bool VisitLValue(const LValue& value) { return false; }
  virtual bool VisitNegated(const Expression& value) { return false; }
  virtual bool VisitBinary(const Expression& left, BinaryOp op,
                           const Expression& right) {
    return false;
  }
  virtual bool VisitAssignment(const LValue& value, const Expression& expr) {
    return false;
  }
  virtual bool
  VisitFunctionCall(const std::string& id,
                    const std::vector<std::shared_ptr<Expression>>& args) {
    return false;
  }
  virtual bool
  VisitBlock(const std::vector<std::shared_ptr<Expression>>& exprs) {
    return false;
  }
  virtual bool VisitRecord(const std::string& type_id,
                           const std::vector<FieldValue>& field_values) {
    return false;
  }
  virtual bool VisitArray(const std::string& type_id, const Expression& size,
                          const Expression& value) {
    return false;
  }
  virtual bool VisitIfThen(const Expression& condition,
                           const Expression& expr) {
    return false;
  }
  virtual bool VisitIfThenElse(const Expression& condition,
                               const Expression& then_expr,
                               const Expression& else_expr) {
    return false;
  }
  virtual bool VisitWhile(const Expression& condition, const Expression& body) {
    return false;
  }
  virtual bool VisitFor(const std::string& id, const Expression& first,
                        const Expression& last, const Expression& body) {
    return false;
  }
  virtual bool VisitBreak() { return false; }
  virtual bool
  VisitLet(const std::vector<std::shared_ptr<Declaration>>& declarations,
           const std::vector<std::shared_ptr<Expression>>& body) {
    return false;
  }
};
