#pragma once

struct SyntaxTreeVisitor {
  virtual ~SyntaxTreeVisitor() = default;
  virtual bool VisitNode(Expression& e) = 0;
};
