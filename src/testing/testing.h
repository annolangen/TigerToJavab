#pragma once
#include "../Expression.h"

namespace testing {

std::shared_ptr<Expression> Parse(const std::string& text);
std::shared_ptr<Expression> ParseFile(const std::string& file_name);
} // namespace testing
