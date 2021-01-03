#pragma once
#include "Expression.h"
#include <algorithm>
// Memory management notes. The nodes of the abstract syntax tree own
// their children, in the sense that the parent destructor is
// responsibile for destroying the child. This is delegated to
// unique_ptr or shared_ptr. The constructors take raw pointers and
// adopt them.

// Reference to a named type.
class TypeReference : public Type {
public:
  TypeReference(std::string_view id) : id_(id) {}
  virtual bool Accept(TypeVisitor& visitor) const {
    return visitor.VisitTypeReference(id_);
  }

private:
  std::string id_;
};

class RecordType : public Type {
public:
  RecordType(std::vector<TypeField>&& fields) : fields_(std::move(fields)) {}
  bool Accept(TypeVisitor& visitor) const override {
    return visitor.VisitRecordType(fields_);
  }
  std::optional<const std::string*>
  GetFieldType(const std::string& field_id) const override {
    for (const auto& f : fields_) {
      if (f.id == field_id) return &f.type_id;
    }
    return {};
  }

private:
  std::vector<TypeField> fields_;
};

class ArrayType : public Type {
public:
  ArrayType(std::string_view type_id) : type_id_(type_id) {}
  bool Accept(TypeVisitor& visitor) const override {
    return visitor.VisitArrayType(type_id_);
  }
  std::optional<const std::string*> GetElementType() const override {
    return &type_id_;
  }

private:
  std::string type_id_;
};

class IntType : public Type {
public:
  bool Accept(TypeVisitor& visitor) const override {
    return visitor.VisitInt();
  }
};

class StringType : public Type {
public:
  bool Accept(TypeVisitor& visitor) const override {
    return visitor.VisitString();
  }
};

class TypeDeclaration : public Declaration {
public:
  TypeDeclaration(std::string_view type_id, Type* type)
      : Declaration(type_id), type_(type) {}
  bool Accept(DeclarationVisitor& visitor) const override {
    return visitor.VisitTypeDeclaration(Id(), *type_);
  }
  std::optional<const Type*> GetType() const override { return type_.get(); }

private:
  std::shared_ptr<Type> type_;
};

class VariableDeclaration : public Declaration {
public:
  VariableDeclaration(std::string_view id, Expression* expr)
      : Declaration(id), expr_(expr) {}
  VariableDeclaration(std::string_view id, std::string_view type_id,
                      Expression* expr)
      : Declaration(id), type_id_(type_id), expr_(expr) {}
  bool Accept(DeclarationVisitor& visitor) const override {
    return visitor.VisitVariableDeclaration(Id(), type_id_, *expr_);
  }
  std::vector<TreeNode*> Children() override { return {expr_.get()}; }
  std::optional<const std::string*> GetValueType() const override {
    return type_id_ ? &*type_id_ : &expr_->GetType();
  }

private:
  std::optional<std::string> type_id_;
  std::unique_ptr<Expression> expr_;
};

class ParamDeclaration : public Declaration {
public:
  ParamDeclaration(std::string_view id, const std::string& type_id)
      : Declaration(id), type_id_(type_id) {}
  bool Accept(DeclarationVisitor& visitor) const override { return true; }
  std::optional<const std::string*> GetValueType() const override {
    return &type_id_;
  }

private:
  const std::string& type_id_;
};

class FunctionDeclaration : public Declaration {
public:
  FunctionDeclaration(std::string_view id, std::vector<TypeField>&& params,
                      Expression* body)
      : Declaration(id), params_(std::move(params)), body_(body) {}
  FunctionDeclaration(std::string_view id, std::vector<TypeField>&& params,
                      std::string_view type_id, Expression* body)
      : Declaration(id), type_id_(type_id), params_(std::move(params)),
        body_(body) {}
  bool Accept(DeclarationVisitor& visitor) const override {
    return visitor.VisitFunctionDeclaration(Id(), params_, type_id_, *body_);
  }

  std::vector<TreeNode*> Children() override { return {body_.get()}; }

  std::optional<const NameSpace*>
  GetNonTypeNameSpace(const NameSpace& non_types) const override {
    if (!name_space_) {
      name_space_.reset(new NameSpace(non_types));
      for (const auto& p : params_) {
        param_decls_.emplace_back(p.id, p.type_id);
        (*name_space_)[p.id] = &*param_decls_.rbegin();
      }
    }
    return name_space_.get();
  }
  std::optional<const std::string*> GetValueType() const override {
    return type_id_ ? &*type_id_ : &body_->GetType();
  }

private:
  std::vector<TypeField> params_;
  std::optional<std::string> type_id_;
  std::unique_ptr<Expression> body_;
  mutable std::unique_ptr<NameSpace> name_space_;
  mutable std::vector<ParamDeclaration> param_decls_;
};

