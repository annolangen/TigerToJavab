#pragma once
#include "Expression.h"
#include "TypeVisitor.h"
#include <string>

template <class T> std::string DebugString(const T& item) {
  std::string result;
  return AppendDebugString(result, item);
}

// Family of functions that return their string reference argument.
std::string& AppendDebugString(std::string& out, const Type& t);
std::string& AppendDebugString(std::string& out, const Expression& e);
std::string& AppendDebugString(std::string& out, const Declaration& d);
std::string& AppendDebugString(std::string& out, const TypeField& f);
