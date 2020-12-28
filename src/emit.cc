#include "emit.h"
#include <optional>
#include <unordered_map>
namespace emit {
namespace {
class FunctionCodeBlock : public CodeBlock {};
class StringConstant : public Pushable {};
class LibraryFunction : public Invocable {};

const std::unordered_map<std::string_view, const char*>
    kTypeByLibraryFunctionName = {{"print", "(Ljava/lang/String;)V"}};

std::optional<const char*> LibraryFunctionType(std::string_view name) {
  if (auto found = kTypeByLibraryFunctionName.find(name);
      found != kTypeByLibraryFunctionName.end()) {
    return found->second;
  }
  return {};
}
} // namespace

Program::Program() : main_(std::make_unique<FunctionCodeBlock>()) {}

std::unique_ptr<Pushable> Program::DefineStringConstant(std::string_view text) {
  return {}; // std::make_unique<StringConstant>(text);
}
std::unique_ptr<Invocable>
Program::LookupLibraryFunction(std::string_view name) {
  return {};
}

// Writes Java class file to given stream
void Program::Emit(std::ostream& os) {}
} // namespace emit
