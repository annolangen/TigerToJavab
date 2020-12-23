%skeleton "lalr1.cc" /* -*- C++ -*- */
%require "3.0.4"
%defines
%define api.parser.class {Parser}
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
}
%define api.token.prefix {TOK_}
%token
  END  0  "end of file"
  ASSIGN  ":="
  MINUS   "-"
  PLUS    "+"
  STAR    "*"
  SLASH   "/"
  LPAREN  "("
  RPAREN  ")"
;
%token <std::string> IDENTIFIER "identifier"
%token <int> NUMBER "number"
%type  <std::shared_ptr<Expression>> exp
%type  <std::unique_ptr<LValue>> l_value
%%
%start unit;
unit: exp  { driver.result = std::move($1); };

l_value: "identifier" { $$ = std::make_unique<IdLValue>($1); };

%left ":=";
%left "+" "-";
%left "*" "/";
exp:
exp "+" exp      { $$ = std::make_shared<Binary>(std::move($1), BinaryOp::kPlus, std::move($3)); }
| "number"         { $$ = std::make_shared<IntegerConstant>($1); }
| l_value ":=" exp { $$ = std::make_shared<Assignment>(std::move($1), $3); }
;
%%
void yy::Parser::error(const location_type& l, const std::string& m) {
  driver.error(l, m);
}
