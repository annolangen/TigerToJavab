#pragma once

#include "syntax.h"

struct DebugStringOptions {
  std::string indent = "  ";

  // If non-null, assigns unique IDs to expressions and prints them.
  std::unordered_map<const syntax::Expr*, int>* id_by_expr = nullptr;
};

template <class T>
std::string DebugString(const T& item, DebugStringOptions options = {}) {
  std::string result;
  return AppendDebugString(result, item, options);
}

// Family of functions that return their string reference argument.
std::string& AppendDebugString(std::string& out, const syntax::Expr& t,
                               DebugStringOptions options = {});
