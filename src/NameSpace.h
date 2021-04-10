#pragma once
#include <optional>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

class Declaration;
std::ostream& operator<<(std::ostream&, const Declaration&);

class NameSpace {
public:
  NameSpace() = default;
  NameSpace(const NameSpace& next) : next_(&next) {}
  using D = const Declaration*;

  // Adds or overwrites a new entry in the current scope
  D& operator[](const std::string& key) { return scope_[key]; }

  // Returns bound value from latest scope where the given key is bound.
  std::optional<D> Lookup(const std::string& key) const {
    for (const NameSpace* i = this; i != nullptr; i = i->next_) {
      if (auto p = i->scope_.find(key); p != i->scope_.end()) return p->second;
    }
    return {};
  }

  // Returns true if the given key is bound in the current scope,
  bool IsBound(const std::string& key) { return scope_.count(key) > 0; }

  std::ostream& AppendString(std::ostream& os) const {
    os << "{";
    const char* sep = "";
    for (const NameSpace* i = this; i != nullptr; i = i->next_) {
      for (const auto& p : i->scope_) {
        os << sep << p.first << ": " << *p.second;
        sep = ",\n";
      }
    }
    return os << "}";
  }

private:
  const NameSpace* next_ = nullptr;
  std::unordered_map<std::string, D> scope_;
};

inline std::ostream& operator<<(std::ostream& os, const NameSpace& name_space) {
  return name_space.AppendString(os);
}
