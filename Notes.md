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
