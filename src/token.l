%{ /* -*- C++ -*- */
# include <cerrno>
# include <climits>
# include <cstdlib>
# include "Token.h"

// Work around an incompatibility in flex (at least versions
// 2.5.31 through 2.5.33): it generates code that does
// not conform to C89.  See Debian bug 333231
// <http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=333231>.
# undef yywrap
# define yywrap() 1
extern "C" int fileno(FILE *);
%}
%option noyywrap nounput batch debug noinput
id    [a-zA-Z][a-zA-Z_0-9]*
int   [0-9]+
blank [ \t]
string \"[^\"]*\"

%%
{blank}+   ;
[\n]+      ;
"&"      return Token::AND;
"("      return Token::LPAREN;
")"      return Token::RPAREN;
"{"      return Token::LBRACE;
"}"      return Token::RBRACE;
"*"      return Token::STAR;
"+"      return Token::PLUS;
","      return Token::COMMA;
"-"      return Token::MINUS;
"."      return Token::DOT;
"/"      return Token::SLASH;
";"      return Token::SEMICOLON;
":"      return Token::COLON;
":="     return Token::ASSIGN;
"<"      return Token::LT;
"<="     return Token::LE;
"<>"     return Token::NE;
"="      return Token::EQUAL;
">"      return Token::GT;
">="     return Token::GE;
"["      return Token::LBRACKET;
"]"      return Token::RBRACKET;
"|"      return Token::OR;
"array"	 return Token::ARRAY;
"break"	 return Token::BREAK;
"do"	 return Token::DO;
"else"	 return Token::ELSE;
"end"	 return Token::END;
"for"	 return Token::FOR;
"function" return Token::FUNCTION;
"if"	 return Token::IF;
"in"     return Token::IN;
"let"	 return Token::LET;
"nil"    return Token::NIL;
"of"	 return Token::OF;
"then"	 return Token::THEN;
"to"	 return Token::TO;
"type"	 return Token::TYPE;
"var"	 return Token::VAR;
"while"	 return Token::WHILE;

{int}    return Token::NUMBER(n, loc);
{string} return Token::STRING_CONSTANT;
{id}     return Token::IDENTIFIER;
.        std::cerr << "invalid character '" << yytext <<"'";
<<EOF>>  return Token::EOF;
%%
