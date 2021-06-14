#include "syntax.h"
#include <algorithm>
#include <climits>
#include <iostream>

namespace syntax {
namespace {
// Parses next expression starting with calling yylex
std::unique_ptr<Expr> ParseExpr(Token&);

// Returns `Expr` that starts with the given l-value. Updates `next` to point
// just past the expression.
std::unique_ptr<Expr> ParseExprWithLValue(Token& next, LValue& l_value) {
  switch (next) {
  case DOT: {
    next = static_cast<Token>(yylex());
    if (next != IDENTIFIER) {
      std::cerr << "Expected identifier in l-value after DOT." << std::endl;
      return {};
    }
    LValue field =
        RecordField{std::make_unique<LValue>(std::move(l_value)), yytext};
    next = static_cast<Token>(yylex());
    return ParseExprWithLValue(next, field);
  }
  case LBRACKET: {
    next = static_cast<Token>(yylex());
    std::unique_ptr<Expr> expr = ParseExpr(next);
    if (next != RBRACKET) {
      std::cerr << "Expected `]` in array element l-value." << std::endl;
      return {};
    }
    LValue element = ArrayElement{std::make_unique<LValue>(std::move(l_value)),
                                  std::move(expr)};
    return ParseExprWithLValue(next, element);
  }
  case ASSIGN: {
    next = static_cast<Token>(yylex());
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
      next = static_cast<Token>(yylex());
    } else if (next != end) {
      std::cerr << "Expected " << separator << " or " << end << " in list"
                << std::endl;
      return;
    }
  }
  next = static_cast<Token>(yylex());
  return;
}

FieldAssignment ParseFieldAssignment(Token& next) {
  if (next != IDENTIFIER) {
    std::cerr << "Expected identifier in field assignment" << std::endl;
    return {};
  }
  std::string id = yytext;
  next = static_cast<Token>(yylex());
  if (next != EQUAL) {
    std::cerr << "Expected EQUAL to separate ID from EXPR in field assignment"
              << std::endl;
    return {};
  }
  next = static_cast<Token>(yylex());
  return FieldAssignment{id, ParseExpr(next)};
}

void ParseFieldList(Token separator, Token end, Token& next,
                    std::vector<FieldAssignment>& fields) {
  ParseList(ParseFieldAssignment, separator, end, next, fields);
}

std::unique_ptr<Expr> ParseExprWithExpr(Token& next,
                                        std::unique_ptr<Expr> expr) {
  static std::vector<Token> ops = {PLUS, MINUS, STAR, SLASH, EQUAL, NE,
                                   LT,   GT,    LE,   GE,    AND,   OR};
  auto i = std::find(ops.begin(), ops.end(), next);
  if (i == ops.end()) return std::move(expr);
  Token op = next;
  next = static_cast<Token>(yylex());
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
  case LPAREN: {
    next = static_cast<Token>(yylex());
    std::vector<std::unique_ptr<Expr>> arguments;
    ParseList(ParseExpr, COMMA, RPAREN, next, arguments);
    return std::make_unique<Expr>(FunctionCall{id, std::move(arguments)});
  }
  case LBRACE: {
    next = static_cast<Token>(yylex());
    std::vector<FieldAssignment> fields;
    ParseList(ParseFieldAssignment, COMMA, RBRACE, next, fields);
    return std::make_unique<Expr>(RecordLiteral{id, std::move(fields)});
  }
  case LBRACKET: {
    next = static_cast<Token>(yylex());
    std::unique_ptr<Expr> expr = ParseExpr(next);
    if (next != RBRACKET) {
      std::cerr << "Expected `]` in array element" << std::endl;
      return {};
    }
    next = static_cast<Token>(yylex());
    if (next == OF) {
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
  if (next != IDENTIFIER) {
    std::cerr << "Expected identifer in type field declaration" << std::endl;
    return {};
  }
  TypeField result{std::string(yytext)};
  next = static_cast<Token>(yylex());
  if (next != IDENTIFIER) {
    std::cerr << "Expected identifer in type field declaration" << std::endl;
    return {};
  }
  result.type_id = yytext;
  next = static_cast<Token>(yylex());
  return result;
}

TypeDeclaration ParseTypeDeclaration(Token next) {
  next = static_cast<Token>(yylex());
  if (next != IDENTIFIER) {
    std::cerr << "Expected identifer in type declaration" << std::endl;
    return {};
  }
  TypeDeclaration result{yytext};
  next = static_cast<Token>(yylex());
  switch (next) {
  case IDENTIFIER:
    result.value = std::string(yytext);
    next = static_cast<Token>(yylex());
    return result;
  case LBRACE: {
    std::vector<TypeField> fields;
    ParseList(ParseTypeField, COMMA, RBRACE, next, fields);
    result.value = std::move(fields);
    return result;
  }
  case ARRAY:
    next = static_cast<Token>(yylex());
    if (next != OF) {
      std::cerr << "Expected OF in array type declaration" << std::endl;
      return {};
    }
    next = static_cast<Token>(yylex());
    if (next != IDENTIFIER) {
      std::cerr << "Expected identifer in array type declaration" << std::endl;
      return {};
    }
    result.value = ArrayType{yytext};
    next = static_cast<Token>(yylex());
    return result;
  default:
    std::cerr
        << "Expected one of IDENTIFIER, LBRACE, or ARRAY in type declaration"
        << std::endl;
    return {};
  }
}

VariableDeclaration ParseVariableDeclaration(Token next) {
  next = static_cast<Token>(yylex());
  if (next != IDENTIFIER) {
    std::cerr << "Expected identifer in type declaration" << std::endl;
    return {};
  }
  VariableDeclaration result{yytext};
  next = static_cast<Token>(yylex());
  if (next == COLON) {
    next = static_cast<Token>(yylex());
    if (next != IDENTIFIER) {
      std::cerr << "Expected identifer in variable declaration after COLON"
                << std::endl;
      return {};
    }
    result.type_id = yytext;
    next = static_cast<Token>(yylex());
  }
  if (next != ASSIGN) {
    std::cerr << "Expected ASSIGN in type declaration" << std::endl;
    return {};
  }
  next = static_cast<Token>(yylex());
  result.value = ParseExpr(next);
  return result;
}

FunctionDeclaration ParseFunctionDeclaration(Token next) {
  next = static_cast<Token>(yylex());
  if (next != IDENTIFIER) {
    std::cerr << "Expected identifer in function declaration" << std::endl;
    return {};
  }
  FunctionDeclaration result{yytext};
  next = static_cast<Token>(yylex());
  if (next != LPAREN) {
    std::cerr << "Expected LPAREN in function declaration" << std::endl;
    return {};
  }
  next = static_cast<Token>(yylex());
  ParseList(ParseTypeField, COMMA, RPAREN, next, result.parameter);
  if (next == COLON) {
    next = static_cast<Token>(yylex());
    if (next != IDENTIFIER) {
      std::cerr << "Expected type-id after COLON in function declaration"
                << std::endl;
      return {};
    }
    result.type_id = yytext;
    next = static_cast<Token>(yylex());
  }
  result.body = ParseExpr(next);
  return result;
}

void ParseDeclarationList(Token& next, std::vector<Declaration>& list) {
  while (next != IN) {
    switch (next) {
    case TYPE:
      list.emplace_back(ParseTypeDeclaration(next));
    case VAR:
      list.emplace_back(ParseVariableDeclaration(next));
    case FUNCTION:
      list.emplace_back(ParseFunctionDeclaration(next));
    default:
      std::cerr << "Expected one of TYPE, VAR, or FUNCTION in declaration list"
                << std::endl;
    }
  }
}

std::unique_ptr<Expr> ParseExpr(Token& next) {
  switch (next) {
  case END_OF_FILE:
    return {};
  case STRING_CONSTANT: {
    StringConstant text = yytext;
    next = static_cast<Token>(yylex());
    return ParseExprWithExpr(next, std::make_unique<Expr>(text));
  }
  case NUMBER: {
    long n = strtol(yytext, NULL, 10);
    if (!(INT_MIN <= n && n <= INT_MAX && errno != ERANGE)) {
      return {};
    }
    next = static_cast<Token>(yylex());
    return ParseExprWithExpr(
        next, std::make_unique<Expr>(static_cast<IntegerConstant>(n)));
  }
  case NIL:
    next = static_cast<Token>(yylex());
    return ParseExprWithExpr(next, std::make_unique<Expr>(Nil()));
  case IDENTIFIER: {
    Identifier id = yytext;
    next = static_cast<Token>(yylex());
    return ParseExprWithExpr(next, ParseExprWithId(next, id));
  }
  case MINUS: {
    next = static_cast<Token>(yylex());
    return ParseExprWithExpr(next,
                             std::make_unique<Expr>(Negated{ParseExpr(next)}));
  }
  case LPAREN: {
    next = static_cast<Token>(yylex());
    std::vector<std::unique_ptr<Expr>> exprs;
    ParseList(ParseExpr, COMMA, RPAREN, next, exprs);
    return ParseExprWithExpr(
        next, std::make_unique<Expr>(Parenthesized{std::move(exprs)}));
  }
  case IF: {
    next = static_cast<Token>(yylex());
    auto cond = ParseExpr(next);
    if (next != THEN) {
      std::cerr << "Expected THEN after condition in IF expression"
                << std::endl;
      return {};
    }
    next = static_cast<Token>(yylex());
    auto then_expr = ParseExpr(next);
    if (next != ELSE) {
      return std::make_unique<Expr>(
          IfThen{std::move(cond), std::move(then_expr)});
    }
    next = static_cast<Token>(yylex());
    return std::make_unique<Expr>(
        IfThenElse{std::move(cond), std::move(then_expr), ParseExpr(next)});
  }
  case WHILE: {
    next = static_cast<Token>(yylex());
    auto cond = ParseExpr(next);
    if (next != DO) {
      std::cerr << "Expected DO after condition in WHILE expr" << std::endl;
      return {};
    }
    next = static_cast<Token>(yylex());
    return std::make_unique<Expr>(While{std::move(cond), ParseExpr(next)});
  }
  case FOR: {
    next = static_cast<Token>(yylex());
    if (next != IDENTIFIER) {
      std::cerr << "Expected IDENTIFIER in FOR expression" << std::endl;
      return {};
    }
    std::string id = yytext;
    next = static_cast<Token>(yylex());
    if (next != ASSIGN) {
      std::cerr << "Expected ASSIGN in FOR expression" << std::endl;
      return {};
    }
    next = static_cast<Token>(yylex());
    auto start = ParseExpr(next);
    if (next != TO) {
      std::cerr << "Expected TO in FOR expression" << std::endl;
      return {};
    }
    next = static_cast<Token>(yylex());
    auto end = ParseExpr(next);
    if (next != DO) {
      std::cerr << "Expected DO in FOR expression" << std::endl;
      return {};
    }
    next = static_cast<Token>(yylex());
    return std::make_unique<Expr>(
        For{id, std::move(start), std::move(end), ParseExpr(next)});
  }
  case BREAK:
    return std::make_unique<Expr>(Break{});
  case LET: {
    next = static_cast<Token>(yylex());
    std::vector<Declaration> declaration;
    ParseDeclarationList(next, declaration);
    if (next != IN) {
      std::cerr << "Expected IN in LET expression" << std::endl;
      return {};
    }
    next = static_cast<Token>(yylex());
    std::vector<std::unique_ptr<Expr>> body;
    ParseList(ParseExpr, SEMICOLON, END, next, body);
    return std::make_unique<Expr>(Let{std::move(declaration), std::move(body)});
  }
  default:
    return {};
  }
}
} // namespace

std::unique_ptr<Expr> Parse() {
  Token next = static_cast<Token>(yylex());
  return ParseExpr(next);
}
} // namespace syntax
