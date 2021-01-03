#pragma once

class Expression;
class Declaration;
struct SyntaxTreeVisitor {
  virtual ~SyntaxTreeVisitor() = default;
  virtual bool BeforeChildren(Expression& parent) { return true; }
  virtual bool BeforeChildren(Declaration& parent) { return true; }
  virtual bool VisitChild(Expression& child) = 0;
  virtual bool AfterChildren(Expression& parent) { return true; }
  virtual bool AfterChildren(Declaration& parent) { return true; }
};
