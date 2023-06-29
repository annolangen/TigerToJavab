#include "checker.h"

#include <functional>
#include <sstream>

#include "debug_string.h"
#include "symbol_table.h"
#include "syntax.h"
#include "type_finder.h"

namespace {
using namespace syntax;
using Errors = std::vector<std::string>;

struct Emitter : public std::ostringstream {
  Emitter(Errors& v) : errors(v) {}
  ~Emitter() { errors.push_back(std::ostringstream::str()); }
  Errors& errors;
};

struct Checker {
  template <class T>
  bool operator()(const T&) {
    return true;
  }
  Emitter emit() { return {errors}; }
  Errors& errors;
  const SymbolTable& symbols;
  TypeFinder& get_type;
};

// Record literal field names, expression types, and the order
// thereof must exactly match those of the given record type (2.3)
struct RecordFieldChecker : Checker {
  RecordFieldChecker(Errors& errors, const SymbolTable& symbols, TypeFinder& tf)
      : Checker{errors, symbols, tf} {}
  using Checker::operator();
  bool operator()(const Expr& e) {
    const RecordLiteral* lit = std::get_if<RecordLiteral>(&e);
    if (!lit) return true;
    const TypeDeclaration* d = symbols.lookupType(e, lit->type_id);
    while (true) {
      if (!d) {
        emit() << "Unknown record type " << lit->type_id;
        return false;
      }
      const auto* alias = std::get_if<Identifier>(&d->value);
      if (!alias) break;
      d = symbols.lookupType(e, *alias);
    }
    const auto* tf = std::get_if<TypeFields>(&d->value);
    if (!tf) {
      emit() << "Type " << lit->type_id << " is not a record";
      return false;
    }
    const std::vector<FieldAssignment>& assignments = lit->fields;
    if (assignments.size() != tf->size()) {
      emit() << "Type " << d->id << " has " << tf->size()
             << " fields and literal has " << assignments.size();
      return false;
    }
    // > Field names, expression types, and the order thereof must exactly match
    // > those of the given record type.
    for (int i = tf->size(); --i >= 0;) {
      if (assignments[i].id != tf->at(i).id) {
        emit() << "Different names " << assignments[i].id << " and "
               << tf->at(i).id << " for field #" << (i + 1) << " of record "
               << d->id;
      } else if (std::string_view t = get_type(*assignments[i].expr);
                 t != tf->at(i).type_id) {
        emit() << "Different types " << t << " and " << tf->at(i).type_id
               << " for field #" << (i + 1) << " of record " << d->id;
      }
    }
    return true;
  }
};

// Binary operators >, <, >=, and <= may be either both integer or both string
// (2.5). Operators & and | are lazy logical operators on integers (2.5)
struct BinaryOpChecker : Checker {
  BinaryOpChecker(Errors& errors, const SymbolTable& symbols, TypeFinder& tf)
      : Checker{errors, symbols, tf} {}
  using Checker::operator();
  bool operator()(const Binary& b) {
    switch (b.op) {
      case kGreaterThan:
      case kLessThan:
      case kNotGreaterThan:
      case kNotLessThan:
        CheckComparison(get_type(*b.left), get_type(*b.right), b.op);
        break;
      case kAnd:
      case kOr:
        CheckInt(get_type(*b.left), b.op);
        CheckInt(get_type(*b.right), b.op);
        break;
      default:
        break;
    }
    return false;
  }
  void CheckComparison(std::string_view left_type, std::string_view right_type,
                       BinaryOp op) {
    CheckPrimitive(left_type, op);
    CheckPrimitive(right_type, op);
    if (left_type != right_type) {
      emit() << "Types of " << op << " should match, but got " << left_type
             << " and " << right_type;
    }
  }
  void CheckPrimitive(std::string_view type, BinaryOp op) {
    if (type != "int" && type != "string") {
      emit() << "Operand type of " << op << " must be int or string, but got "
             << type;
    }
  }
  void CheckInt(std::string_view type, BinaryOp op) {
    if (type != "int") {
      emit() << "Operand type for " << op << " must be int, but got " << type;
    }
  }
};

// Conditionals must evaluate to integers (2.8)
struct ConditionalChecker : Checker {
  ConditionalChecker(Errors& errors, const SymbolTable& symbols, TypeFinder& tf)
      : Checker{errors, symbols, tf} {}
  bool operator()(const Expr& e) { return std::visit(*this, e); }
  bool operator()(const IfThen& it) { return CheckInt(*it.condition); }
  bool operator()(const IfThenElse& ite) { return CheckInt(*ite.condition); }
  bool operator()(const While& w) { return CheckInt(*w.condition); }
  using Checker::operator();

  bool CheckInt(const Expr& condition) {
    std::string_view type = get_type(condition);
    if (type != "int") {
      emit() << "Conditions must be int, but got " << type;
    }
    return true;
  }
};

template <class C>
void CheckBelow(const Expr& root, C&& checker) {
  checker(root);
  std::visit(
      [&](const auto& n) {
        VisitChildren(n, [&](const auto& child) {
          checker(child);
          return true;
        });
      },
      root);
}

template <class... C>
void CheckBelow(const Expr& root, Errors& errors, const SymbolTable& t,
                TypeFinder& tf) {
  (CheckBelow<C>(root, C{errors, t, tf}), ...);
}
}  // namespace

Errors ListErrors(const Expr& root, const SymbolTable& t, TypeFinder& tf) {
  Errors errors;
  CheckBelow<RecordFieldChecker, BinaryOpChecker, ConditionalChecker>(
      root, errors, t, tf);
  return errors;
}
