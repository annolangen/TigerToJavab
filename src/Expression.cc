#include "ToString.h"
#include "syntax_nodes.h"
#include <iostream>
namespace {

NameSpace kBuiltInTypes;
Declaration* kTypeDecls[2] = {new TypeDeclaration("int", new IntType()),
                              new TypeDeclaration("string", new StringType())};

NameSpace kBuiltInFunctions;
void AddDecl(const FunctionDeclaration* f) { kBuiltInFunctions[f->Id()] = f; }

class BuiltInBody : public Expression {
public:
  bool Accept(ExpressionVisitor&) const { return true; }
};
void AddProc(std::string_view id, std::vector<TypeField> params) {
  AddDecl(new FunctionDeclaration(id, std::move(params), new BuiltInBody()));
}
void AddFun(std::string_view id, std::vector<TypeField> params,
            std::string_view type_id) {
  AddDecl(new FunctionDeclaration(id, std::move(params), type_id,
                                  new BuiltInBody()));
}

// Type of expressions that lack a value, e.g. the `break` expression.
std::string kNoneType = "none";
// Marker value when type of expression could not be inferred, e.g. undefined
// variable reference.
std::string kUnknownType = "???";

std::string kUnsetType = "unset";

} // namespace
struct TreeNameSpaceSetter : SyntaxTreeVisitor {
  TreeNameSpaceSetter(const NameSpace& types, const NameSpace& non_types)
      : types_(types), non_types_(non_types) {}
  const NameSpace& types_;
  const NameSpace& non_types_;
  bool VisitNode(Expression& child) override {
    child.SetNameSpaces(types_, non_types_);
    return true;
  }
};

// Sets inferred_type for all nodex with values, except for Nil. Nil requires a
// separate visitor. This is used in combination with a SyntaxTreeVisitor, which
// should ensure that types of all child expression have already been set.
// Refrains from type checking.
struct TypeSetter : public ExpressionVisitor, LValueVisitor {
  TypeSetter(const Expression& expr) : expr_(expr) {}
  virtual bool VisitStringConstant(const std::string& text) {
    return SetType(kTypeDecls[1]->Id());
  }
  virtual bool VisitIntegerConstant(int value) {
    return SetType(kTypeDecls[0]->Id());
  }
  // Nil requires a more complex traversal
  virtual bool VisitNil() { return false; }
  virtual bool VisitLValue(const LValue& value) {
    return SetType(value.GetType());
  }
  virtual bool VisitNegated(const Expression& value) {
    return SetType(value.GetType());
  }
  virtual bool VisitBinary(const Expression& left, BinaryOp op,
                           const Expression& right) {
    return SetType(right.GetType());
  }
  virtual bool VisitAssignment(const LValue& value, const Expression& expr) {
    return SetType(kNoneType);
  }
  virtual bool
  VisitFunctionCall(const std::string& id,
                    const std::vector<std::shared_ptr<Expression>>& args) {
    if (auto d = expr_.non_types_->Lookup(id); d) {
      if (auto vt = (*d)->GetValueType(); vt) {
        return SetType(**vt);
      }
    }
    return SetType(kUnknownType);
  }
  virtual bool
  VisitBlock(const std::vector<std::shared_ptr<Expression>>& exprs) {
    return SetType(exprs.empty() ? kNoneType : (*exprs.rbegin())->GetType());
  }
  virtual bool VisitRecord(const std::string& type_id,
                           const std::vector<FieldValue>& field_values) {
    return SetType(type_id);
  }
  virtual bool VisitArray(const std::string& type_id, const Expression& size,
                          const Expression& value) {
    return SetType(type_id);
  }
  virtual bool VisitIfThen(const Expression& condition,
                           const Expression& expr) {
    return SetType(kNoneType);
  }
  virtual bool VisitIfThenElse(const Expression& condition,
                               const Expression& then_expr,
                               const Expression& else_expr) {
    return SetType(then_expr.GetType());
  }
  virtual bool VisitWhile(const Expression& condition, const Expression& body) {
    return SetType(kNoneType);
  }
  virtual bool VisitFor(const std::string& id, const Expression& first,
                        const Expression& last, const Expression& body) {
    return SetType(kNoneType);
  }
  virtual bool VisitBreak() { return SetType(kNoneType); }
  virtual bool
  VisitLet(const std::vector<std::shared_ptr<Declaration>>& declarations,
           const std::vector<std::shared_ptr<Expression>>& body) {
    return SetType(body.empty() ? kNoneType : (*body.rbegin())->GetType());
  }
  virtual bool VisitId(const std::string& id) {
    if (auto found = expr_.non_types_->Lookup(id); found) {
      std::cout << "found " << id << std::endl;
      return SetType(**(*found)->GetValueType());
    }
    std::cout << "not found " << id << " in " << *expr_.non_types_ << std::endl;
    return SetType(kUnknownType);
  }
  virtual bool VisitField(const LValue& value, const std::string& id) {
    if (auto d = expr_.types_->Lookup(value.GetType()); d) {
      if (auto type = (*d)->GetType(); type) {
        if (auto field_type = (*type)->GetFieldType(id); field_type) {
          return SetType(**field_type);
        }
      }
    }
    return SetType(kUnknownType);
  }
  virtual bool VisitIndex(const LValue& value, const Expression& expr) {
    if (auto d = expr_.types_->Lookup(value.GetType()); d) {
      if (auto type = (*d)->GetType(); type) {
        if (auto element_type = (*type)->GetElementType(); element_type) {
          return SetType(**element_type);
        }
      }
    }
    return SetType(kUnknownType);
  }
  bool SetType(const std::string& type) {
    expr_.type_ = &type;
    return false;
  }
  const Expression& expr_;
};

struct TreeTypeSetter : SyntaxTreeVisitor {
  virtual bool VisitNode(Expression& e) {
    TypeSetter setter(e);
    e.Accept(setter);
    std::cout << "Set type on children of " << e << std::endl;
    return true;
  }
};

void Expression::SetNameSpaces(const NameSpace& types,
                               const NameSpace& non_types) {
  types_ = &types;
  non_types_ = &non_types;
  TreeNameSpaceSetter visitor(types, non_types);
  Accept(visitor);
}

void Expression::SetNameSpacesBelow(Expression& root) {
  for (auto* d : kTypeDecls) kBuiltInTypes[d->Id()] = d;
  AddProc("print", {{"s", "string"}});
  AddProc("print", {{"i", "int"}});
  AddProc("flush", {});
  AddFun("getchar", {}, "string");
  AddFun("ord", {{"s", "string"}}, "int");
  AddFun("chr", {{"i", "int"}}, "string");
  AddFun("size", {{"s", "string"}}, "int");
  AddFun("substring", {{"s", "string"}, {"f", "int"}, {"n", "int"}}, "string");
  AddFun("concat", {{"s1", "string"}, {"s2", "string"}}, "string");
  AddFun("not", {{"i", "int"}}, "int");
  AddProc("exit", {{"i", "int"}});
  root.SetNameSpaces(kBuiltInTypes, kBuiltInFunctions);
}

void Expression::SetTypesBelow(Expression& root) {
  TreeTypeSetter children_setter;
  root.Accept(children_setter);
  TypeSetter root_setter(root);
  root.Accept(root_setter);
}

Expression::Expression() : type_(&kUnsetType) {}
