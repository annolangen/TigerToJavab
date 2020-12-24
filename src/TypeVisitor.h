#pragma once
#include <vector>

struct TypeField {
  std::string id;
  std::string type_id;
};

class Type;
class TypeVisitor {
public:
  virtual bool VisitTypeReference(const std::string& id) { return true; }
  virtual bool VisitRecordType(const std::vector<TypeField>& fields) {
    return true;
  }
  virtual bool VisitArrayType(const std::string& type_id) { return true; }
  virtual bool VisitInt() { return true; }
  virtual bool VisitString() { return true; }
};
