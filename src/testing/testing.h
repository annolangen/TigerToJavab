#pragma once
#include "../Expression.h"

namespace testing {

std::shared_ptr<Expression> Parse(const std::string& text);
std::shared_ptr<Expression> ParseFile(const std::string& file_name);

// Returns output of executing code in /tmp/Main.class with Std.class in
// classpath.
std::string RunJava();
} // namespace testing
