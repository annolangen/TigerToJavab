# Why Another Tiger Compiler?

There are countless textbook implementations of Andrew Appel's classic Tiger Compiler scattered across GitHub. Most of them share the same architectural blueprints, either closely porting the original C textbook code, or adapting it to standard Object-Oriented C++.

So, why write *another* one? What makes `TigerToJavab` different?

This project was built to explore two primary architectural departures from the standard C++ compiler implementations found online:

## 1. Functional-Style ASTs via `std::variant`

The vast majority of C++ Tiger compilers use a standard Object-Oriented paradigm for their Abstract Syntax Tree (AST): a base class `ASTNode`, with inherited child classes like `IfNode`, `BinOpNode`, and `LetNode`. 

This approach naturally leads to heavy boilerplate. Implementing passes (like semantic checking or code emission) requires sprawling, double-dispatch Visitor patterns that spread compiler logic across massive class hierarchies. This is why the famous EPITA Tiger Compiler (`tc`) and similar projects are highly modular, but notoriously bloated.

`TigerToJavab` takes a completely different approach. It leverages modern C++17 features to emulate the Algebraic Data Types (ADTs) and pattern matching found in functional languages like OCaml or Haskell (which the original textbook often praised). 

Instead of an inheritance tree, the AST is defined compactly using `std::variant`:

```cpp
using Expr = std::variant<StringConstant, IntegerConstant, Nil,
                          std::unique_ptr<LValue>, Negated, Binary, Assignment,
                          FunctionCall, RecordLiteral, ArrayLiteral, IfThen,
                          IfThenElse, While, For, Break, Let, Parenthesized>;
```

AST traversal is handled via C++17 template deduction (`template <class... F> struct Overloaded : F...`) combined with `std::visit`. This leads to an incredibly compact, readable, and elegant AST manipulation footprint that completely side-steps OOP visitor bloat.

## 2. Direct JVM Bytecode Emission in C++

Typically, C/C++ Tiger implementations target either the textbook's MIPS assembly or hook into a massive backend framework like LLVM to generate machine code. Compilers that target the JVM are almost universally written in Java.

`TigerToJavab` bridges this gap natively. Instead of relying on a multi-megabyte backend dependency, the compiler manually unpacks JVM bytecode directly into a binary `.class` file representation.

In `emit.cc`, the compiler:
- Writes the `0xcafebabe` magic number directly.
- Serializes and indexes a dynamic JVM Constant Pool.
- Formats bytecode instructions and method attributes via stream byte-packing (`Put2`, `Put4`).

This results in a lightweight, zero-backend-dependency C++ compiler that generates fully compliant Java bytecode executable by any standard JRE.

## Summary

`TigerToJavab` is an experiment in extreme compactness and mechanical elegance. It demonstrates that by embracing modern C++ standard library features (`std::variant`, `std::unique_ptr`) and writing a bespoke binary emitter, you can implement a full, spec-compliant JVM-targeting Tiger compiler without the enormous directory structures and boilerplate typical of static OOP projects.
