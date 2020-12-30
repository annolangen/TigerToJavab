%skeleton "lalr1.cc" /* -*- C++ -*- */
%require "3.0.4"
%defines
%define parser_class_name {Parser}
%define api.token.constructor
%define api.value.type variant
%define parse.assert
%code requires
{
#include "syntax_nodes.h"
#include <string>
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
%type  <Expression*> expr
%type  <LValue*> l_value
%type  <std::vector<std::shared_ptr<Expression>>> expr_list expr_list_opt expr_seq expr_seq_opt
%type  <std::vector<FieldValue>> field_list field_list_opt
%type  <std::vector<std::shared_ptr<Declaration>>> declaration_list
%type  <Declaration*> declaration type_declaration variable_declaration function_declaration
%type  <Type*> type
%type  <std::vector<TypeField>> type_fields_opt type_fields
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

%expect 1; //lvalue and array litteral. 

%%
%start unit;
unit: expr  { driver.result.reset($1); };

l_value:
  "identifier" { $$ = new IdLValue($1); }
| l_value "." "identifier" { $$ = new FieldLValue($1, $3); }
| l_value "[" expr "]" {$$ = new IndexLValue($1, $3); }
;

expr_list:
  expr {$$.emplace_back($1);}
| expr_list "," expr { $1.emplace_back($3); $$ = std::move($1); }
;
expr_list_opt: %empty {} | expr_list {$$ = std::move($1);};
expr_seq:
  expr {$$.emplace_back($1);}
| expr_seq SEMICOLON expr { $1.emplace_back($3); $$ = std::move($1); }
;
expr_seq_opt: %empty {} | expr_seq {$$ = std::move($1);};
field_list:
  "identifier" "=" expr {AppendFieldValue($1, $3, $$);}
| field_list COMMA "identifier" "=" expr {AppendFieldValue($3, $5, $1);}
;
field_list_opt: %empty {} | field_list {$$ = std::move($1);};
expr:
  "string"    { $$ = new StringConstant($1.substr(1, $1.size()-2));}
| "number"    { $$ = new IntegerConstant($1); }
| "nil"       { $$ = new Nil(); }
| l_value     { $$ = $1; }
| "-" expr     { $$ = new Negated($2); }
| expr "+" expr { $$ = new Binary($1, BinaryOp::kPlus, $3);}
| expr "-" expr { $$ = new Binary($1, BinaryOp::kMinus, $3);}
| expr "*" expr { $$ = new Binary($1, BinaryOp::kTimes, $3);}
| expr "/" expr { $$ = new Binary($1, BinaryOp::kDivide, $3);}
| expr "=" expr { $$ = new Binary($1, BinaryOp::kEqual, $3);}
| expr "<>" expr { $$ = new Binary($1, BinaryOp::kUnequal, $3);}
| expr "<" expr { $$ = new Binary($1, BinaryOp::kLessThan, $3);}
| expr ">" expr { $$ = new Binary($1, BinaryOp::kGreaterThan, $3);}
| expr "<=" expr { $$ = new Binary($1, BinaryOp::kNotGreaterThan, $3);}
| expr ">=" expr { $$ = new Binary($1, BinaryOp::kNotLessThan, $3);}
| expr "&" expr { $$ = new Binary($1, BinaryOp::kAnd, $3);}
| expr "|" expr { $$ = new Binary($1, BinaryOp::kOr, $3);}
| l_value ":=" expr { $$ = new Assignment(std::shared_ptr<LValue>($1), $3); }
| "identifier" "(" expr_list_opt ")" {$$ = new FunctionCall($1, std::move($3));}
| "(" expr_seq_opt ")" {$$ = new Block(std::move($2));}
| "identifier" "{" field_list_opt "}" {$$ = new Record($1, std::move($3));}
| "identifier" "[" expr "]" "of" expr {$$ = new Array($1, $3, $6);}
| "if" expr "then" expr {$$ = new IfThen($2, $4);}
| "if" expr "then" expr "else" expr {$$ = new IfThenElse($2, $4, $6);}
| "while" expr "do" expr {$$ = new While($2, $4);}
| "for" "identifier" ":=" expr "to" expr "do" expr {$$ = new For($2, $4, $6, $8);}
| "break" {$$ = new Break();}
| "let" declaration_list "in" expr_seq_opt "end" {$$ = new Let(std::move($2), std::move($4));}
;
declaration_list:
  declaration {$$.emplace_back($1);}
| declaration_list declaration {$1.emplace_back($2); $$ = std::move($1);}
;
declaration:
  type_declaration {$$ = $1;}
| variable_declaration {$$ = $1;}
| function_declaration {$$ = $1;}
;
type_declaration:
  "type" "identifier" "=" type {$$ = new TypeDeclaration($2, $4);}
;
type:
  "identifier" {$$ = new TypeReference($1);}
| "{" type_fields_opt "}" {$$ = new RecordType(std::move($2));}
| "array" "of" "identifier" {$$ = new ArrayType($3);}
;
type_fields:
  type_field {$$.push_back(std::move($1));}
| type_fields "," type_field {$1.push_back(std::move($3)); $$ = std::move($1);}
;
type_field:
  "identifier" ":" "identifier" {$$ = {$1, $3};}
;
type_fields_opt: %empty {} | type_fields {$$ = std::move($1);};
variable_declaration:
  "var" "identifier" ":=" expr {$$ = new VariableDeclaration($2, $4);}
| "var" "identifier" ":" "identifier" ":=" expr {$$ = new VariableDeclaration($2, $4, $6);}
;
function_declaration:
  "function" "identifier" "(" type_fields_opt ")" "=" expr {
    $$ = new FunctionDeclaration($2, std::move($4), $7);
  }
| "function" "identifier" "(" type_fields_opt ")" ":" "identifier" "=" expr {
    $$ = new FunctionDeclaration($2, std::move($4), $7, $9);
  }
;
%%
void yy::Parser::error(const location_type& l, const std::string& m) {
  driver.error(l, m);
}
