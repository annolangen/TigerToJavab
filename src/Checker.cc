#include "Checker.h"
#include "StoppingExpressionVisitor.h"
#include "ToString.h"
#include "syntax_nodes.h"
#include <functional>
#include <sstream>

namespace {
using Errors = std::vector<std::string>;
using CheckFunction = std::function<void(const Expression&, Errors&)>;

struct Emitter : public std::ostringstream {
  Emitter(Errors& v) : errors(v) {}
  ~Emitter() { errors.push_back(std::ostringstream::str()); }
  Errors& errors;
};

struct Checker : public StoppingExpressionVisitor {
  Checker(Errors& errors) : errors_(errors) {}
  Emitter emit() { return {errors_}; }
  Errors& errors_;
};

// Record literal field names, expression types, and the order
// thereof must exactly match those of the given record type (2.3)
struct RecordFieldChecker : Checker {
  RecordFieldChecker(const Expression& e, Errors& errors)
      : expr_(e), Checker(errors) {}
  bool VisitRecord(const std::string& type_id,
                   const std::vector<FieldValue>& field_values) override {
    if (auto d = expr_.GetTypeNameSpace().Lookup(type_id); !d) {
      emit() << "Unknown record type " << type_id;
    } else if (auto t = (*d)->GetType(); !t) {
      emit() << "Internal error: missing RHS for type declaration " << type_id;
    } else if (auto r = (*t)->recordType(); !r) {
      emit() << "Type " << type_id << " is not a record";
    } else if (auto record_fields = (*r)->Fields();
               field_values.size() != record_fields.size()) {
      emit() << "Field counts differ for " << type_id << " " << **t << " and "
             << expr_;
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

  const Expression& expr_;
};

std::vector<CheckFunction> kCheckFunctions = {
    [](const Expression& exp, Errors& errors) {
      RecordFieldChecker checker(exp, errors);
      exp.Accept(checker);
    }};

void CheckBelow(const Expression& parent, Errors& errors,
                std::vector<CheckFunction>& checkers) {
  for (auto f : checkers) f(parent, errors);
  for (auto* n : parent.Children()) {
    if (auto e = n->expression(); e) CheckBelow(**e, errors, checkers);
  }
}

} // namespace

Errors ListErrors(const Expression& root) {
  Errors errors;
  CheckBelow(root, errors, kCheckFunctions);
  return errors;
}
