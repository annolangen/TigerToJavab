#pragma once
#include "../syntax.h"

namespace testing {

std::unique_ptr<syntax::Expr> Parse(const std::string& text);
std::unique_ptr<syntax::Expr> ParseFile(const std::string& file_name);

// Returns output of executing code in /tmp/Main.class with Std.class in
// classpath.
std::string RunJava();
} // namespace testing
