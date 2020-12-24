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
#include "Expression.h"
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
  EQUAL   "="
  BREAK	  "break"
  DO	  "do"
  ELSE	  "else"
  END	  "end"
  FOR	  "for"
  IF	  "if"
  LET	  "let"
  NIL	  "nil"
  OF	  "of"
  THEN	  "then"
  WHILE	  "while"
;
%token <std::string> IDENTIFIER "identifier"
%token <std::string> STRING_CONSTANT "string"
%token <int> NUMBER "number"
%type  <Expression*> exp
%type  <LValue*> l_value
%type  <std::vector<std::shared_ptr<Expression>>> exp_list exp_list_opt exp_seq exp_seq_opt
%type  <std::vector<FieldValue>> field_list field_list_opt
%%
%start unit;
unit: exp  { driver.result.reset($1); };

l_value: "identifier" { $$ = new IdLValue($1); };

exp_list:
exp {$$.emplace_back($1);}
| exp_list "," exp { $1.emplace_back($3); $$ = std::move($1); }
;
exp_list_opt: %empty {} | exp_list {$$ = std::move($1);};
exp_seq:
exp {$$.emplace_back($1);}
| exp_seq ";" exp { $1.emplace_back($3); $$ = std::move($1); }
;
exp_seq_opt: %empty {} | exp_seq {$$ = std::move($1);};
field_list:
"identifier" "=" exp {AppendFieldValue($1, $3, $$);}
| field_list "," "identifier" "=" exp {AppendFieldValue($3, $5, $1);}
;
field_list_opt: %empty {} | field_list {$$ = std::move($1);};
%left ":=";
%left "+" "-";
%left "*" "/";
exp:
  "string"    { $$ = new StringConstant($1);}
| "number"    { $$ = new IntegerConstant($1); }
| "nil"       { $$ = new Nil(); }
| "-" exp     { $$ = new Negated(std::shared_ptr<Expression>($2)); }
| exp "+" exp { $$ = new Binary(std::shared_ptr<Expression>($1), BinaryOp::kPlus, std::shared_ptr<Expression>($3)); }
| l_value ":=" exp { $$ = new Assignment(std::shared_ptr<LValue>($1), std::shared_ptr<Expression>($3)); }
| "identifier" "(" exp_list_opt ")" {$$ = new FunctionCall($1, std::move($3));}
| "(" exp_seq_opt ")" {$$ = new Block(std::move($2));}
| "identifier" "{" field_list_opt "}" {$$ = new Record($1, std::move($3));}
;
%%
void yy::Parser::error(const location_type& l, const std::string& m) {
  driver.error(l, m);
}
