%skeleton "lalr1.cc" /* -*- C++ -*- */
%require "3.0.4"
%defines
%define api.parser.class {Parser}
%define api.token.constructor
%define api.value.type variant
%define api.value.automove
%define parse.assert
%code requires
{
#include "syntax.h"
#include <string>
class Driver;
using namespace syntax;
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
 // shift-reduce conflict avoided from https://tinyurl.com/2azc9rm8
%code
{
#include "driver.h"
#include "syntax_builders.h"
}
%define api.token.prefix {TOK_}
%token
  EOF  0  "end of file"
  AND     "&"
  ASSIGN  ":="
  COLON   ":"
  COMMA   ","
  DOT     "."
  EQUAL   "="
  GE      ">="
  GT      ">"
  LBRACKET "["
  LE      "<="
  LPAREN  "("
  LBRACE  "{"
  LT      "<"
  MINUS   "-"
  NE      "<>"
  OR      "|"
  PLUS    "+"
  RBRACKET "]"
  RPAREN  ")"
  RBRACE  "}"
  SLASH   "/"
  STAR    "*"
  ARRAY	  "array"
  BREAK	  "break"
  DO	  "do"
  ELSE	  "else"
  END	  "end"
  FOR	  "for"
  FUNCTION	  "function"
  IF	  "if"
  IN    "in"
  LET	  "let"
  NIL	  "nil"
  OF	  "of"
  THEN	  "then"
  TO	  "to"
  TYPE	  "type"
  VAR	  "var"
  WHILE	  "while"
  SEMICOLON ";"
;
%token <std::string> IDENTIFIER "identifier"
%token <std::string> STRING_CONSTANT "string"
%token <int> NUMBER "number"
%type  <std::unique_ptr<Expr>> expr
%type  <std::unique_ptr<LValue>> l_value l_value_not_id
%type  <std::vector<std::unique_ptr<Expr>>> expr_list expr_list_opt expr_seq expr_seq_opt
%type  <std::vector<FieldAssignment>> field_list field_list_opt
%type  <std::vector<std::unique_ptr<Declaration>>> declaration_list
%type  <std::unique_ptr<Declaration>> declaration type_declaration variable_declaration function_declaration
%type  <Type> type
%type  <TypeFields> type_fields_opt type_fields
%type  <TypeField> type_field

%left ";";
%left ",";
%left ":=";
%nonassoc "then";
%nonassoc "else";
%nonassoc "while" "do" "for" "of";
%left "[" "(" "{";
%left "|";
%left "&";
%left "<>" "=";
%left "<" "<=" ">" ">=";
%left MINUS PLUS;
%left STAR SLASH;

%%
%start unit;
unit: expr  { driver.result = $1; }
;
l_value:
  "identifier" { $$ = std::make_unique<LValue>($1); }
| l_value_not_id { $$ = $1; }
;
l_value_not_id:
  l_value "." "identifier" { $$ = std::make_unique<LValue>(RecordField{$1, $3}); }
| "identifier" "[" expr "]" { $$ = std::make_unique<LValue>(ArrayElement{std::make_unique<LValue>($1), $3}); }
| l_value_not_id "[" expr "]" { $$ = std::make_unique<LValue>(ArrayElement{$1, $3}); }
;
expr_list:
  expr { $$.emplace_back($1);}
| expr_list "," expr { $$ = Add($1, $3); }
;
expr_list_opt: %empty {} | expr_list { $$ = $1; }
;
expr_seq:
  expr { $$.emplace_back($1); }
| expr_seq SEMICOLON expr { $$ = Add($1, $3); }
;
expr_seq_opt: %empty {} | expr_seq { $$ = $1; }
;
field_list:
  "identifier" "=" expr { $$ = Add({}, $1, $3); }
| field_list COMMA "identifier" "=" expr { $$ = Add($1, $3, $5); }
;
field_list_opt: %empty {} | field_list { $$ = $1; }
;
expr:
  "string"    {
    std::string quoted = $1;
    $$ = std::make_unique<Expr>(StringConstant{quoted.substr(1, quoted.size()-2)});
  }
| "number"    { $$ = std::make_unique<Expr>(IntegerConstant{$1}); }
| "nil"       { $$ = std::make_unique<Expr>(Nil{}); }
| l_value_not_id { $$ = std::make_unique<Expr>($1); }
| "identifier" { $$ = std::make_unique<Expr>(std::make_unique<LValue>($1)); }
| "-" expr     { $$ = std::make_unique<Expr>(Negated{$2}); }
| expr "+" expr { $$ = std::make_unique<Expr>(Binary{$1, BinaryOp::kPlus, $3}); }
| expr "-" expr { $$ = std::make_unique<Expr>(Binary{$1, BinaryOp::kMinus, $3}); }
| expr "*" expr { $$ = std::make_unique<Expr>(Binary{$1, BinaryOp::kTimes, $3}); }
| expr "/" expr { $$ = std::make_unique<Expr>(Binary{$1, BinaryOp::kDivide, $3}); }
| expr "=" expr { $$ = std::make_unique<Expr>(Binary{$1, BinaryOp::kEqual, $3}); }
| expr "<>" expr { $$ = std::make_unique<Expr>(Binary{$1, BinaryOp::kUnequal, $3}); }
| expr "<" expr { $$ = std::make_unique<Expr>(Binary{$1, BinaryOp::kLessThan, $3}); }
| expr ">" expr { $$ = std::make_unique<Expr>(Binary{$1, BinaryOp::kGreaterThan, $3}); }
| expr "<=" expr { $$ = std::make_unique<Expr>(Binary{$1, BinaryOp::kNotGreaterThan, $3}); }
| expr ">=" expr { $$ = std::make_unique<Expr>(Binary{$1, BinaryOp::kNotLessThan, $3}); }
| expr "&" expr { $$ = std::make_unique<Expr>(Binary{$1, BinaryOp::kAnd, $3}); }
| expr "|" expr { $$ = std::make_unique<Expr>(Binary{$1, BinaryOp::kOr, $3}); }
| l_value ":=" expr { $$ = std::make_unique<Expr>(Assignment{std::unique_ptr<LValue>($1), $3}); }
| "identifier" "(" expr_list_opt ")" { $$ = std::make_unique<Expr>(FunctionCall{$1, $3}); }
| "(" expr_seq_opt ")" { $$ = std::make_unique<Expr>(Parenthesized{$2}); }
| "identifier" "{" field_list_opt "}" { $$ = std::make_unique<Expr>(RecordLiteral{$1, $3}); }
| "identifier" "[" expr "]" "of" expr { $$ = std::make_unique<Expr>(ArrayLiteral{$1, $3, $6}); }
| "if" expr "then" expr { $$ = std::make_unique<Expr>(IfThen{$2, $4}); }
| "if" expr "then" expr "else" expr { $$ = std::make_unique<Expr>(IfThenElse{$2, $4, $6}); }
| "while" expr "do" expr { $$ = std::make_unique<Expr>(While{$2, $4}); }
| "for" "identifier" ":=" expr "to" expr "do" expr { $$ = std::make_unique<Expr>(For{$2, $4, $6, $8}); }
| "break" { $$ = std::make_unique<Expr>(Break{}); }
| "let" declaration_list "in" expr_seq_opt "end" { $$ = std::make_unique<Expr>(Let{$2, $4}); }
;
declaration_list:
  declaration { $$.emplace_back($1); }
| declaration_list declaration { $$ = Add($1, $2); }
;
declaration:
  type_declaration { $$ = $1; }
| variable_declaration { $$ = $1; }
| function_declaration { $$ = $1; }
;
type_declaration:
  "type" "identifier" "=" type { $$ = std::make_unique<Declaration>(TypeDeclaration{$2, $4}); }
;
type:
  "identifier" { $$ = Identifier{$1}; }
| "{" type_fields_opt "}" { $$ = $2; }
| "array" "of" "identifier" { $$ = ArrayType{$3}; }
;
type_fields:
  type_field { $$.push_back($1); }
| type_fields "," type_field { $$ = $1; $$.push_back($3); }
;
type_field: "identifier" ":" "identifier" { $$ = {$1, $3}; }
;
type_fields_opt: %empty {} | type_fields { $$ = $1; };
variable_declaration:
"var" "identifier" ":=" expr { $$ = std::make_unique<Declaration>(VariableDeclaration{$2, $4}); }
| "var" "identifier" ":" "identifier" ":=" expr { $$ = std::make_unique<Declaration>(VariableDeclaration{$2, $6, $4}); }
;
function_declaration:
"function" "identifier" "(" type_fields_opt ")" "=" expr { $$ = std::make_unique<Declaration>(FunctionDeclaration{$2, $4, $7}); }
| "function" "identifier" "(" type_fields_opt ")" ":" "identifier" "=" expr { $$ = std::make_unique<Declaration>(FunctionDeclaration{$2, $4, $9, $7}); }
;
%%
void yy::Parser::error(const location_type& l, const std::string& m) {
  driver.error(l, m);
}
