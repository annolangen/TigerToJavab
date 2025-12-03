# Project Plan for TigerToJavab

This document outlines the milestones and tasks required to complete the `TigerToJavab` compiler, focusing on using modern C++ practices and ensuring a robust, well-tested final product.

## A Note on Target: Java Bytecode vs. LLVM/Clang

The current goal is to compile to **Java Bytecode**. This is a great target because the JVM is a high-level, garbage-collected, and platform-independent virtual machine. This simplifies code generation, as we don't need to manage memory or deal with machine-specific instruction sets.

An alternative, as you mentioned, would be to use the **Clang/LLVM API**.

- **Pros**: LLVM is a powerful, industry-standard compiler infrastructure. It provides mature optimizations and can target multiple native architectures (x86, ARM, etc.).
- **Cons**: The learning curve for the LLVM API is significantly steeper. It's a much lower-level target, requiring manual memory management (or integrating a garbage collector) and handling platform-specific calling conventions.

**Recommendation**: Stick with the Java bytecode target. It aligns with the project's history and presents a more manageable path to a working compiler. The lessons learned can be applied to an LLVM backend later if desired.

---

## Milestone 1: Foundational Setup & AST

_Goal: Establish a modern C++ environment and build a complete Abstract Syntax Tree (AST)._

- [x] **Task 1.1: Modernize CMake**

  - Review and refactor `CMakeLists.txt` to use modern CMake functions (`target_link_libraries`, `target_include_directories`, etc.).
  - Set the C++ standard to C++17 or C++20.
  - Enable warnings (`-Wall`, `-Wextra`, `-Wpedantic`) to enforce code quality.

- [x] **Task 1.2: Define the AST**

  - Design a class hierarchy for all Tiger language constructs (expressions, declarations, types, etc.).
  - Use modern C++ features: `std::unique_ptr` for tree ownership, `std::vector` for lists of nodes, and `std::variant` or `std::optional` where appropriate.

- [x] **Task 1.3: Integrate AST with Parser**

- [x] **Task 1.4: Implement an AST Printer**

## Milestone 2: Semantic Analysis

_Goal: Type-check the program and build a symbol table._

- [x] **Task 2.1: Design Symbol Table**

  - Implement a `SymbolTable` class that can manage scopes. It should map identifiers (strings) to their definitions (variable type, function signature, etc.).
  - The table must support entering and exiting scopes to handle nested functions and `let` expressions.

- [ ] **Task 2.2: Implement Semantic Checker (Visitor)**
  - [x] Create a visitor that traverses the AST (`checker.cc`).
  - [x] Check record literal field names and types. (`test_record_lit_field_type_mismatch`)
  - [x] Check binary operator type constraints (`<`, `>`, `&`, `|`). (`test_op_type_mismatch`)
  - [x] Check that `if`/`while` conditions are integers. (`test_if_cond_type_mismatch`, `test_while_cond_type_mismatch`)
  - [x] **Check type matching for variable declarations and assignments.** (`test_var_decl_type_mismatch`, `test_assign_type_mismatch`)
  - [ ] **Check function calls:** Verify argument count and types match the function signature.
  - [ ] **Check `if-then-else`:** Ensure `then` and `else` branches have compatible types.
  - [ ] **Check loop constraints:**
    - [ ] Loop body must not produce a value.
    - [ ] `for` loop variable is read-only.
    - [ ] `break` is only used inside a loop.
  - [x] **Check `nil` usage:** `nil` is only assignable to record types. (`test_nil_assign_to_int`)
  - [ ] **Check for duplicate definitions** in the same scope.
  - [ ] **Check for illegal recursive type declarations** (cycles not involving a record/array).

## Milestone 3: Java Bytecode Generation

_Goal: Translate the validated AST into `.class` files._

- [ ] **Task 3.1: Choose a Bytecode Library**

  - Research and select a C++ library for generating Java `.class` files. A simple library that helps write the binary format would be ideal.
  - If no suitable library exists, the alternative is to generate a textual assembly format like Jasmin and use an assembler (`jasmin.jar`) to create the `.class` file. This is often easier.

- [ ] **Task 3.2: Implement Code Generation Visitor**

  - Create a final visitor that traverses the semantically-checked AST.
  - For each AST node, emit the corresponding sequence of Java bytecode instructions.
  - Manage the JVM's operand stack and local variable array.
  - Translate Tiger's standard library functions (e.g., `print`, `getchar`) into calls to appropriate Java methods (e.g., `System.out.println`).

- [ ] **Task 3.3: End-to-End Testing**
  - Expand the test suite (`make test`) to compile Tiger source files and execute the resulting `.class` files with a Java runtime.
  - Compare the program's output against expected output to verify correctness.

## Milestone 4: Advanced Features & Refinements

_Goal: Polish the compiler and add advanced language features._

- [ ] **Task 4.1**: Implement Garbage Collection integration (if targeting native code, less relevant for JVM).
- [ ] **Task 4.2**: Implement Escape Analysis.
- [ ] **Task 4.3**: Add basic optimizations (e.g., constant folding).
