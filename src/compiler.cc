#include "compiler.h"
#include "emit.h"
#include "instruction.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <vector>

namespace {
using emit::Invocable;
using emit::Program;
using emit::Pushable;

bool CheckFunctionArgs(const Declaration& declaration,
                       const std::vector<std::shared_ptr<Expression>>& args) {
  return true;
}

class CompileExpressionVisitor : public ExpressionVisitor {
public:
  CompileExpressionVisitor(Program& program, std::ostream& main_os)
      : program_(program) {
    instruction_streams_.push_back(&main_os);
  }
  bool VisitStringConstant(const std::string& text) override {
    pushables_.push_back(program_.DefineStringConstant(text));
    return true;
  }
  bool VisitIntegerConstant(int value) override {
    pushables_.push_back(program_.DefineIntegerConstant(value));
    return true;
  }
  bool VisitNil() override { return true; }
  bool VisitLValue(const LValue& value) override { return true; }
  bool VisitNegated(const Expression& value) override {
    return value.Accept(*this);
  }
  bool VisitBinary(const Expression& left, BinaryOp op,
                   const Expression& right) override {
    return left.Accept(*this) && right.Accept(*this);
  }
  bool VisitAssignment(const LValue& value, const Expression& expr) override {
    return expr.Accept(*this);
  }
  bool VisitFunctionCall(const std::string& id,
                         const std::vector<std::shared_ptr<Expression>>& args,
                         const Expression& exp) override {
    auto d = exp.GetNonTypeNameSpace().Lookup(id);
    if (!d) {
      std::cerr << "No declaration for function " << id << std::endl;
      return false;
    }
    if (!CheckFunctionArgs(**d, args)) return false;

    // save the size of pushables for later - will make argument counting
    // easier.
    for (const auto& a : args) a->Accept(*this);

    if (auto f = program_.LookupLibraryFunction(id); f) {
      return EmitFunctionCall(*f, args.size());
    }
    std::cerr << "Non-library function calls not yet implemented";
    return true;
  }
  bool
  VisitBlock(const std::vector<std::shared_ptr<Expression>>& exprs) override {
    return std::all_of(exprs.begin(), exprs.end(),
                       [this](const auto& arg) { return arg->Accept(*this); });
  }
  bool VisitRecord(const std::string& type_id,
                   const std::vector<FieldValue>& field_values,
                   const Expression& exp) override {
    return true;
  }
  bool VisitArray(const std::string& type_id, const Expression& size,
                  const Expression& value) override {
    return size.Accept(*this) && value.Accept(*this);
  }
  bool VisitIfThen(const Expression& condition,
                   const Expression& expr) override {
    return condition.Accept(*this) && expr.Accept(*this);
  }
  bool VisitIfThenElse(const Expression& condition, const Expression& then_expr,
                       const Expression& else_expr) override {
    return condition.Accept(*this) && then_expr.Accept(*this) &&
           else_expr.Accept(*this);
  }
  bool VisitWhile(const Expression& condition,
                  const Expression& body) override {
    return condition.Accept(*this) && body.Accept(*this);
  }
  bool VisitFor(const std::string& id, const Expression& first,
                const Expression& last, const Expression& body) override {
    return first.Accept(*this) && last.Accept(*this) && body.Accept(*this);
  }
  bool VisitBreak() override { return true; }
  bool VisitLet(const std::vector<std::shared_ptr<Declaration>>& declarations,
                const std::vector<std::shared_ptr<Expression>>& body) override {
    return std::all_of(body.begin(), body.end(),
                       [this](const auto& arg) { return arg->Accept(*this); });
  }

  bool EmitFunctionCall(const Invocable& invocable, int arg_count) {
    assert(!instruction_streams_.empty());
    auto& os = **instruction_streams_.rbegin();
    while (--arg_count >= 0) {
      assert(!pushables_.empty());
      const Pushable* arg = *pushables_.rbegin();
      pushables_.pop_back();
      arg->Push(os);
    }
    invocable.Invoke(os);
    return true;
  }

  Program& program_;
  std::vector<const Pushable*> pushables_;
  std::vector<std::ostream*> instruction_streams_;
};
} // namespace

// Given a tiger expression, create a java class file to execute.
void Compile(const Expression& e) {
  auto program = Program::JavaProgram();
  std::ostringstream main_os;
  CompileExpressionVisitor visitor(*program, main_os);
  e.Accept(visitor);
  main_os.put(Instruction::_return);
  program->DefineFunction(emit::ACC_PUBLIC | emit::ACC_STATIC, "main",
                          "([Ljava/lang/String;)V", main_os.str());
  std::ofstream out("/tmp/Main.class");
  program->Emit(out);
}
