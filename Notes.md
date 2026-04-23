# Notes

### What is type-id?

The [Tiger spec](http://www.cs.columbia.edu/~sedwards/classes/2002/w4115/tiger.pdf) uses distinct identifier tokens type-id and id.
With distinct identifier tokens the generated one token lookahead parser has no conflicts.
There is one shift/shift conflict when identifier tokens are conflated. It is "ID [", which can be the start of an array element reference l-value or an array literal expression.
The cheap fix that sort of works is to globally register identifiers in type declarations so that the lexer can report subsequent references as type-id.
A proper fix would model scope visibility rules.

More work would be required to produce better error messages when the type of an array literal is mistyped:

```
let
  type Pont = array of int
  var center: Point := Point [3] of 0
in printi(center[2]) end
```

This example will fail to parse unexpected keyword "of", because "Point[3]" is parsed as an l-value.
Should the grammar be refactored using a new token to capture "id '[' expr ']'"?

## Warmup - Compiling to Java

- Array literals use Arrays.fill
- built-in functions
  - print as System.out.print
  - printi as System.out.print
- Identifiers must be escaped with \_ to avoid Java keywords
- Implement scopes by passing all accessed variables as parameters
- Functions can be static methods of a main class

### Insights from Scope and Java Generation

- **Scope Resolution:** The `SymbolTable` API `getScope(expr)` returns the enclosing scope for a node (the scope the expression is evaluated *in*), not the scope it creates. To retrieve the new scope created by a `Let`, `For`, or `FunctionDeclaration` expression, one must query the scope of its body: `getScope(*expr.body)` (or `expr.body[0]`).
- **AST VisitChildren Behavior:** `VisitChildren` recursively traverses the children of a node. Because `Expr` is a `std::variant`, `VisitChildren(expr)` iterates over its children without invoking `operator()` on the variant member object (e.g., `For`) itself. To ensure a variant member creates its scope in `StBuilder`, `std::visit(*this, v)` must be explicitly called or intercepted correctly. Because of this, `For` loop variables currently don't generate dedicated SymbolTable scopes. In `java_source.cc`, it's actually simpler to rely on standard Java block scoping for `For` loop variables instead of manually generating a `ScopeX` class for them.
- **Root Main Scope:** The Tiger code's standard library functions (like `print` and `flush`) are injected into the outermost scope (`Scope0`). Thus, the first `Let` expression in the program doesn't create `Scope0`, but creates `Scope1` inside it. We handle this seamlessly by manually instantiating `Scope0` at the start of `main()`.
