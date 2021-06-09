#include "syntax.h"
#include <climits>
#include <iostream>

namespace syntax {
namespace {
std::unique_ptr<Expr> ParseExpr(Token&);

LValue ParseLValue(Token& next, const Identifier& id) {
  LValue result = id;
  for (;; next = static_cast<Token>(yylex())) {
    switch (next) {
    case Token::DOT:
      next = static_cast<Token>(yylex());
      if (next != Token::IDENTIFIER) {
        std::cerr << "Expected identifier in l-value after DOT." << std::endl;
        return {};
      }
      result = RecordField{std::make_unique<LValue>(std::move(result)), yytext};
      continue;

    case Token::LBRACE: {
      next = static_cast<Token>(yylex());
      std::unique_ptr<Expr> expr = ParseExpr(next);
      if (next != Token::RBRACE) {
        std::cerr << "Expected `]` in array element l-value." << std::endl;
        return {};
      }
      result = ArrayElement{std::make_unique<LValue>(std::move(result)),
                            std::move(expr)};
    }
    default:
      return result;
    }
  }
}

Parenthesized ParseParenthesized(Token& next) {
  Parenthesized result;
  if (next != Token::LPAREN) return result; // precondition
  next = static_cast<Token>(yylex());
  if (next == Token::RPAREN) return result;
  while (true) {
    result.emplace_back(ParseExpr(next));
    switch (next) {
    case Token::RPAREN:
      next = static_cast<Token>(yylex());
      return result;
    case Token::COMMA:
      next = static_cast<Token>(yylex());
      continue;
    default:
      std::cerr << "Expected comma or closing parenthesis in parenthesized "
                   "expression list."
                << std::endl;
      return result;
    }
  }
}

std::unique_ptr<Expr> ParseExprWithId(Token& next, const Identifier& id) {
  switch (next) {
  case Token::DOT:
  case Token::LBRACE:
    return std::make_unique<Expr>(ParseLValue(next, id));
  case Token::LPAREN:
    return std::make_unique<Expr>(FunctionCall{id, ParseParenthesized(next)});
  default:
    return std::make_unique<Expr>(LValue{id});
  }
}

std::unique_ptr<Expr> ParseExpr(Token& next) {
  switch (next) {
  case Token::T_EOF:
    return {};
  case Token::STRING_CONSTANT: {
    StringConstant text = yytext;
    return std::make_unique<Expr>(text);
  }
  case Token::NUMBER: {
    long n = strtol(yytext, NULL, 10);
    if (!(INT_MIN <= n && n <= INT_MAX && errno != ERANGE)) {
      return {};
    }
    return std::make_unique<Expr>(static_cast<IntegerConstant>(n));
  }
  case Token::NIL:
    return std::make_unique<Expr>(Nil());
  case Token::IDENTIFIER:
    Identifier id = yytext;
    next = static_cast<Token>(yylex());
    return ParseExprWithId(next, id);
  }
}
} // namespace

std::unique_ptr<Expr> Parse() {
  Token next = static_cast<Token>(yylex());
  return ParseExpr(next);
}

} // namespace syntax
