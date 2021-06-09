#pragma once

enum Token {
  T_EOF = 0,
  AND,
  ARRAY,
  ASSIGN,
  BREAK,
  COLON,
  COMMA,
  DO,
  DOT,
  ELSE,
  END,
  EQUAL,
  FOR,
  FUNCTION,
  GE,
  GT,
  IDENTIFIER,
  IF,
  IN,
  LBRACE,
  LBRACKET,
  LE,
  LET,
  LPAREN,
  LT,
  MINUS,
  NE,
  NIL,
  NUMBER,
  OF,
  OR,
  PLUS,
  RBRACE,
  RBRACKET,
  RPAREN,
  SEMICOLON,
  SLASH,
  STAR,
  STRING_CONSTANT,
  THEN,
  TO,
  TYPE,
  VAR,
  WHILE
};

extern int yylex();
extern char* yytext;
extern int yylen;
