#pragma once
#include "TypeVisitor.h"
#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

class Expression;

class DeclarationVisitor {
public:
  virtual bool VisitTypeDeclaration(const std::string& id, const Type& type) {
    return true;
  }
  virtual bool
  VisitVariableDeclaration(const std::string& id,
                           const std::optional<std::string>& type_id,
                           const Expression& expr) {
    return true;
  }
  virtual bool VisitFunctionDeclaration(
      const std::string& id, const std::vector<TypeField>& params,
      const std::optional<std::string> type_id, const Expression& body) {
    return true;
  }
};
