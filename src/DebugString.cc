#include "DebugString.h"
#include "BinaryOp.h"
#include "ToString.h"
#include <functional>
#include <initializer_list>
#include <sstream>
#include <utility>

namespace {
using KVPair = std::pair<std::string, std::string>;
using KVPairs = std::vector<KVPair>;

struct ExprVisitorPair {
  const std::string_view key;
  const Expression& expr;
  ExpressionVisitor& visitor;
};

struct Appendable {
  Appendable& operator<<(const std::string_view s) {
    out += s;
    return *this;
  }
  template <class T> Appendable& operator<<(const std::vector<T>& v) {
    const char* sep = "";
    for (const auto& e : v) {
      out += sep;
      (*this) << e;
      sep = " ";
    }
    return *this;
  }
  Appendable& operator<<(const TypeField& f) {
    return *this << "TypeField{"
                 << KVPairs{{"id", f.id}, {"type_id", f.type_id}} << "}";
  }
  Appendable& operator<<(const KVPair& p) {
    return *this << p.first << ": " << p.second;
  }
  Appendable& operator<<(ExprVisitorPair&& p) {
    *this << p.key << ": ";
    p.expr.Accept(p.visitor);
    return *this;
  }
  Appendable& operator<<(const std::function<Appendable&(Appendable&)>& f) {
    return f(*this);
  }
  // So that returning an Appendable is like "return true" in a Visit* function.
  operator bool() { return true; }
  std::string& out;
};

Appendable emit(std::string& out) { return {out}; }

class AppendTypeVisitor : public TypeVisitor {
public:
  AppendTypeVisitor(std::string& out) : out_(out) {}
  bool VisitTypeReference(const std::string& id) override {
    return emit(out_) << "TypeReference{" << KVPairs{{"id", id}} << "}";
  }

  bool VisitRecordType(const std::vector<TypeField>& fields) override {
    return emit(out_) << "RecordType{" << fields << "}";
  }

  bool VisitArrayType(const std::string& type_id) override {
    return emit(out_) << "ArrayType{" << KVPair{"type_id", type_id} << "}";
  }

  bool VisitInt() override { return emit(out_) << "Int{}"; }

  bool VisitString() override { return emit(out_) << "String{}"; }

private:
  std::string& out_;
};

class AppendLValueVisitor : public LValueVisitor {
public:
  AppendLValueVisitor(std::string& out, ExpressionVisitor& expression_visitor)
      : out_(out), expression_visitor_(expression_visitor) {}

