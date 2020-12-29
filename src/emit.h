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

class Function : public Invocable {
public:
  virtual CodeBlock* codeBlock() = 0;
};

enum Flag {
  ACC_PUBLIC = 0x0001,    // Declared public; may be accessed from
                          // outside its package.
  ACC_FINAL = 0x0010,     // Declared final; no subclasses allowed.
  ACC_SUPER = 0x0020,     // Treat superclass methods specially when
                          // invoked by the invokespecial instruction.
  ACC_INTERFACE = 0x0200, // Is an interface, not a class.
  ACC_ABSTRACT = 0x0400,  // Declared abstract; must not be instantiated.
  ACC_SYNTHETIC = 0x1000, // Declared synthetic; not present in the source code.
  ACC_ANNOTATION = 0x2000, // Declared as an annotation type.
  ACC_ENUM = 0x4000        // Declared as an enum type.
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
  virtual Function* DefineFunction(uint16_t flags, std::string_view name,
                                   std::string_view descriptor) = 0;
};
} // namespace emit
