#pragma once

#include <sstream>

#include "syntax.h"

struct DebugStringOptions {
  std::string indent = "  ";

  // If non-null, assigns unique IDs to expressions and prints them.
  std::unordered_map<const syntax::Expr*, int>* id_by_expr = nullptr;
};

template <class T>
std::string DebugString(const T& item, DebugStringOptions options = {}) {
  std::ostringstream out;
  AppendDebugString(item, out, options);
  return out.str();
}

// Family of functions that return their stream reference argument.
std::ostream& AppendDebugString(const syntax::Declaration& d, std::ostream& out,
                                DebugStringOptions options = {});
std::ostream& AppendDebugString(const syntax::Expr& e, std::ostream& out,
                                DebugStringOptions options = {});
std::ostream& AppendDebugString(const syntax::LValue& v, std::ostream& out,
                                DebugStringOptions options = {});
std::ostream& AppendDebugString(const syntax::Type& t, std::ostream& out,
                                DebugStringOptions options = {});
