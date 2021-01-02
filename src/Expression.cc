#include "syntax_nodes.h"
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

std::string kNoneType = "none";

// Sets inferred_type for all nodex with values, except for Nil. Nil requires a
// separate visitor. This is used in combination with a SyntaxTreeVisitor, which
// should ensure that types of all child expression have already been set.
struct TypeSetter : public ExpressionVisitor, LValueVisitor {
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
    return SetType(expr.GetType());
  }
  virtual bool
  VisitFunctionCall(const std::string& id,
                    const std::vector<std::shared_ptr<Expression>>& args) {
    return std::all_of(args.begin(), args.end(),
                       [this](const auto& arg) { return arg->Accept(*this); });
  }
  virtual bool
  VisitBlock(const std::vector<std::shared_ptr<Expression>>& exprs) {
    return std::all_of(exprs.begin(), exprs.end(),
                       [this](const auto& arg) { return arg->Accept(*this); });
  }
  virtual bool VisitRecord(const std::string& type_id,
                           const std::vector<FieldValue>& field_values) {
    return true;
  }
  virtual bool VisitArray(const std::string& type_id, const Expression& size,
                          const Expression& value) {
    return size.Accept(*this) && value.Accept(*this);
  }
  virtual bool VisitIfThen(const Expression& condition,
                           const Expression& expr) {
    return condition.Accept(*this) && expr.Accept(*this);
  }
  virtual bool VisitIfThenElse(const Expression& condition,
                               const Expression& then_expr,
                               const Expression& else_expr) {
    return condition.Accept(*this) && then_expr.Accept(*this) &&
           else_expr.Accept(*this);
  }
  virtual bool VisitWhile(const Expression& condition, const Expression& body) {
    return condition.Accept(*this) && body.Accept(*this);
  }
  virtual bool VisitFor(const std::string& id, const Expression& first,
                        const Expression& last, const Expression& body) {
    return first.Accept(*this) && last.Accept(*this) && body.Accept(*this);
  }
  virtual bool VisitBreak() { return true; }
  virtual bool
  VisitLet(const std::vector<std::shared_ptr<Declaration>>& declarations,
           const std::vector<std::shared_ptr<Expression>>& body) {
    return std::all_of(body.begin(), body.end(),
                       [this](const auto& arg) { return arg->Accept(*this); });
  }
  virtual bool VisitId(const std::string& id) {
    if (auto found = expr.non_types_->Lookup(id); found) {
      return SetType(**(*found)->GetValueType());
    }
    return false;
  }
  virtual bool VisitField(const LValue& value, const std::string& id) {
    return false;
  }
  virtual bool VisitIndex(const LValue& value, const Expression& expr) {
    return false;
  }
  bool SetType(const std::string& type) {
    inferred_type = &type;
    return false;
  }
  const std::string* inferred_type = &kNoneType;
  const Expression& expr;
};
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

void SetTypesBelow(Expression& root) {}
