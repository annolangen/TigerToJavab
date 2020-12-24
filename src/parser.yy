%skeleton "lalr1.cc" /* -*- C++ -*- */
%require "3.0.4"
%defines
%define parser_class_name {Parser}
%define api.token.constructor
%define api.value.type variant
%define parse.assert
%code requires
{
# include <string>
#include "syntax_nodes.h"
class Driver;
}
// The parsing context.
%param { Driver& driver }
%locations
%initial-action
{
  // Initialize the initial location.
  @$.begin.filename = @$.end.filename = &driver.file;
};
%define parse.trace
%define parse.error verbose
%code
{
#include "driver.h"
inline void AppendFieldValue(const std::string& id, Expression* expr,
                             std::vector<FieldValue>& out) {
  out.emplace_back();
  auto i = out.rbegin();
  i->id = id;
  i->expr.reset(expr);
}
}
%define api.token.prefix {TOK_}
%token
  EOF  0  "end of file"
  ASSIGN  ":="
  MINUS   "-"
  PLUS    "+"
  STAR    "*"
  SLASH   "/"
  LPAREN  "("
  RPAREN  ")"
  COMMA   ","
  COLON   ":"
  EQUAL   "="
  ARRAY	  "array"
  BREAK	  "break"
  DO	  "do"
  ELSE	  "else"
  END	  "end"
  FOR	  "for"
  FUNCTION	  "function"
  IF	  "if"
  LET	  "let"
  NIL	  "nil"
  OF	  "of"
  THEN	  "then"
  TO	  "to"
  TYPE	  "type"
  VAR	  "var"
  WHILE	  "while"
;
%token <std::string> IDENTIFIER "identifier"
%token <std::string> STRING_CONSTANT "string"
%token <int> NUMBER "number"
%type  <Expression*> expr
%type  <LValue*> l_value
%type  <std::vector<std::shared_ptr<Expression>>> expr_list expr_list_opt expr_seq expr_seq_opt
%type  <std::vector<FieldValue>> field_list field_list_opt
%%
%start unit;
unit: expr  { driver.result.reset($1); };

l_value: "identifier" { $$ = new IdLValue($1); };

expr_list:
expr {$$.emplace_back($1);}
| expr_list "," expr { $1.emplace_back($3); $$ = std::move($1); }
;
expr_list_opt: %empty {} | expr_list {$$ = std::move($1);};
expr_seq:
expr {$$.emplace_back($1);}
| expr_seq ";" expr { $1.emplace_back($3); $$ = std::move($1); }
;
expr_seq_opt: %empty {} | expr_seq {$$ = std::move($1);};
field_list:
"identifier" "=" expr {AppendFieldValue($1, $3, $$);}
| field_list "," "identifier" "=" expr {AppendFieldValue($3, $5, $1);}
;
field_list_opt: %empty {} | field_list {$$ = std::move($1);};
%left ":=";
%left "+" "-";
%left "*" "/";
expr:
  "string"    { $$ = new StringConstant($1);}
| "number"    { $$ = new IntegerConstant($1); }
| "nil"       { $$ = new Nil(); }
| "-" expr     { $$ = new Negated($2); }
| expr "+" expr { $$ = new Binary($1, BinaryOp::kPlus, $3); }
| l_value ":=" expr { $$ = new Assignment(std::shared_ptr<LValue>($1), $3); }
| "identifier" "(" expr_list_opt ")" {$$ = new FunctionCall($1, std::move($3));}
| "(" expr_seq_opt ")" {$$ = new Block(std::move($2));}
| "identifier" "{" field_list_opt "}" {$$ = new Record($1, std::move($3));}
| "identifier" "[" expr "]" "of" expr {$$ = new Array($1, $3, $6);}
| "if" expr "then" expr {$$ = new IfThen($2, $4);}
| "if" expr "then" expr "else" expr {}
| "while" expr "do" expr {}
| "for" "identifier" ":=" expr "to" expr "do" expr {}
| "break" {}
| "let" declaration_list "in" expr_seq_opt "end" {}
;
declaration_list:
  declaration {}
| declaration_list declaration {}
;
declaration:
  type_declaration {}
| variable_declaration {}
| function_declaration {}
;
type_declaration:
  "type" "identifier" "=" type {}
;
type:
  "identifier" "{" type_fields_opt "}" {}
| "array" "of" "identifier" {}
;
type_fields:
  type_field {}
| type_fields "," type_field {}
;
type_field:
  "identifier" ":" "identifier" {}
;
type_fields_opt: %empty {} | type_fields {};
variable_declaration:
  "var" "identifier" ":=" expr {}
| "var" "identifier" ":" "identifier" ":=" expr {}
;
function_declaration:
  "function" "identifier" "(" type_fields_opt ")" "=" expr {}
| "function" "identifier" "(" type_fields_opt ")" ":" "identifier" "=" expr {}
;
%%
void yy::Parser::error(const location_type& l, const std::string& m) {
  driver.error(l, m);
}
