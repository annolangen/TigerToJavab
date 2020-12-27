#include "ToString.h"
#include "util.h"

std::ostream& operator<<(std::ostream& os, const TypeField& f) {
  return os << f.id << ": " << f.type_id;
}

namespace {
using util::join;

class AppendTypeVisitor : public TypeVisitor {
public:
  AppendTypeVisitor(std::ostream& os) : os_(os) {}
  bool VisitTypeReference(const std::string& id) override {
    os_ << id;
    return true;
  }

  bool VisitRecordType(const std::vector<TypeField>& fields) override {
    os_ << "{" << (fields | join(", ")) << "}";
    return true;
  }

  bool VisitArrayType(const std::string& type_id) override {
    os_ << "array of " << type_id;
    return true;
  }

  bool VisitInt() override {
    os_ << "int";
    return true;
  }

  bool VisitString() override {
    os_ << "string";
    return true;
  }

private:
  std::ostream& os_;
};

class AppendLValueVisitor : public LValueVisitor {
public:
  AppendLValueVisitor(std::ostream& os, ExpressionVisitor& expression_visitor)
      : os_(os), expression_visitor_(expression_visitor) {}

  bool VisitId(const std::string& id) override {
    os_ << id;
    return true;
  }
  bool VisitField(const LValue& value, const std::string& id) override {
    if (!value.Accept(*this)) return false;
    os_ << '.' << id;
    return true;
  }
  bool VisitIndex(const LValue& value, const Expression& expr) override {
    if (!value.Accept(*this)) return false;
    os_ << '[';
    expr.Accept(expression_visitor_);
    os_ << ']';
    return true;
  }

private:
  std::ostream& os_;
  ExpressionVisitor& expression_visitor_;
};

class AppendExpressionVisitor : public ExpressionVisitor {
public:
  AppendExpressionVisitor(std::ostream& os) : os_(os) {}
  bool VisitStringConstant(const std::string& text) override {
    // TODO escape characters that need escaping
    os_ << '"' << text << '"';
    return true;
  }
  bool VisitIntegerConstant(int value) override {
    os_ << value;
    return true;
  }
  bool VisitNil() override {
    os_ << "nil";
    return true;
  }
  bool VisitLValue(const LValue& value) override {
    AppendLValueVisitor l_value_visitor(os_, *this);
    value.Accept(l_value_visitor);
    return true;
  }
  bool VisitNegated(const Expression& value) override {
    os_ << '-';
    return value.Accept(*this);
  }
  bool VisitBinary(const Expression& left, BinaryOp op,
                   const Expression& right) override {
    if (!left.Accept(*this)) return false;
    os_ << op;
    return right.Accept(*this);
  }
  bool VisitAssignment(const LValue& value, const Expression& expr) override {
    AppendLValueVisitor l_value_visitor(os_, *this);
    value.Accept(l_value_visitor);
    os_ << ":=";
    return expr.Accept(*this);
  }
  bool VisitFunctionCall(
      const std::string& id,
      const std::vector<std::shared_ptr<Expression>>& args) override {
    os_ << id << "(";
    Join(args, ", ");
    os_ << ")";
    return true;
  }
  bool
  VisitBlock(const std::vector<std::shared_ptr<Expression>>& exprs) override {
    os_ << "(";
    Join(exprs, "; ");
    os_ << ")";
    return true;
  }
  bool VisitRecord(const std::string& type_id,
                   const std::vector<FieldValue>& field_values) override {
    return true;
  }
  bool VisitArray(const std::string& type_id, const Expression& size,
                  const Expression& value) override {
    return size.Accept(*this) && value.Accept(*this);
  }
  bool VisitIfThen(const Expression& condition,
                   const Expression& expr) override {
    return condition.Accept(*this) && expr.Accept(*this);
  }
  bool VisitIfThenElse(const Expression& condition, const Expression& then_expr,
                       const Expression& else_expr) override {
    return condition.Accept(*this) && then_expr.Accept(*this) &&
           else_expr.Accept(*this);
  }
  bool VisitWhile(const Expression& condition,
                  const Expression& body) override {
    return condition.Accept(*this) && body.Accept(*this);
  }
  bool VisitFor(const std::string& id, const Expression& first,
                const Expression& last, const Expression& body) override {
    return first.Accept(*this) && last.Accept(*this) && body.Accept(*this);
  }
  bool VisitBreak() override { return true; }
  bool VisitLet(const std::vector<std::shared_ptr<Declaration>>& declarations,
                const std::vector<std::shared_ptr<Expression>>& body) override {
    return std::all_of(body.begin(), body.end(),
                       [this](const auto& arg) { return arg->Accept(*this); });
  }
  void Join(const std::vector<std::shared_ptr<Expression>>& expressions,
            const char* join_text) {
    const char* sep = "";
    for (const auto& e : expressions) {
      os_ << sep;
      e->Accept(*this);
      sep = join_text;
    }
  }

private:
  std::ostream& os_;
};
} // namespace

std::ostream& operator<<(std::ostream& os, const Type& t) {
  AppendTypeVisitor visitor(os);
  t.Accept(visitor);
  return os;
}

std::ostream& operator<<(std::ostream& os, const Expression& e) {
  AppendExpressionVisitor visitor(os);
  e.Accept(visitor);
  return os;
}

std::ostream& operator<<(std::ostream& os, const Declaration& d) { return os; }
