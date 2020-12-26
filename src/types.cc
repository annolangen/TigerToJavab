#include "types.h"
#include "ScopedMap.h"

namespace {
using types::EnterScope;
using types::InferType;
using types::ScopeExiter;

// Visitor that infers the type of an L-value in the context of
// declarations. Writes the inferred type to string `type`.
class InferLValueVisitor : public LValueVisitor {
public:
  InferLValueVisitor(const ScopedMap<std::string>& inferred_type_by_name,
                     const ScopedMap<const Type*>& declared_type_by_name,
                     std::string& type)
      : inferred_type_by_name_(inferred_type_by_name),
        declared_type_by_name_(declared_type_by_name), type_(type) {}
  bool VisitId(const std::string& id) override {
    auto found = inferred_type_by_name_.Lookup(id);
    type_ = found ? *found : "none";
    return true;
  }
  bool VisitField(const LValue& value, const std::string& id) override {
    if (!value.Accept(*this)) return false;
    // Now type_ holds the ID for the record type
    if (auto value_type = declared_type_by_name_.Lookup(type_); value_type) {
      if (auto field_type = (*value_type)->GetFieldType(id); field_type) {
        type_ = *field_type;
      }
    }
    return true;
  }
  bool VisitIndex(const LValue& value, const Expression& expr) override {
    if (!value.Accept(*this)) return false;
    // Now type_ holds the ID for the array type
    if (auto array_type = declared_type_by_name_.Lookup(type_); array_type) {
      if (auto element_type = (*array_type)->GetElementType(); element_type) {
        type_ = *element_type;
      }
    }
    return true;
  }

private:
  const ScopedMap<std::string>& inferred_type_by_name_;
  const ScopedMap<const Type*>& declared_type_by_name_;
  std::string& type_;
};

std::string
InferLValueType(const LValue& value,
                const ScopedMap<std::string>& inferred_type_by_name,
                const ScopedMap<const Type*>& declared_type_by_name) {
  std::string result = "none";
  InferLValueVisitor visitor(inferred_type_by_name, declared_type_by_name,
                             result);
  value.Accept(visitor);
  return result;
}

class InferExpressionVisitor : public ExpressionVisitor {
public:
  InferExpressionVisitor(std::string& type) : type_(type) {}
  bool VisitStringConstant(const std::string& text) override {
    return SetType("string");
  }
  bool VisitIntegerConstant(int value) override { return SetType("int"); }
  bool VisitNil() override { return SetType("none"); }
  bool VisitLValue(const LValue& value) override {
    return SetType(
        InferLValueType(value, inferred_type_by_name_, declared_type_by_name_));
  }
  bool VisitNegated(const Expression& value) override { return SetType("int"); }
  bool VisitBinary(const Expression& left, BinaryOp op,
                   const Expression& right) override {
    return right.Accept(*this);
  }
  bool VisitAssignment(const LValue& value, const Expression& expr) override {
    return SetType("none");
  }
  bool VisitFunctionCall(
      const std::string& id,
      const std::vector<std::shared_ptr<Expression>>& args) override {
    auto found = inferred_type_by_name_.Lookup(id);
    return SetType(found ? *found : "none");
  }
  bool
  VisitBlock(const std::vector<std::shared_ptr<Expression>>& exprs) override {
    return (*exprs.rbegin())->Accept(*this);
  }
  bool VisitRecord(const std::string& type_id,
                   const std::vector<FieldValue>& field_values) override {
    return SetType(type_id);
  }
  bool VisitArray(const std::string& type_id, const Expression& size,
                  const Expression& value) override {
    return SetType(type_id);
  }
  bool VisitIfThen(const Expression& condition,
                   const Expression& expr) override {
    return SetType("none");
  }
  bool VisitIfThenElse(const Expression& condition, const Expression& then_expr,
                       const Expression& else_expr) override {
    return else_expr.Accept(*this);
  }
  bool VisitWhile(const Expression& condition,
                  const Expression& body) override {
    return SetType("none");
  }
  bool VisitFor(const std::string& id, const Expression& first,
                const Expression& last, const Expression& body) override {
    return SetType("none");
  }
  bool VisitBreak() override { return SetType("none"); }
  bool VisitLet(const std::vector<std::shared_ptr<Declaration>>& declarations,
                const std::vector<std::shared_ptr<Expression>>& body) override;
  bool SetType(const std::string& type) {
    type_ = type;
    return true;
  }
  std::string InferType(const Expression& e) {
    e.Accept(*this);
    return type_;
  }
  std::string& type_;
  ScopedMap<std::string> inferred_type_by_name_;
  ScopedMap<const Type*> declared_type_by_name_;
};

// Visitor that binds the type of a declared value to the declaration ID.
class InferDeclarationVisitor : public DeclarationVisitor {
public:
  InferDeclarationVisitor(std::optional<std::string>& result,
                          InferExpressionVisitor& expression_visitor)
      : result_(result), expression_visitor_(expression_visitor) {}
  bool VisitVariableDeclaration(const std::string& id,
                                const std::optional<std::string>& type_id,
                                const Expression& expr) override {
    result_ = type_id ? *type_id : expression_visitor_.InferType(expr);
    return true;
  }
  bool VisitFunctionDeclaration(const std::string& id,
                                const std::vector<TypeField>& params,
                                const std::optional<std::string> type_id,
                                const Expression& body) override {
    result_ = type_id ? *type_id : expression_visitor_.InferType(body);
    return true;
  }

private:
  std::optional<std::string>& result_;
  InferExpressionVisitor expression_visitor_;
};

bool InferExpressionVisitor::VisitLet(
    const std::vector<std::shared_ptr<Declaration>>& declarations,
    const std::vector<std::shared_ptr<Expression>>& body) {
  if (body.empty()) {
    return SetType("none");
  }
  ScopeExiter<const Type*> declared_exiter{
      EnterScope(declarations, declared_type_by_name_)};
  auto infer_declaration_type = [this](const Declaration& d) {
    std::optional<std::string> result;
    InferDeclarationVisitor visitor(result, *this);
    d.Accept(visitor);
    return result;
  };
  ScopeExiter<std::string> inferred_exiter{EnterScope<std::string>(
      declarations, infer_declaration_type, inferred_type_by_name_)};
  (*body.rbegin())->Accept(*this);
  return true;
}

} // namespace

namespace types {
std::string InferType(const Expression& e) {
  std::string result;
  return InferExpressionVisitor(result).InferType(e);
}

ScopedMap<const Type*>&
EnterScope(const std::vector<std::shared_ptr<Declaration>>& declarations,
           ScopedMap<const Type*>& type_namespace) {
  return EnterScope<const Type*>(declarations, &Declaration::GetType,
                                 type_namespace);
}
} // namespace types