class StringConstant : public Expression {
public:
  StringConstant(std::string_view text) : text_(text) {}
  bool Accept(ExpressionVisitor& visitor) const override {
    return visitor.VisitStringConstant(text_);
  }

private:
  std::string text_;
};

class IntegerConstant : public Expression {
public:
  IntegerConstant(int value) : value_(value) {}
  bool Accept(ExpressionVisitor& visitor) const override {
    return visitor.VisitIntegerConstant(value_);
  }

private:
  int value_;
};

class Nil : public Expression {
public:
  Nil() {}
  bool Accept(ExpressionVisitor& visitor) const override {
    return visitor.VisitNil();
  }
};

class IdLValue : public LValue {
public:
  IdLValue(std::string_view id) : id_(id) {}
  bool Accept(LValueVisitor& visitor) const override {
    return visitor.VisitId(id_);
  }
  std::optional<std::string> GetId() const override { return id_; }

private:
  std::string id_;
};

class FieldLValue : public LValue {
public:
  FieldLValue(LValue* value, std::string_view id) : value_(value), id_(id) {}
  bool Accept(LValueVisitor& visitor) const override {
    return visitor.VisitField(*value_, id_);
  }
  std::vector<TreeNode*> Children() override { return {value_.get()}; }

  std::optional<std::string> GetField() const override { return id_; }
  std::optional<const LValue*> GetChild() const override {
    return value_.get();
  }

private:
  std::shared_ptr<LValue> value_;
  std::string id_;
};

class IndexLValue : public LValue {
public:
  IndexLValue(LValue* value, Expression* expr) : value_(value), expr_(expr) {}
  bool Accept(LValueVisitor& visitor) const override {
    return visitor.VisitIndex(*value_, *expr_);
  }
  virtual std::vector<TreeNode*> Children() {
    return {value_.get(), expr_.get()};
  }

  std::optional<const Expression*> GetIndexValue() const override {
    return expr_.get();
  }
  std::optional<const LValue*> GetChild() const override {
    return value_.get();
  }

private:
  std::shared_ptr<LValue> value_;
  std::unique_ptr<Expression> expr_;
};

class Negated : public Expression {
public:
  Negated(Expression* expr) : expr_(expr) {}
  bool Accept(ExpressionVisitor& visitor) const override {
    return visitor.VisitNegated(*expr_);
  }
  std::vector<TreeNode*> Children() override { return {expr_.get()}; }

private:
  std::unique_ptr<Expression> expr_;
};

class Binary : public Expression {
public:
  Binary(Expression* left, BinaryOp op, Expression* right)
      : left_(left), op_(op), right_(right) {}
  bool Accept(ExpressionVisitor& visitor) const override {
    return visitor.VisitBinary(*left_, op_, *right_);
  }
  std::vector<TreeNode*> Children() override {
    return {left_.get(), right_.get()};
  }

private:
  std::unique_ptr<Expression> left_;
  BinaryOp op_;
  std::unique_ptr<Expression> right_;
};

class Assignment : public Expression {
public:
  Assignment(std::shared_ptr<LValue> value, Expression* expr)
      : value_(value), expr_(expr) {}
  bool Accept(ExpressionVisitor& visitor) const override {
    return visitor.VisitAssignment(*value_, *expr_);
  }
  std::vector<TreeNode*> Children() override {
    return {value_.get(), expr_.get()};
  }

private:
  std::shared_ptr<LValue> value_;
  std::unique_ptr<Expression> expr_;
};

class FunctionCall : public Expression {
public:
  FunctionCall(std::string_view id,
               std::vector<std::shared_ptr<Expression>>&& args)
      : id_(id), args_(std::move(args)) {}
  bool Accept(ExpressionVisitor& visitor) const override {
    return visitor.VisitFunctionCall(id_, args_);
  }
  std::vector<TreeNode*> Children() override {
    std::vector<TreeNode*> children;
    for (auto& c : args_) children.push_back(c.get());
    return children;
  }

private:
  std::string id_;
  std::vector<std::shared_ptr<Expression>> args_;
};

// From syntax for bracketed sequence `( expr-seq_opt )`.
class Block : public Expression {
public:
  Block(std::vector<std::shared_ptr<Expression>>&& exprs)
      : exprs_(std::move(exprs)) {}
  bool Accept(ExpressionVisitor& visitor) const override {
    return visitor.VisitBlock(exprs_);
  }
  std::vector<TreeNode*> Children() override {
    std::vector<TreeNode*> children;
    for (auto& c : exprs_) children.push_back(c.get());
    return children;
  }

private:
  std::vector<std::shared_ptr<Expression>> exprs_;
};

