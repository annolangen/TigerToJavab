#include "syntax.h"
#include "syntax_insertion.h"
#include <algorithm>
#include <climits>
#include <iostream>

namespace syntax {
namespace {
// Parses next expression starting with result of previous yylex call.
std::unique_ptr<Expr> ParseExpr(Token&);

// Returns `Expr` that starts with the given l-value. Updates `next` to point
// just past the expression.
std::unique_ptr<Expr> ParseExprWithLValue(Token& next, LValue& l_value) {
  switch (next) {
  case Token::DOT: {
    next = yylex();
    if (next != Token::IDENTIFIER) {
      std::cerr << "Expected identifier in l-value after DOT." << std::endl;
      return {};
    }
    LValue field =
        RecordField{std::make_unique<LValue>(std::move(l_value)), yytext};
    next = yylex();
    return ParseExprWithLValue(next, field);
  }
  case Token::LBRACKET: {
    next = yylex();
    std::unique_ptr<Expr> expr = ParseExpr(next);
    if (next != Token::RBRACKET) {
      std::cerr << "Expected `]` in array element l-value." << std::endl;
      return {};
    }
    LValue element = ArrayElement{std::make_unique<LValue>(std::move(l_value)),
                                  std::move(expr)};
    return ParseExprWithLValue(next, element);
  }
  case Token::ASSIGN: {
    next = yylex();
    std::unique_ptr<Expr> expr = ParseExpr(next);
    return std::make_unique<Expr>(Assignment{
        std::make_unique<LValue>(std::move(l_value)), std::move(expr)});
  }
  default:
    return std::make_unique<Expr>(std::move(l_value));
  }
}

template <class T, class P>
void ParseList(P parser, Token separator, Token end, Token& next,
               std::vector<T>& list) {
  while (next != end) {
    list.emplace_back(parser(next));
    if (next == separator) {
      next = yylex();
    } else if (next != end) {
      std::cerr << "Expected " << separator << " or " << end << " in list"
                << std::endl;
      return;
    }
  }
  next = yylex();
  return;
}

FieldAssignment ParseFieldAssignment(Token& next) {
  if (next != Token::IDENTIFIER) {
    std::cerr << "Expected identifier in field assignment" << std::endl;
    return {};
  }
  std::string id = yytext;
  next = yylex();
  if (next != Token::EQUAL) {
    std::cerr << "Expected EQUAL to separate ID from EXPR in field assignment"
              << std::endl;
    return {};
  }
  next = yylex();
  return FieldAssignment{id, ParseExpr(next)};
}

void ParseFieldList(Token separator, Token end, Token& next,
                    std::vector<FieldAssignment>& fields) {
  ParseList(ParseFieldAssignment, separator, end, next, fields);
}

std::unique_ptr<Expr> ParseExprWithExpr(Token& next,
                                        std::unique_ptr<Expr> expr) {
  static std::vector<Token> ops = {Token::PLUS,  Token::MINUS, Token::STAR,
                                   Token::SLASH, Token::EQUAL, Token::NE,
                                   Token::LT,    Token::GT,    Token::LE,
                                   Token::GE,    Token::AND,   Token::OR};
  auto i = std::find(ops.begin(), ops.end(), next);
  if (i == ops.end()) return std::move(expr);
  Token op = next;
  next = yylex();
  std::unique_ptr<Expr> right = ParseExpr(next);
  // TODO(implement precedence based on order in ops)
  // check whether `expr` is binary and its op appears before `op` in `ops`
  return ParseExprWithExpr(next, std::make_unique<Expr>(Binary{
                                     std::move(expr), op, std::move(right)}));
}

// Parses expression that starts with the given identifier. Updates `next` to
// the token just past the parsed expression.
std::unique_ptr<Expr> ParseExprWithId(Token& next, const Identifier& id) {
  switch (next) {
  case Token::LPAREN: {
    next = yylex();
    std::vector<std::unique_ptr<Expr>> arguments;
    ParseList(ParseExpr, Token::COMMA, Token::RPAREN, next, arguments);
    return std::make_unique<Expr>(FunctionCall{id, std::move(arguments)});
  }
  case Token::LBRACE: {
    next = yylex();
    std::vector<FieldAssignment> fields;
    ParseList(ParseFieldAssignment, Token::COMMA, Token::RBRACE, next, fields);
    return std::make_unique<Expr>(RecordLiteral{id, std::move(fields)});
  }
  case Token::LBRACKET: {
    next = yylex();
    std::unique_ptr<Expr> expr = ParseExpr(next);
    if (next != Token::RBRACKET) {
      std::cerr << "Expected `]` in array element" << std::endl;
      return {};
    }
    next = yylex();
    if (next == Token::OF) {
      return std::make_unique<Expr>(
          ArrayLiteral{id, std::move(expr), ParseExpr(next)});
    }
    return ParseExprWithExpr(
        next, std::make_unique<Expr>(
                  ArrayElement{std::make_unique<LValue>(id), std::move(expr)}));
  }
  default: {
    LValue l_value = id;
    return ParseExprWithLValue(next, l_value);
  }
  }
}

TypeField ParseTypeField(Token& next) {
  if (next != Token::IDENTIFIER) {
    std::cerr << "Expected identifer in type field declaration" << std::endl;
    return {};
  }
  TypeField result{std::string(yytext)};
  next = yylex();
  if (next != Token::IDENTIFIER) {
    std::cerr << "Expected identifer in type field declaration" << std::endl;
    return {};
  }
  result.type_id = yytext;
  next = yylex();
  return result;
}

TypeDeclaration ParseTypeDeclaration(Token next) {
  next = yylex();
  if (next != Token::IDENTIFIER) {
    std::cerr << "Expected identifer in type declaration" << std::endl;
    return {};
  }
  TypeDeclaration result{yytext};
  next = yylex();
  switch (next) {
  case Token::IDENTIFIER:
    result.value = std::string(yytext);
    next = yylex();
    return result;
  case Token::LBRACE: {
    std::vector<TypeField> fields;
    ParseList(ParseTypeField, Token::COMMA, Token::RBRACE, next, fields);
    result.value = std::move(fields);
    return result;
  }
  case Token::ARRAY:
    next = yylex();
    if (next != Token::OF) {
      std::cerr << "Expected OF in array type declaration" << std::endl;
      return {};
    }
    next = yylex();
    if (next != Token::IDENTIFIER) {
      std::cerr << "Expected identifer in array type declaration" << std::endl;
      return {};
    }
    result.value = ArrayType{yytext};
    next = yylex();
    return result;
  default:
    std::cerr
        << "Expected one of IDENTIFIER, LBRACE, or ARRAY in type declaration"
        << std::endl;
    return {};
  }
}

VariableDeclaration ParseVariableDeclaration(Token next) {
  next = yylex();
  if (next != Token::IDENTIFIER) {
    std::cerr << "Expected identifer in type declaration" << std::endl;
    return {};
  }
  VariableDeclaration result{yytext};
  next = yylex();
  if (next == Token::COLON) {
    next = yylex();
    if (next != Token::IDENTIFIER) {
      std::cerr << "Expected identifer in variable declaration after COLON"
                << std::endl;
      return {};
    }
    result.type_id = yytext;
    next = yylex();
  }
  if (next != Token::ASSIGN) {
    std::cerr << "Expected ASSIGN in type declaration" << std::endl;
    return {};
  }
  next = yylex();
  result.value = ParseExpr(next);
  return result;
}

FunctionDeclaration ParseFunctionDeclaration(Token next) {
  next = yylex();
  if (next != Token::IDENTIFIER) {
    std::cerr << "Expected identifer in function declaration" << std::endl;
    return {};
  }
  FunctionDeclaration result{yytext};
  next = yylex();
  if (next != Token::LPAREN) {
    std::cerr << "Expected LPAREN in function declaration" << std::endl;
    return {};
  }
  next = yylex();
  ParseList(ParseTypeField, Token::COMMA, Token::RPAREN, next,
            result.parameter);
  if (next == Token::COLON) {
    next = yylex();
    if (next != Token::IDENTIFIER) {
      std::cerr << "Expected type-id after COLON in function declaration"
                << std::endl;
      return {};
    }
    result.type_id = yytext;
    next = yylex();
  }
  if (next != Token::EQUAL) {
    std::cerr << "Expected EQUAL before body in function declaration"
              << std::endl;
  }
  next = yylex();
  result.body = ParseExpr(next);
  return result;
}

void ParseDeclarationList(Token& next, std::vector<Declaration>& list) {
  while (next != Token::IN) {
    switch (next) {
    case Token::TYPE:
      list.emplace_back(ParseTypeDeclaration(next));
    case Token::VAR:
      list.emplace_back(ParseVariableDeclaration(next));
    case Token::FUNCTION:
      list.emplace_back(ParseFunctionDeclaration(next));
    default:
      std::cerr << "Expected one of TYPE, VAR, or FUNCTION in declaration list"
                << std::endl;
    }
  }
}

std::unique_ptr<Expr> ParseExpr(Token& next) {
  switch (next) {
  case Token::END_OF_FILE:
    return {};
  case Token::STRING_CONSTANT: {
    StringConstant text{std::string(yytext + 1, yyleng - 2)};
    next = yylex();
    return ParseExprWithExpr(next, std::make_unique<Expr>(text));
  }
  case Token::NUMBER: {
    long n = strtol(yytext, NULL, 10);
    if (!(INT_MIN <= n && n <= INT_MAX && errno != ERANGE)) {
      return {};
    }
    next = yylex();
    return ParseExprWithExpr(
        next, std::make_unique<Expr>(static_cast<IntegerConstant>(n)));
  }
  case Token::NIL:
    next = yylex();
    return ParseExprWithExpr(next, std::make_unique<Expr>(Nil()));
  case Token::IDENTIFIER: {
    Identifier id = yytext;
    next = yylex();
    return ParseExprWithExpr(next, ParseExprWithId(next, id));
  }
  case Token::MINUS: {
    next = yylex();
    return ParseExprWithExpr(next,
                             std::make_unique<Expr>(Negated{ParseExpr(next)}));
  }
  case Token::LPAREN: {
    next = yylex();
    std::vector<std::unique_ptr<Expr>> exprs;
    ParseList(ParseExpr, Token::SEMICOLON, Token::RPAREN, next, exprs);
    return ParseExprWithExpr(
        next, std::make_unique<Expr>(Parenthesized{std::move(exprs)}));
  }
  case Token::IF: {
    next = yylex();
    auto cond = ParseExpr(next);
    if (next != Token::THEN) {
      std::cerr << "Expected THEN after condition in IF expression"
                << std::endl;
      return {};
    }
    next = yylex();
    auto then_expr = ParseExpr(next);
    if (next != Token::ELSE) {
      return std::make_unique<Expr>(
          IfThen{std::move(cond), std::move(then_expr)});
    }
    next = yylex();
    return std::make_unique<Expr>(
        IfThenElse{std::move(cond), std::move(then_expr), ParseExpr(next)});
  }
  case Token::WHILE: {
    next = yylex();
    auto cond = ParseExpr(next);
    if (next != Token::DO) {
      std::cerr << "Expected DO after condition in WHILE expr" << std::endl;
      return {};
    }
    next = yylex();
    return std::make_unique<Expr>(While{std::move(cond), ParseExpr(next)});
  }
  case Token::FOR: {
    next = yylex();
    if (next != Token::IDENTIFIER) {
      std::cerr << "Expected IDENTIFIER in FOR expression" << std::endl;
      return {};
    }
    std::string id = yytext;
    next = yylex();
    if (next != Token::ASSIGN) {
      std::cerr << "Expected ASSIGN in FOR expression" << std::endl;
      return {};
    }
    next = yylex();
    auto start = ParseExpr(next);
    if (next != Token::TO) {
      std::cerr << "Expected TO in FOR expression" << std::endl;
      return {};
    }
    next = yylex();
    auto end = ParseExpr(next);
    if (next != Token::DO) {
      std::cerr << "Expected DO in FOR expression" << std::endl;
      return {};
    }
    next = yylex();
    return std::make_unique<Expr>(
        For{id, std::move(start), std::move(end), ParseExpr(next)});
  }
  case Token::BREAK:
    return std::make_unique<Expr>(Break{});
  case Token::LET: {
    next = yylex();
    std::vector<Declaration> declaration;
    ParseDeclarationList(next, declaration);
    if (next != Token::IN) {
      std::cerr << "Expected IN in LET expression" << std::endl;
      return {};
    }
    next = yylex();
    std::vector<std::unique_ptr<Expr>> body;
    ParseList(ParseExpr, Token::SEMICOLON, Token::END, next, body);
    return std::make_unique<Expr>(Let{std::move(declaration), std::move(body)});
  }
  default:
    return {};
  }
}
} // namespace

std::unique_ptr<Expr> Parse() {
  Token next = yylex();
  return ParseExpr(next);
}
} // namespace syntax
