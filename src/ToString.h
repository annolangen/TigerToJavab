#pragma once
#include "Expression.h"
#include <sstream>

template <class T> std::string ToString(const T& item) {
  std::ostringstream out;
  out << item;
  return out.str();
}

// Family of operators
std::ostream& operator<<(std::ostream& os, const Type& e);
std::ostream& operator<<(std::ostream& os, const Expression& t);
std::ostream& operator<<(std::ostream& os, const Declaration& d);
