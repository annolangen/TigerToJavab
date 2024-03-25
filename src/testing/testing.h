#pragma once
#include "../driver.h"
#include "../syntax.h"

namespace testing {

std::unique_ptr<syntax::Expr> Parse(std::string_view text,
                                    DriverOptions options = {});
std::unique_ptr<syntax::Expr> ParseFile(const std::string& file_name,
                                        DriverOptions options = {});

// Returns output of executing code in /tmp/Main.class with Std.class in
// classpath.
std::string RunJava();
}  // namespace testing
