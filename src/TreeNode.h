#pragma once
#include <optional>
#include <vector>

class Expression;
class Declaration;
class NameSpace;
// Base class for abstract syntax tree, which is shared by Expression
// and Declaration.
class TreeNode {
public:
  virtual std::vector<TreeNode*> Children() {
    static std::vector<TreeNode*> kNoChildren;
    return kNoChildren;
  }

  // Returns new scope for a Let expression
  virtual std::optional<const NameSpace*>
  GetTypeNameSpace(const NameSpace& types) const {
    return {};
  }

  // Returns new scope for a Let expression or FunctionDeclaration.
  virtual std::optional<const NameSpace*>
  GetNonTypeNameSpace(const NameSpace& non_types) const {
    return {};
  }

  // Returns this, if an Expression
  virtual std::optional<Expression*> expression() { return {}; }

  // Returns this, if a Declaration
  virtual std::optional<Declaration*> declaration() { return {}; }

protected:
  virtual void SetNameSpacesBelow(const NameSpace* types,
                                  const NameSpace* non_types) {
    if (auto n = GetTypeNameSpace(*types); n) types = *n;
    if (auto n = GetNonTypeNameSpace(*non_types); n) non_types = *n;
    for (auto c : Children()) c->SetNameSpacesBelow(types, non_types);
  }
};
