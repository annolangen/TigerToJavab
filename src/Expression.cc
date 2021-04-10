#include "DebugString.h"
#include "StoppingExpressionVisitor.h"
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
  bool Accept(ExpressionVisitor&) const override { return true; }
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

struct RecordTypeClassifier : public TypeVisitor {
  bool VisitRecordType(const std::vector<TypeField>& fields) override {
    is_record_type = true;
    return false;
  }

  bool is_record_type = false;
};

bool IsRecordType(const Type& type) {
  RecordTypeClassifier classifier;
  type.Accept(classifier);
  return classifier.is_record_type;
}

struct NilTypeClassifier : public StoppingExpressionVisitor {
public:
  virtual bool VisitNil() {
    is_nil = true;
    return false;
  }
  bool is_nil = false;
};

bool IsNil(const Expression& e) {
  NilTypeClassifier classifier;
  e.Accept(classifier);
  return classifier.is_nil;
}

} // namespace

// TODO deal with Nil!

// Sets type_ for all Expression nodes with values, except for
// Nil. Assumes that types of all child expression have already been
// set.  Refrains from type checking.
struct TypeSetter : public ExpressionVisitor, LValueVisitor {
  TypeSetter(const Expression& expr) : expr_(expr) {}
  bool VisitStringConstant(const std::string& text) override {
    return SetType(kTypeDecls[1]->Id());
  }
  bool VisitIntegerConstant(int value) override {
    return SetType(kTypeDecls[0]->Id());
  }
  // Nil requires a more complex traversal
  bool VisitNil() override { return false; }
  bool VisitLValue(const LValue& value) override {
    value.Accept(*(LValueVisitor*)this);
    return false;
  }
  bool VisitNegated(const Expression& value) override {
    return SetType(value.GetType());
  }
  bool VisitBinary(const Expression& left, BinaryOp op,
                   const Expression& right) override {
    return SetType(right.GetType());
  }
  bool VisitAssignment(const LValue& value, const Expression& expr) override {
    // Type nil right hand side, if left hand side is record with known type
    if (auto r = RecordType(value); r && IsNil(expr)) expr.type_ = *r;
    return SetType(kNoneType);
  }
  bool VisitFunctionCall(const std::string& id,
                         const std::vector<std::shared_ptr<Expression>>& args,
                         const Expression& exp) override {
    if (auto d = expr_.non_types_->Lookup(id); d) {
      if (auto vt = (*d)->GetValueType(); vt) return SetType(**vt);
    }
    return SetType(kUnknownType);
  }
  bool
  VisitBlock(const std::vector<std::shared_ptr<Expression>>& exprs) override {
    return SetType(exprs.empty() ? kNoneType : (*exprs.rbegin())->GetType());
  }
  bool VisitRecord(const std::string& type_id,
                   const std::vector<FieldValue>& field_values,
                   const Expression& exp) override {
    return SetType(type_id);
  }
  bool VisitArray(const std::string& type_id, const Expression& size,
                  const Expression& value) override {
    return SetType(type_id);
  }
  bool VisitIfThen(const Expression& condition,
                   const Expression& expr) override {
    return SetType(kNoneType);
  }
  bool VisitIfThenElse(const Expression& condition, const Expression& then_expr,
                       const Expression& else_expr) override {
    return SetType(then_expr.GetType());
  }
  bool VisitWhile(const Expression& condition,
                  const Expression& body) override {
    return SetType(kNoneType);
  }
  bool VisitFor(const std::string& id, const Expression& first,
                const Expression& last, const Expression& body) override {
    return SetType(kNoneType);
  }
  bool VisitBreak() override { return SetType(kNoneType); }
  bool VisitLet(const std::vector<std::shared_ptr<Declaration>>& declarations,
                const std::vector<std::shared_ptr<Expression>>& body) override {
    return SetType(body.empty() ? kNoneType : (*body.rbegin())->GetType());
  }
  bool VisitId(const std::string& id) override {
    if (auto found = expr_.non_types_->Lookup(id); found) {
      return SetType(**(*found)->GetValueType());
    }
    return SetType(kUnknownType);
  }
  bool VisitField(const LValue& value, const std::string& id) override {
    if (auto d = expr_.types_->Lookup(value.GetType()); d) {
      if (auto type = (*d)->GetType(); type) {
        if (auto field_type = (*type)->GetFieldType(id); field_type) {
          return SetType(**field_type);
        }
      }
    }
    return SetType(kUnknownType);
  }
  bool VisitIndex(const LValue& value, const Expression& expr) override {
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

  // Returns name of record type, if the given child expression has one.
  std::optional<const std::string*> RecordType(const Expression& child) {
    const std::string& n = child.GetType();
    if (auto d = child.types_->Lookup(n); d) {
      if (IsRecordType(**(*d)->GetType())) return &n;
    } else {
      std::cerr << "Undefined type " << n << std::endl;
    }
    return {};
  }
  const Expression& expr_;
};

void Expression::SetNameSpacesBelow(Expression& root) {
  for (auto* d : kTypeDecls) kBuiltInTypes[d->Id()] = d;
  AddProc("print", {{"s", "string"}});
  AddProc("printi", {{"i", "int"}});
  AddProc("flush", {});
  AddFun("getchar", {}, "string");
  AddFun("ord", {{"s", "string"}}, "int");
  AddFun("chr", {{"i", "int"}}, "string");
  AddFun("size", {{"s", "string"}}, "int");
  AddFun("substring", {{"s", "string"}, {"f", "int"}, {"n", "int"}}, "string");
  AddFun("concat", {{"s1", "string"}, {"s2", "string"}}, "string");
  AddFun("not", {{"i", "int"}}, "int");
  AddProc("exit", {{"i", "int"}});
  root.SetNameSpacesBelow(&kBuiltInTypes, &kBuiltInFunctions);
}

void Expression::SetTypesBelow(TreeNode& root) {
  for (auto c : root.Children()) SetTypesBelow(*c);
  if (auto e = root.expression(); e) {
    TypeSetter setter(**e);
    (*e)->Accept(setter);
  }
}

Expression::Expression() : type_(&kUnsetType) {}
