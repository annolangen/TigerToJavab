#pragma once
#include <ostream>

#include "syntax.h"

std::ostream& operator<<(std::ostream& os, const syntax::ArrayElement&);
std::ostream& operator<<(std::ostream& os, const syntax::ArrayLiteral&);
std::ostream& operator<<(std::ostream& os, const syntax::ArrayType&);
std::ostream& operator<<(std::ostream& os, const syntax::Assignment&);
std::ostream& operator<<(std::ostream& os, const syntax::Binary&);
std::ostream& operator<<(std::ostream& os, const syntax::Break&);
std::ostream& operator<<(std::ostream& os, const syntax::Declaration&);
std::ostream& operator<<(std::ostream& os, const syntax::Expr&);
std::ostream& operator<<(std::ostream& os, const syntax::FieldAssignment&);
std::ostream& operator<<(std::ostream& os, const syntax::For&);
std::ostream& operator<<(std::ostream& os, const syntax::FunctionCall&);
std::ostream& operator<<(std::ostream& os, const syntax::FunctionDeclaration&);
std::ostream& operator<<(std::ostream& os, const syntax::IfThen&);
std::ostream& operator<<(std::ostream& os, const syntax::IfThenElse&);
std::ostream& operator<<(std::ostream& os, const syntax::Let&);
std::ostream& operator<<(std::ostream& os, const syntax::LValue&);
std::ostream& operator<<(std::ostream& os, const syntax::Negated&);
std::ostream& operator<<(std::ostream& os, const syntax::Nil&);
std::ostream& operator<<(std::ostream& os, const syntax::Parenthesized&);
std::ostream& operator<<(std::ostream& os, const syntax::RecordField&);
std::ostream& operator<<(std::ostream& os, const syntax::RecordLiteral&);
std::ostream& operator<<(std::ostream& os, const syntax::StringConstant&);
std::ostream& operator<<(std::ostream& os, const syntax::Type&);
std::ostream& operator<<(std::ostream& os, const syntax::TypeDeclaration&);
std::ostream& operator<<(std::ostream& os, const syntax::TypeField&);
std::ostream& operator<<(std::ostream& os, const syntax::TypeFields&);
std::ostream& operator<<(std::ostream& os, const syntax::VariableDeclaration&);
std::ostream& operator<<(std::ostream& os, const syntax::While&);

std::ostream& operator<<(std::ostream& os,
                         const std::unique_ptr<syntax::LValue>&);
