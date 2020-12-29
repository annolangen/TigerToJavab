#pragma once
#include <memory>
#include <ostream>
#include <sstream>
#include <string_view>
#include <vector>

namespace emit {

// Abstract block of JVM byte codes.
struct CodeBlock {
  virtual ~CodeBlock() = default;
  std::ostringstream bytes;
};

// Things that can emit instructions to push one value on the JVM stack.
class Pushable {
public:
  virtual ~Pushable() = default;
  // Adds instructions to the given code block to push one value on the JVM
  // stack.
  virtual void Push(CodeBlock& block) const = 0;
};

// Defined or standard library functions.
class Invocable {
public:
  virtual ~Invocable() = default;
  virtual void Invoke(CodeBlock& block) const = 0;
  void Call(CodeBlock& block, const std::vector<const Pushable*>& args) const {
    for (const auto& a : args) {
      a->Push(block);
    }
    Invoke(block);
  }
};

struct Program {
  // Returns Program instance for Java class files.
  static std::unique_ptr<Program> JavaProgram();

  virtual ~Program() = default;

  // Returns code block for main method, which is owned by the Program
  virtual CodeBlock* GetMainCodeBlock() = 0;

  // Writes Java class file to given stream
  virtual void Emit(std::ostream& os) = 0;
  virtual const Pushable* DefineStringConstant(std::string_view text) = 0;
  virtual const Invocable* LookupLibraryFunction(std::string_view name) = 0;
};
} // namespace emit
