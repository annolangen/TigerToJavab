#include "ToString.h"

namespace {
class AppendTypeVisitor : public TypeVisitor {
public:
  AppendTypeVisitor(std::ostream &os) : os_(os) {}
  virtual bool VisitTypeReference(const std::string &id) {
    os_ << id;
    return true;
  }

  virtual bool VisitRecordType(const std::vector<TypeField> fields) {
    const char *sep = "";
    os_ << "{";
    for (const auto &f : fields) {
      os_ << sep << f.id << ": " << f.type_id;
      sep = ", ";
    }
    os_ << "}";
    return true;
  }

  virtual bool VisitArrayType(const std::string &type_id) {
    os_ << "array of " << type_id;
    return true;
  }

private:
  std::ostream &os_;
};

} // namespace

std::ostream &operator<<(std::ostream &os, const Type &t) {
  AppendTypeVisitor visitor(os);
  t.Accept(visitor);
  return os;
}

std::ostream &operator<<(std::ostream &os, const Expression &e) { return os; }

std::ostream &operator<<(std::ostream &os, const Declaration &d) { return os; }
