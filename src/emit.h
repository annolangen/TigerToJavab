#pragma once
#include <memory>
#include <ostream>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace emit {

// Abstract block of JVM byte codes.
class CodeBlock {
public:
  virtual ~CodeBlock() = default;
};

// Things that can emit instructions to push one value on the JVM stack.
class Pushable {
public:
  virtual ~Pushable() = default;
  // Adds instructions to the given code block to push one value on the JVM
  // stack.
  virtual void Push(CodeBlock& block) const = 0;
};

struct Constant {
  virtual ~Constant() = default;
};

// Defined or standard library functions.
class Invocable {
public:
  virtual ~Invocable() = default;
  virtual void Invoke(CodeBlock& block) = 0;
  void Call(CodeBlock& block, const std::vector<const Pushable*>& args) {
    for (const auto& a : args) {
      a->Push(block);
    }
    Invoke(block);
  }
};

class Program {
public:
  Program();
  Program(const Program&) = delete;

  std::unique_ptr<Pushable> DefineStringConstant(std::string_view text);
  std::unique_ptr<Invocable> LookupLibraryFunction(std::string_view name);

  // Returns code block for main method, which is owned by the Program
  CodeBlock* GetMainCodeBlock() { return main_.get(); }

  // Writes Java class file to given stream
  void Emit(std::ostream& os);

private:
  std::unique_ptr<CodeBlock> main_;
  std::vector<std::unique_ptr<Constant>> constant_pool_;
  std::unordered_map<std::string, int> pool_index_by_string_;
  std::unordered_map<int, int> pool_index_by_int_;
};
} // namespace emit
