#pragma once
#include "Expression.h"
#include <sstream>

template <class T> std::string ToString(const T& item) {
  std::ostringstream out;
  out << item;
  return out.str();
}

// Family of operators
std::ostream& operator<<(std::ostream& os, const Type& t);
std::ostream& operator<<(std::ostream& os, const Expression& e);
std::ostream& operator<<(std::ostream& os, const Declaration& d);
