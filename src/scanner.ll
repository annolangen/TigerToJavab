%{ /* -*- C++ -*- */
# include <cerrno>
# include <climits>
# include <cstdlib>
# include <string>
# include "driver.h"
# include "parser.hh"

// Work around an incompatibility in flex (at least versions
// 2.5.31 through 2.5.33): it generates code that does
// not conform to C89.  See Debian bug 333231
// <http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=333231>.
# undef yywrap
# define yywrap() 1

// The location of the current token.
static yy::location loc;

// For comment nesting.
static int comment_depth;

extern "C" int fileno(FILE *);
%}
%option noyywrap nounput batch debug noinput

%x C_COMMENT

id    [a-zA-Z][a-zA-Z_0-9]*
int   [0-9]+
blank [ \t]
string \"[^\"]*\"

%{
  // Code run each time a pattern is matched.
  # define YY_USER_ACTION  loc.columns(yyleng);
%}

%%

%{
  // Code run each time yylex is called.
  loc.step();
%}

{blank}+   loc.step ();
[\n]+      loc.lines (yyleng); loc.step ();

"/*"     { comment_depth = 1; BEGIN(C_COMMENT); }

"&"      return yy::Parser::make_AND(loc);
"("      return yy::Parser::make_LPAREN(loc);
")"      return yy::Parser::make_RPAREN(loc);
"{"      return yy::Parser::make_LBRACE(loc);
"}"      return yy::Parser::make_RBRACE(loc);
"*"      return yy::Parser::make_STAR(loc);
"+"      return yy::Parser::make_PLUS(loc);
","      return yy::Parser::make_COMMA(loc);
"-"      return yy::Parser::make_MINUS(loc);
"."      return yy::Parser::make_DOT(loc);
"/"      return yy::Parser::make_SLASH(loc);
";"      return yy::Parser::make_SEMICOLON(loc);
":"      return yy::Parser::make_COLON(loc);
":="     return yy::Parser::make_ASSIGN(loc);
"<"      return yy::Parser::make_LT(loc);
"<="     return yy::Parser::make_LE(loc);
"<>"     return yy::Parser::make_NE(loc);
"="      return yy::Parser::make_EQUAL(loc);
">"      return yy::Parser::make_GT(loc);
">="     return yy::Parser::make_GE(loc);
"["      return yy::Parser::make_LBRACKET(loc);
"]"      return yy::Parser::make_RBRACKET(loc);
"|"      return yy::Parser::make_OR(loc);
"array"	 return yy::Parser::make_ARRAY(loc);
"break"	 return yy::Parser::make_BREAK(loc);
"do"	 return yy::Parser::make_DO(loc);
"else"	 return yy::Parser::make_ELSE(loc);
"end"	 return yy::Parser::make_END(loc);
"for"	 return yy::Parser::make_FOR(loc);
"function"	 return yy::Parser::make_FUNCTION(loc);
"if"	 return yy::Parser::make_IF(loc);
"in"     return yy::Parser::make_IN(loc);
"let"	 return yy::Parser::make_LET(loc);
"nil"    return yy::Parser::make_NIL(loc);
"of"	 return yy::Parser::make_OF(loc);
"then"	 return yy::Parser::make_THEN(loc);
"to"	 return yy::Parser::make_TO(loc);
"type"	 return yy::Parser::make_TYPE(loc);
"var"	 return yy::Parser::make_VAR(loc);
"while"	 return yy::Parser::make_WHILE(loc);

{int}      {
  errno = 0;
  long n = strtol(yytext, NULL, 10);
  if (!(INT_MIN <= n && n <= INT_MAX && errno != ERANGE)) {
    driver.error(loc, "integer is out of range");
  }
  return yy::Parser::make_NUMBER(n, loc);
}
{string}   return yy::Parser::make_STRING_CONSTANT(yytext, loc);
{id}       return yy::Parser::make_IDENTIFIER(yytext, loc);
.          driver.error(loc, std::string("invalid character '")+yytext+"'");
<<EOF>>    return yy::Parser::make_EOF(loc);

<C_COMMENT>{
  "/*"                { comment_depth++; loc.columns(yyleng); }
  "*/"                { loc.columns(yyleng); if (--comment_depth == 0) BEGIN(INITIAL); }
  [^*\n/]+            { loc.columns(yyleng); }
  \n                  { loc.lines(yyleng); loc.step(); }
  .                   { loc.columns(yyleng); }
  <<EOF>>             { driver.error(loc, "unterminated comment"); return yy::Parser::make_EOF(loc); }
}
%%

void Driver::scan_begin() {
  loc.initialize();
  yy_flex_debug = options_.trace_scanning;
  if (file.empty() || file == "-") {
    yyin = stdin;
  } else if (!(yyin = fopen(file.c_str(), "r"))) {
    error("cannot open " + file + ": " + strerror(errno));
    exit(EXIT_FAILURE);
  }
}

void Driver::scan_end() { fclose(yyin); }
