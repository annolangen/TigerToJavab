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