  bool VisitId(const std::string& id) override {
    return emit(out_) << "Id{" << KVPair{"id", id} << "}";
  }
  bool VisitField(const LValue& value, const std::string& id) override {
    out_ += "Field{l_value: ";
    value.Accept(*this);
    return emit(out_) << " " << KVPair{" id", id} << "}";
  }
  bool VisitIndex(const LValue& value, const Expression& expr) override {
    out_ += "Index{l_value: ";
    value.Accept(*this);
    return emit(out_) << ExprVisitorPair{" expr", expr, expression_visitor_}
                      << "}";
  }

private:
  std::string& out_;
  ExpressionVisitor& expression_visitor_;
};

template <class T>
std::function<Appendable&(Appendable&)>
ArrayString(const std::vector<std::shared_ptr<T>>& v) {
  return [&v](Appendable& a) -> Appendable& {
    const char* sep = "";
    a << "[";
    for (const auto& i : v) {
      a << sep << ToString(*i);
      sep = "\n";
    }
    return a << "]";
  };
}

class AppendExpressionVisitor : public ExpressionVisitor {
public:
  AppendExpressionVisitor(std::string& out) : out_(out) {}
  bool VisitStringConstant(const std::string& text) override {
    return emit(out_) << "String{" << text << "}";
  }
  bool VisitIntegerConstant(int value) override {
    std::ostringstream value_string;
    value_string << value;
    return emit(out_) << "Int{" << value_string.str() << "}";
  }
  bool VisitNil() override { return emit(out_) << "Nil"; }
  bool VisitLValue(const LValue& value) override {
    AppendLValueVisitor l_value_visitor(out_, *this);
    value.Accept(l_value_visitor);
    return true;
  }
  bool VisitNegated(const Expression& value) override {
    return emit(out_) << "Negated{" << ExprVisitorPair{"value", value, *this}
                      << "}";
  }
  bool VisitBinary(const Expression& left, BinaryOp op,
                   const Expression& right) override {
    return emit(out_) << "Binary{" << ExprVisitorPair{"left", left, *this}
                      << KVPair{" op", ToString(op)}
                      << ExprVisitorPair{" right", right, *this} << "}";
  }
  bool VisitAssignment(const LValue& value, const Expression& expr) override {
    out_ += "Assign{l_value: ";
    AppendLValueVisitor l_value_visitor(out_, *this);
    value.Accept(l_value_visitor);
    return emit(out_) << ExprVisitorPair{" expr", expr, *this} << "}";
  }
  bool VisitFunctionCall(
      const std::string& id,
      const std::vector<std::shared_ptr<Expression>>& args) override {
    return emit(out_) << "FunctionCall{" << KVPair{"id", id} << " "
                      << Join("arg", args) << "}";
  }
  bool
  VisitBlock(const std::vector<std::shared_ptr<Expression>>& exprs) override {
    return emit(out_) << "Block{" << Join("expr", exprs) << "}";
  }
  bool VisitRecord(const std::string& type_id,
                   const std::vector<FieldValue>& field_values) override {
    emit(out_) << "Record{";
    const char* sep = "";
    for (const auto& f : field_values) {
      emit(out_) << sep << ExprVisitorPair{f.id, *f.expr, *this};
      sep = " ";
    }
    return emit(out_) << "}";
  }
  bool VisitArray(const std::string& type_id, const Expression& size,
                  const Expression& value) override {
    return emit(out_) << "Array{" << KVPair{"type_id", type_id}
                      << ExprVisitorPair{" size", size, *this}
                      << ExprVisitorPair{" value", value, *this} << "}";
  }
  bool VisitIfThen(const Expression& condition,
                   const Expression& expr) override {
    return emit(out_) << "IfThen{"
                      << ExprVisitorPair{"condition", condition, *this}
                      << ExprVisitorPair{" expr", expr, *this} << "}";
  }
  bool VisitIfThenElse(const Expression& condition, const Expression& then_expr,
                       const Expression& else_expr) override {
    return emit(out_) << "IfThen{"
                      << ExprVisitorPair{"condition", condition, *this}
                      << ExprVisitorPair{" then_expr", then_expr, *this}
                      << ExprVisitorPair{" else_expr", else_expr, *this} << "}";
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
    return emit(out_) << "Let{declarations: " << ArrayString(declarations)
                      << Join(" expr", body) << "}";
  }
  std::function<Appendable&(Appendable&)>
  Join(const char* key, const std::vector<std::shared_ptr<Expression>>& v) {
    return [key, &v, this](Appendable& out) -> Appendable& {
      const char* sep = "";
      for (const auto& e : v) {
        out << sep;
        out << ExprVisitorPair{key, *e, *this};
        sep = " ";
      }
      return out;
    };
  }

private:
  std::string& out_;
};
} // namespace

std::string& AppendDebugString(std::string& out, const TypeField& f) {
  emit(out) << "TypeField{" << KVPairs{{"id", f.id}, {"type_id", f.type_id}}
            << "}";
  return out;
}

std::string& AppendDebugString(std::string& out, const Type& t) {
  AppendTypeVisitor visitor(out);
  t.Accept(visitor);
  return out;
}

std::string& AppendDebugString(std::string& out, const Expression& e) {
  AppendExpressionVisitor visitor(out);
  e.Accept(visitor);
  return out;
}
std::string& AppendDebugString(std::string& out, const Declaration& d) {
  return out += ToString(d);
}
