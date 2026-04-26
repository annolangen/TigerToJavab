#include "checker.h"

#include <algorithm>
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
// NOTE: This performs a DEEP traversal. Use `VisitChildren` internally to
// ensure recursion continues through all node types.
template <class C, class Node>
void CheckBelow(const Node& root, C&& checker) {
  checker(root);
  VisitChildren(root, [&](const auto& child) {
    CheckBelow(child, checker);
    return true;
  });
}

// Runs several checkers sequentially.
template <class... C>
void CheckBelow(const Expr& root, Errors& errors, const SymbolTable& t, TypeFinder& tf) {
  (CheckBelow<C>(root, C{errors, t, tf}), ...);
}

static std::unordered_set<std::string_view> kBuiltinTypes{"int", "string"};

// Record literal field names, expression types, and the order
// thereof must exactly match those of the given record type (2.3)
struct RecordFieldChecker : Checker {
  RecordFieldChecker(Errors& errors, const SymbolTable& symbols, TypeFinder& tf) : Checker{errors, symbols, tf} {}
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
      emit() << "Type " << d->id << " has " << tf->size() << " fields and literal has " << assignments.size();
      return;
    }
    // > Field names, expression types, and the order thereof must exactly match
    // > those of the given record type.
    for (size_t i = 0; i < tf->size(); ++i) {
      if (assignments[i].id != tf->at(i).id) {
        emit() << "Different names " << assignments[i].id << " and " << tf->at(i).id << " for field #" << (i + 1)
               << " of record " << d->id;
      } else if (std::string_view t = get_type(*assignments[i].expr); t != tf->at(i).type_id) {
        emit() << "Different types " << t << " and " << tf->at(i).type_id << " for field #" << (i + 1) << " of record "
               << d->id;
      }
    }
  }
};

// Binary operators >, <, >=, and <= may be either both integer or both string
// (2.5). Operators & and | are lazy logical operators on integers (2.5)
struct BinaryOpChecker : Checker {
  BinaryOpChecker(Errors& errors, const SymbolTable& symbols, TypeFinder& tf) : Checker{errors, symbols, tf} {}

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

