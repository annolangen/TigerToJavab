#include "checker.h"

#include <functional>
#include <iostream>
#include <sstream>
#include <string_view>
#include <unordered_set>

#include "debug_string.h"
#include "symbol_table.h"
#include "syntax.h"
#include "type_finder.h"

namespace {
using namespace syntax;
using Errors = std::vector<std::string>;

// @brief A stream that adds its content to an error list upon destruction.
// Supports a simple, one-line error reporting syntax like:
// `emit() << "error message";`
struct Emitter : public std::ostringstream {
  Emitter(Errors& v) : errors(v) {}
  ~Emitter() { errors.push_back(std::ostringstream::str()); }
  Errors& errors;
};

// @brief Base class for semantic checkers that operate on the AST.
//
// Provides common utilities needed for semantic analysis, including access to
// the symbol table, a type finder, and an error-reporting stream. Subclasses
// implement `operator()` overloads for the specific AST node types
// they are designed to validate.
struct Checker {
  Emitter emit() { return {errors}; }
  Errors& errors;
  const SymbolTable& symbols;
  TypeFinder& get_type;
};

// @brief Traverses an AST subtree and applies a checker to each node.
//
// Performs a pre-order traversal starting from `root`. Invokes the provided
// `checker` on the current node, then recursively on its children.
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

// Runs several checkers sequentially.
template <class... C>
void CheckBelow(const Expr& root, Errors& errors, const SymbolTable& t,
                TypeFinder& tf) {
  (CheckBelow<C>(root, C{errors, t, tf}), ...);
}

static std::unordered_set<std::string_view> kBuiltinTypes{"int", "string"};

// Record literal field names, expression types, and the order
// thereof must exactly match those of the given record type (2.3)
struct RecordFieldChecker : Checker {
  RecordFieldChecker(Errors& errors, const SymbolTable& symbols, TypeFinder& tf)
      : Checker{errors, symbols, tf} {}
  void operator()(const auto&) {}
  void operator()(const Expr& e) {
    const RecordLiteral* lit = std::get_if<RecordLiteral>(&e);
    if (!lit) return;
    const TypeDeclaration* d = symbols.lookupType(e, lit->type_id);
    while (true) {
      if (!d) {
        emit() << "Unknown record type " << lit->type_id;
        return;
      }
      const auto* alias = std::get_if<Identifier>(&d->value);
      if (!alias) break;
      if (kBuiltinTypes.contains(*alias)) {
        emit() << "Type " << lit->type_id << " is not a record";
        return;
      }
      d = symbols.lookupType(e, *alias);
    }
    const auto* tf = std::get_if<TypeFields>(&d->value);
    if (!tf) {
      emit() << "Type " << lit->type_id << " is not a record";
      return;
    }
    const std::vector<FieldAssignment>& assignments = lit->fields;
    if (assignments.size() != tf->size()) {
      emit() << "Type " << d->id << " has " << tf->size()
             << " fields and literal has " << assignments.size();
      return;
    }
    // > Field names, expression types, and the order thereof must exactly match
    // > those of the given record type.
    for (size_t i = 0; i < tf->size(); ++i) {
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
  }
};

// Binary operators >, <, >=, and <= may be either both integer or both string
// (2.5). Operators & and | are lazy logical operators on integers (2.5)
struct BinaryOpChecker : Checker {
  BinaryOpChecker(Errors& errors, const SymbolTable& symbols, TypeFinder& tf)
      : Checker{errors, symbols, tf} {}
  void operator()(const auto&) {}
  void operator()(const Expr& e) {
    const Binary* b = std::get_if<Binary>(&e);
    if (!b) return;
    switch (b->op) {
      case kGreaterThan:
      case kLessThan:
      case kNotGreaterThan:
      case kNotLessThan:
        CheckComparison(get_type(*b->left), get_type(*b->right), b->op);
        break;
      case kAnd:
      case kOr:
        CheckInt(get_type(*b->left), b->op);
        CheckInt(get_type(*b->right), b->op);
        break;
      default:
        break;
    }
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
    if (!kBuiltinTypes.contains(type)) {
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

  void operator()(const auto&) {}
  void operator()(const Expr& e) {
    if (const auto* it = std::get_if<IfThen>(&e)) {
      CheckInt(*it->condition);
    } else if (const auto* ite = std::get_if<IfThenElse>(&e)) {
      CheckInt(*ite->condition);
    } else if (const auto* w = std::get_if<While>(&e)) {
      CheckInt(*w->condition);
    }
  }

  void CheckInt(const Expr& condition) {
    std::string_view type = get_type(condition);
    if (type != "int") {
      emit() << "Conditions must be int, but got " << type;
    }
  }
};

// Nil may only be used for records with known type (2.7)
struct NilChecker : Checker {
  NilChecker(Errors& errors, const SymbolTable& symbols, TypeFinder& tf)
      : Checker{errors, symbols, tf} {}

  void CheckRecordType(std::string_view type, const Expr& e) {
    const TypeDeclaration* decl = symbols.lookupUnaliasedType(e, type);
    if (decl == nullptr || !std::get_if<TypeFields>(&decl->value)) {
      emit() << "Type " << type << " is not a record type";
    }
  }
  void operator()(const auto&) {}
  void operator()(const Expr& e) {
    const Binary* b = std::get_if<Binary>(&e);
    if (b && std::get_if<Nil>(b->left.get())) {
      CheckRecordType(get_type(*b->right), e);
    }
    if (b && std::get_if<Nil>(b->right.get())) {
      CheckRecordType(get_type(*b->left), e);
    }
    const Assignment* a = std::get_if<Assignment>(&e);
    if (a && std::get_if<Nil>(a->expr.get())) {
      CheckRecordType(get_type.GetLValueType(e, *a->l_value), e);
    }
  }
};
}  // namespace

Errors ListErrors(const Expr& root, const SymbolTable& t, TypeFinder& tf) {
  Errors errors;
  CheckBelow<RecordFieldChecker, BinaryOpChecker, ConditionalChecker,
             NilChecker>(root, errors, t, tf);
  return errors;
}
