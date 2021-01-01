#pragma once
#include "Expression.h"

// Checks Tiger program constraints statically. Specifically:
// - Record literal field names, expression types, and the order
//   thereof must exactly match those of the given record type (2.3)
// - Binary operators >, <, >=, and <= may be either both integer or
//   both string (2.5)
// - Operators & and | are lazy logical operators on integers (2.5)
// - Nil may only be used for records with known type (2.7)
// - Conditionals must evaluate to integers (2.8)
// - If-then-else branches must be of the same type or both not return
//   a value (2.8)
// - Loop body must not return a value (2.8)
// - For loop variable may not be assigned to (2.8)
// - Break expression is illegal outside loop bodies
// - Mutually recursive type declrations must pass through a record or
//   array type (3.1)
// - The scope of the type declaration begins at the start of the
//   sequence of type declarations (3.1) [Types may be referenced
//   before their declaration, but only within a given declaration
//   sequence]
// - No two defined types in such a sequence may have the same name (3.1)
// - The scope of the variable declaration begins just after the
//   declaration (3.1) [Unlike types and functions, there is no
//   forward reference to variables, even in the same declaration
//   list]
// - (3.2) [NOTE: no explicit prohibition on variable definitions
//   having the same name]
// - Variables and functions share the same name space (3.2) [Maybe
//   restrict: No two definitions of types or variables in a
//   declaration sequence may have the same name]
// - (3.2) [Assigned expression in variable declaration must have a
//   value. Common sense indicates that a declared variable type
//   should match the value type]
// - Functions return a value of the specified type (3.3)
// - No two functions in a declaration sequence may have the same name
//   (3.3)
// - The scope of the function declaration begins at the start of the
//   sequence of function declarations to which it belongs (3.3)
// - (3.3) [Common sense indicates that values of a called function
//   should have compatible types]