// Record literal
class Record : public Expression {
public:
  Record(std::string_view type_id, std::vector<FieldValue>&& field_values)
      : type_id_(type_id), field_values_(std::move(field_values)) {}
  bool Accept(ExpressionVisitor& visitor) const override {
    return visitor.VisitRecord(type_id_, field_values_);
  }
  std::vector<TreeNode*> Children() override {
    std::vector<TreeNode*> children;
    for (auto& f : field_values_) children.push_back(f.expr.get());
    return children;
  }

private:
  std::string type_id_;
  std::vector<FieldValue> field_values_;
};

// Array literal
class Array : public Expression {
public:
  Array(std::string_view type_id, Expression* size, Expression* value)
      : type_id_(type_id), size_(size), value_(value) {}
  bool Accept(ExpressionVisitor& visitor) const override {
    return visitor.VisitArray(type_id_, *size_, *value_);
  }
  std::vector<TreeNode*> Children() override {
    return {size_.get(), value_.get()};
  }

private:
  std::string type_id_;
  std::unique_ptr<Expression> size_;
  std::unique_ptr<Expression> value_;
};

class IfThen : public Expression {
public:
  IfThen(Expression* condition, Expression* expr)
      : condition_(condition), expr_(expr) {}
  bool Accept(ExpressionVisitor& visitor) const override {
    return visitor.VisitIfThen(*condition_, *expr_);
  }
  std::vector<TreeNode*> Children() override {
    return {condition_.get(), expr_.get()};
  }

private:
  std::unique_ptr<Expression> condition_;
  std::unique_ptr<Expression> expr_;
};

class IfThenElse : public Expression {
public:
  IfThenElse(Expression* condition, Expression* then_expr,
             Expression* else_expr)
      : condition_(condition), then_expr_(then_expr), else_expr_(else_expr) {}
  bool Accept(ExpressionVisitor& visitor) const override {
    return visitor.VisitIfThenElse(*condition_, *then_expr_, *else_expr_);
  }
  std::vector<TreeNode*> Children() override {
    return {condition_.get(), then_expr_.get(), else_expr_.get()};
  }

private:
  std::unique_ptr<Expression> condition_;
  std::unique_ptr<Expression> then_expr_;
  std::unique_ptr<Expression> else_expr_;
};

class While : public Expression {
public:
  While(Expression* condition, Expression* body)
      : condition_(condition), body_(body) {}
  bool Accept(ExpressionVisitor& visitor) const override {
    return visitor.VisitWhile(*condition_, *body_);
  }
  std::vector<TreeNode*> Children() override {
    return {condition_.get(), body_.get()};
  }

private:
  std::unique_ptr<Expression> condition_;
  std::unique_ptr<Expression> body_;
};

class For : public Expression {
public:
  For(std::string_view id, Expression* first, Expression* last,
      Expression* body)
      : id_(id), first_(first), last_(last), body_(body) {}
  bool Accept(ExpressionVisitor& visitor) const override {
    return visitor.VisitFor(id_, *first_, *last_, *body_);
  }
  std::vector<TreeNode*> Children() override {
    return {first_.get(), last_.get(), body_.get()};
  }

private:
  std::string id_;
  std::unique_ptr<Expression> first_;
  std::unique_ptr<Expression> last_;
  std::unique_ptr<Expression> body_;
};

class Break : public Expression {
public:
  Break() {}
  bool Accept(ExpressionVisitor& visitor) const override {
    return visitor.VisitBreak();
  }
};

class Let : public Expression {
public:
  Let(std::vector<std::shared_ptr<Declaration>>&& declarations,
      std::vector<std::shared_ptr<Expression>>&& body)
      : declarations_(std::move(declarations)), body_(std::move(body)) {}
  bool Accept(ExpressionVisitor& visitor) const override {
    return visitor.VisitLet(declarations_, body_);
  }
  std::vector<TreeNode*> Children() override {
    std::vector<TreeNode*> children;
    for (auto& c : declarations_) children.push_back(c.get());
    for (auto& c : body_) children.push_back(c.get());
    return children;
  }

  virtual std::optional<const NameSpace*>
  GetTypeNameSpace(const NameSpace& types) {
    my_types_.reset(new NameSpace(types));
    for (const auto& d : declarations_) {
      if (d->GetType()) (*my_types_)[d->Id()] = d.get();
    }
    return my_types_.get();
  }

  // Returns new scope for a Let expression
  virtual std::optional<const NameSpace*>
  GetNonTypeNameSpace(const NameSpace& non_types) const {
    my_non_types_.reset(new NameSpace(non_types));
    for (const auto& d : declarations_) {
      if (!d->GetType()) (*my_non_types_)[d->Id()] = d.get();
    }
    return my_non_types_.get();
  }

private:
  std::vector<std::shared_ptr<Declaration>> declarations_;
  std::vector<std::shared_ptr<Expression>> body_;
  mutable std::unique_ptr<NameSpace> my_types_;
  mutable std::unique_ptr<NameSpace> my_non_types_;
};
