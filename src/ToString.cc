#include "ToString.h"
#include "util.h"

std::ostream& operator<<(std::ostream& os, const TypeField& f) {
  return os << f.id << ": " << f.type_id;
}

namespace {
using util::join;

class AppendTypeVisitor : public TypeVisitor {
public:
  AppendTypeVisitor(std::ostream& os) : os_(os) {}
  bool VisitTypeReference(const std::string& id) override {
    os_ << id;
    return true;
  }

  bool VisitRecordType(const std::vector<TypeField>& fields) override {
    os_ << "{" << (fields | join(", ")) << "}";
    return true;
  }

  bool VisitArrayType(const std::string& type_id) override {
    os_ << "array of " << type_id;
    return true;
  }

  bool VisitInt() override {
    os_ << "int";
    return true;
  }

  bool VisitString() override {
    os_ << "string";
    return true;
  }

private:
  std::ostream& os_;
};

} // namespace

std::ostream& operator<<(std::ostream& os, const Type& t) {
  AppendTypeVisitor visitor(os);
  t.Accept(visitor);
  return os;
}

std::ostream& operator<<(std::ostream& os, const Expression& e) { return os; }

std::ostream& operator<<(std::ostream& os, const Declaration& d) { return os; }