  void CheckComparison(std::string_view left_type, std::string_view right_type, BinaryOp op) {
    CheckPrimitive(left_type, op);
    CheckPrimitive(right_type, op);
    if (left_type != right_type) {
      emit() << "Types of " << op << " should match, but got " << left_type << " and " << right_type;
    }
  }
  void CheckPrimitive(std::string_view type, BinaryOp op) {
    if (!kBuiltinTypes.contains(type)) {
      emit() << "Operand type of " << op << " must be int or string, but got " << type;
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
  ConditionalChecker(Errors& errors, const SymbolTable& symbols, TypeFinder& tf) : Checker{errors, symbols, tf} {}

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
  NilChecker(Errors& errors, const SymbolTable& symbols, TypeFinder& tf) : Checker{errors, symbols, tf} {}

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

  void CheckRecordType(std::string_view type, const Expr& e) {
    const TypeDeclaration* decl = symbols.lookupUnaliasedType(e, type);
    if (decl == nullptr || !std::get_if<TypeFields>(&decl->value)) {
      emit() << "Type " << type << " is not a record type";
    }
  }
};

// Function calls must have the same number of arguments as parameters,
// and argument types must match parameter types.
struct FunctionCallChecker : Checker {
  FunctionCallChecker(Errors& errors, const SymbolTable& symbols, TypeFinder& tf) : Checker{errors, symbols, tf} {}

  void operator()(const auto&) {}
  void operator()(const Expr& e) {
    const FunctionCall* fc = std::get_if<FunctionCall>(&e);
    if (!fc) return;

    const FunctionDeclaration* fd = symbols.lookupFunction(e, fc->id);
    if (!fd) return;

    if (fc->arguments.size() != fd->parameter.size()) {
      emit() << "Function " << fc->id << " expects " << fd->parameter.size() << " arguments, but got "
             << fc->arguments.size();
      return;
    }

    for (size_t i = 0; i < fc->arguments.size(); ++i) {
      bool is_nil = std::holds_alternative<Nil>(*fc->arguments[i]);
      std::string_view arg_type = is_nil ? "nil" : get_type(*fc->arguments[i]);
      std::string_view declared_type = fd->parameter[i].type_id;
      std::string_view expected_type = declared_type;

      const TypeDeclaration* td = symbols.lookupType(e, expected_type);
      while (td) {
        if (const auto* alias = std::get_if<Identifier>(&td->value)) {
          expected_type = *alias;
          td = symbols.lookupType(e, expected_type);
        } else {
          break;
        }
      }

      if (arg_type != expected_type && arg_type != "NOTYPE" && arg_type != "nil") {
        emit() << "Argument " << i + 1 << " of function " << fc->id << " expects type " << declared_type << " but got "
               << arg_type;
      } else if (arg_type == "nil") {
        const TypeDeclaration* decl = symbols.lookupUnaliasedType(e, declared_type);
        if (!decl || !std::get_if<TypeFields>(&decl->value)) {
          emit() << "Type " << declared_type << " is not a record type";
        }
      }
    }
  }
};

// - Mutually recursive type declrations must pass through a record or
//   array type (3.1)
// - No two defined types in such a sequence may have the same name (3.1)
// - Assigned expression in variable declaration must have a value (3.2)
// - No two functions in a declaration sequence may have the same name (3.3)
// - Functions return a value of the specified type (3.3)
struct DeclarationChecker : Checker {
  DeclarationChecker(Errors& errors, const SymbolTable& symbols, TypeFinder& tf) : Checker{errors, symbols, tf} {}

  void operator()(const auto&) {}

  void operator()(const Expr& e) {
    if (const auto* l = std::get_if<Let>(&e)) (*this)(*l);
  }

  void operator()(const Declaration& d) {
    std::visit([&](const auto& v) { (*this)(v); }, d);
  }

  void operator()(const Let& l) {
    auto chunks = GroupDeclarations(l.declaration);
    for (const auto& chunk : chunks) {
      if (std::holds_alternative<const TypeDeclaration*>(chunk[0])) {
        CheckTypeGroup(chunk);
      } else if (std::holds_alternative<const FunctionDeclaration*>(chunk[0])) {
        CheckFunctionGroup(chunk);
      }
    }
  }

  void operator()(const VariableDeclaration& v) {
    bool is_nil = std::holds_alternative<Nil>(*v.value);
    std::string_view type = is_nil ? "nil" : get_type(*v.value);

    if (type == "NOTYPE") {
      emit() << "Variable " << v.id << " initialized with no value";
    }
    if (v.type_id) {
      std::string_view declared_type = *v.type_id;
      const TypeDeclaration* td = symbols.lookupType(*v.value, declared_type);
      while (td) {
        if (const auto* alias = std::get_if<Identifier>(&td->value)) {
          declared_type = *alias;
          td = symbols.lookupType(*v.value, declared_type);
        } else {
          break;
        }
      }

      if (type != declared_type && type != "NOTYPE" && type != "nil") {
        // Basic string mismatch check resolved.
        emit() << "Variable " << v.id << " declared type " << *v.type_id << " but initialized with " << type;
      } else {
        if (type == "nil") {
          // Check for nil assignment to record
          const TypeDeclaration* decl = symbols.lookupUnaliasedType(*v.value, *v.type_id);
          if (!decl || !std::get_if<TypeFields>(&decl->value)) {
            emit() << "Nil may only be used for records with known type";
          }
        }
      }
    } else {  // No type declared
      if (type == "nil") {
        emit() << "Nil may only be used for records with known type";
      }
    }
  }

  void operator()(const FunctionDeclaration& f) {
    std::string_view body_type = get_type(*f.body);
    if (f.type_id) {
      // TODO: This uses strict string comparison and does not currently resolve
      // type aliases. If a function returns an alias (e.g., `myint`) but is
      // declared to return the base type (`int`), or vice-versa, this will
      // incorrectly emit an error.
      if (body_type != *f.type_id) {
        emit() << "Function " << f.id << " declared to return " << *f.type_id << " but body returns " << body_type;
      }
    } else {
      if (body_type != "NOTYPE") {
        emit() << "Procedure " << f.id << " body must not return a value";
      }
    }
  }

 private:
  using DeclPtr = std::variant<const TypeDeclaration*, const VariableDeclaration*, const FunctionDeclaration*>;

  std::vector<std::vector<DeclPtr>> GroupDeclarations(const std::vector<std::unique_ptr<Declaration>>& decls) {
    std::vector<std::vector<DeclPtr>> chunks;
    if (decls.empty()) return chunks;

    chunks.push_back({});
    for (const auto& d : decls) {
      DeclPtr current = std::visit([](const auto& x) -> DeclPtr { return &x; }, *d);
      if (chunks.back().empty()) {
        chunks.back().push_back(current);
      } else {
        if (current.index() == chunks.back().back().index() && current.index() != 1) {  // Same type and not Variable(1)
          chunks.back().push_back(current);
        } else {
          chunks.push_back({current});
        }
      }
    }
    return chunks;
  }

  void CheckTypeGroup(const std::vector<DeclPtr>& group) {
    std::unordered_set<std::string_view> names;
    for (const auto& d : group) {
      const auto* td = std::get<const TypeDeclaration*>(d);
      if (!names.insert(td->id).second) {
        emit() << "Duplicate type definition: " << td->id;
      }
    }
    // Cycle check
    for (const auto& d : group) {
      const auto* td = std::get<const TypeDeclaration*>(d);
      if (HasIllegalCycle(td, group)) {
        emit() << "Illegal mutually recursive type cycle for " << td->id;
      }
    }
  }

  bool HasIllegalCycle(const TypeDeclaration* start, const std::vector<DeclPtr>& group) {
    // Valid cycle must pass through Record or Array.
    // Illegal cycle: definitions are just aliases of each other in the group.
    // e.g. type a = b; type b = a;
    // Start BFS/DFS from start. If we hit start again without passing
    // record/array, illegal. Only traverse references within the group.
    std::unordered_set<std::string_view> visited;
    std::vector<const TypeDeclaration*> q;
    q.push_back(start);
    visited.insert(start->id);

    size_t index = 0;
    while (index < q.size()) {
      const TypeDeclaration* curr = q[index++];
      // Check RHS.
      if (const auto* alias = std::get_if<Identifier>(&curr->value)) {
        // Find definition in group
        auto it = std::find_if(group.begin(), group.end(),
                               [&](DeclPtr p) { return std::get<const TypeDeclaration*>(p)->id == *alias; });
        if (it != group.end()) {
          const TypeDeclaration* next = std::get<const TypeDeclaration*>(*it);
          if (next == start) return true;  // Cycle found!
          if (visited.insert(next->id).second) {
            q.push_back(next);
          }
        }
      }
      // If RHS is Record or Array, cycle is broken (valid). Do not traverse
      // further.
    }
    return false;
  }

  void CheckFunctionGroup(const std::vector<DeclPtr>& group) {
    std::unordered_set<std::string_view> names;
    for (const auto& d : group) {
      const auto* fd = std::get<const FunctionDeclaration*>(d);
      if (!names.insert(fd->id).second) {
        emit() << "Duplicate function definition: " << fd->id;
      }
    }
  }
};

// - If-then-else branches must be of the same type or both not return
//   a value (2.8)
// - Loop body must not return a value (2.8)
// - For loop variable may not be assigned to (2.8)
// - Break expression is illegal outside loop bodies
struct StructureChecker {
  StructureChecker(Errors& errors, const SymbolTable& symbols, TypeFinder& tf)
      : errors(errors), symbols(symbols), get_type(tf) {}

  Errors& errors;
  const SymbolTable& symbols;
  TypeFinder& get_type;
  int loops_entered = 0;

  Emitter emit() { return {errors}; }

  void Check(const Expr& e) {
    if (const auto* loop = std::get_if<While>(&e)) {
      if (get_type(*loop->body) != "NOTYPE") {
        emit() << "Loop body must not return a value";
      }
      Check(*loop->condition);
      loops_entered++;
      Check(*loop->body);
      loops_entered--;
      return;
    } else if (const auto* loop = std::get_if<For>(&e)) {
      if (get_type(*loop->body) != "NOTYPE") {
        emit() << "Loop body must not return a value";
      }
      Check(*loop->start);
      Check(*loop->end);
      loops_entered++;
      Check(*loop->body);
      loops_entered--;
      return;
    } else if (const auto* ite = std::get_if<IfThenElse>(&e)) {
      CheckBranchTypes(*ite->then_expr, *ite->else_expr);
    } else if (std::get_if<Break>(&e)) {
      if (loops_entered == 0) {
        emit() << "Break must be inside a loop";
      }
    } else if (const auto* assign = std::get_if<Assignment>(&e)) {
      if (const auto* id = std::get_if<Identifier>(assign->l_value.get())) {
        StorageLocation loc = symbols.lookupStorageLocation(e, *id);
        if (std::holds_alternative<const For*>(loc)) {
          emit() << "For loop variable " << *id << " may not be assigned to";
        }
      }
    }

    // Generic recursion
    VisitChildren(e, [&](const auto& child) {
      Check(child);
      return true;
    });
  }

  template <class T>
  void Check(const T& e) {
    VisitChildren(e, [&](const auto& child) {
      Check(child);
      return true;
    });
  }

  void CheckBranchTypes(const Expr& then_e, const Expr& else_e) {
    if (get_type(then_e) != get_type(else_e)) {
      // If one is void, it's fine? No, "Branches must be of the same type OR
      // both not return a value". "Both not return a value" == Both void. So if
      // both void -> Same type. If one void, one int -> different type ->
      // Error. The rule effectively means: Types must match. (Void is a type).
      // Wait, "or both not return a value" implies void is "no value".
      // Tiger semantics: If expressions must produce a value if used in value
      // context. If `if-then-else` returns void, then both branches must return
      // void. So checking `type != type` is sufficient.
      emit() << "If-then-else branches must have same type";
    }
  }
};

}  // namespace

Errors ListErrors(const Expr& root, const SymbolTable& t, TypeFinder& tf) {
  Errors errors;
  CheckBelow<DeclarationChecker, RecordFieldChecker, BinaryOpChecker, ConditionalChecker, NilChecker,
             FunctionCallChecker>(root, errors, t, tf);
  StructureChecker(errors, t, tf).Check(root);
  return errors;
}
