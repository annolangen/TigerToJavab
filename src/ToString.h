#pragma once
#include "syntax.h"
#include "syntax_insertion.h"
#include <sstream>

template <class T> std::string ToString(const T& item) {
  std::ostringstream out;
  out << item;
  return out.str();
}
