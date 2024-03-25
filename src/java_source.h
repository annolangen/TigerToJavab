#pragma once

#include <string>
#include <vector>

#include "symbol_table.h"
#include "syntax.h"
#include "type_finder.h"

namespace java {

std::string Compile(const syntax::Expr& expr, const SymbolTable& t,
                    TypeFinder& tf, std::string_view class_name = "Main");

}  // namespace java