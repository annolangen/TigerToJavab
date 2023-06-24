#include "Checker.h"

#include <functional>
#include <sstream>

#include "ToString.h"
#include "syntax.h"
#include "syntax_insertion.h"

namespace {
using Errors = std::vector<std::string>;

struct Emitter : public std::ostringstream {
  Emitter(Errors& v) : errors(v) {}
  ~Emitter() { errors.push_back(std::ostringstream::str()); }
  Errors& errors;
};

template <class... O>
struct Checker : TreeVisitor<Checker<O...>, O...> {
  Emitter emit() { return {errors_}; }
  Errors& errors_;
};

// Record literal field names, expression types, and the order
// thereof must exactly match those of the given record type (2.3)
struct RecordFieldChecker : Checker {
  RecordFieldChecker(Errors& errors)
      : Checker(
            ExpressionVisitor{

            },
            errors) {}
  bool VisitRecord(const std::string& type_id,
                   const std::vector<FieldValue>& field_values,
                   const Expression& exp) override {
    if (auto d = exp.GetTypeNameSpace().Lookup(type_id); !d) {
      emit() << "Unknown record type " << type_id;
    } else if (auto t = (*d)->GetType(); !t) {
      emit() << "Internal error: missing RHS for type declaration " << type_id;
    } else if (auto r = (*t)->recordType(); !r) {
      emit() << "Type " << type_id << " is not a record";
    } else if (auto record_fields = (*r)->Fields();
               field_values.size() != record_fields.size()) {
      emit() << "Field counts differ for " << type_id << " " << **t << " and "
             << exp;
    } else {
      for (int i = field_values.size(); --i >= 0;) {
        if (auto id = field_values[i].id; id != record_fields[i].id) {
          emit() << "Different names " << id << " and " << record_fields[i].id
                 << " for field #" << (i + 1) << " of record " << type_id;
        } else if (auto t = field_values[i].expr->GetType();
                   t != record_fields[i].type_id) {
          emit() << "Different types " << t << " and "
                 << record_fields[i].type_id << " for field " << id
                 << " of record " << type_id;
        }
      }
    }
    return false;
  }
};

bool IsPrimitive(const std::string& type) {
  return type == "int" || type == "string";
}

// Binary operators >, <, >=, and <= may be either both integer or both string
// (2.5). Operators & and | are lazy logical operators on integers (2.5)
struct BinaryOpChecker : Checker {
  BinaryOpChecker(Errors& errors) : Checker(errors) {}
  bool VisitBinary(const Expression& left, BinaryOp op,
                   const Expression& right) override {
    switch (op) {
      case kGreaterThan:
      case kLessThan:
      case kNotGreaterThan:
      case kNotLessThan:
        CheckComparison(left.GetType(), right.GetType(), op);
        break;
      case kAnd:
      case kOr:
        CheckInt(left.GetType(), op);
        CheckInt(right.GetType(), op);
        break;
      default:
        break;
    }
    return false;
  }
  void CheckComparison(const std::string& left_type,
                       const std::string& right_type, BinaryOp op) {
    CheckPrimitive(left_type, op);
    CheckPrimitive(right_type, op);
    if (left_type != right_type) {
      emit() << "Types of " << op << " should match, but got " << left_type
             << " and " << right_type;
    }
  }
  void CheckPrimitive(const std::string& type, BinaryOp op) {
    if (!IsPrimitive(type)) {
      emit() << "Operand type of " << op << " must be int or string, but got "
             << type;
    }
  }
  void CheckInt(const std::string& type, BinaryOp op) {
    if (type != "int") {
      emit() << "Operand type for " << op << " must be int, but got " << type;
    }
  }
};

// Conditionals must evaluate to integers (2.8)
struct ConditionalChecker : Checker {
  ConditionalChecker(Errors& errors) : Checker(errors) {}
  bool VisitIfThen(const Expression& condition, const Expression&) override {
    return CheckInt(condition);
  }
  bool VisitIfThenElse(const Expression& condition, const Expression&,
                       const Expression&) override {
    return CheckInt(condition);
  }
  virtual bool VisitWhile(const Expression& condition,
                          const Expression&) override {
    return CheckInt(condition);
  }
  bool CheckInt(const Expression& condition) {
    const std::string& type = condition.GetType();
    if (type != "int") {
      emit() << "Conditions must be int, but got " << type;
    }
    return false;
  }
};

#define CHECK_BUILDER(C) \
  [](Errors& errors) { return std::make_unique<C>(errors); }

std::vector<CheckerBuilder> kCheckerBuilders = {
    CHECK_BUILDER(RecordFieldChecker),
    CHECK_BUILDER(BinaryOpChecker),
    CHECK_BUILDER(ConditionalChecker),
};

void CheckBelow(const Expression& parent, Errors& errors,
                std::vector<CheckerBuilder>& checker_builders) {
  for (auto f : checker_builders) {
    auto checker = f(errors);
    parent.Accept(*checker);
  }
  for (auto* n : parent.Children()) {
    if (auto e = n->expression(); e) CheckBelow(**e, errors, checker_builders);
  }
}

}  // namespace

Errors ListErrors(const Expr& root) {
  Errors errors;
  // CheckBelow(root, errors, kCheckerBuilders);
  return errors;
}
