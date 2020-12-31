#include "compiler.h"
#include "emit.h"
#include "instruction.h"
#include <assert.h>
#include <fstream>
#include <vector>

namespace {
using emit::Invocable;
using emit::Program;
using emit::Pushable;

class CompileExpressionVisitor : public ExpressionVisitor {
public:
  CompileExpressionVisitor(Program& program, std::ostream& main_os)
      : program_(program) {
    instruction_streams_.push_back(&main_os);
  }

  virtual bool VisitStringConstant(const std::string& text) {
    pushables_.push_back(program_.DefineStringConstant(text));
    return true;
  }
  virtual bool VisitIntegerConstant(int value) { return true; }
  virtual bool VisitNil() { return true; }
  virtual bool VisitLValue(const LValue& value) { return true; }
  virtual bool VisitNegated(const Expression& value) {
    return value.Accept(*this);
  }
  virtual bool VisitBinary(const Expression& left, BinaryOp op,
                           const Expression& right) {
    return left.Accept(*this) && right.Accept(*this);
  }
  virtual bool VisitAssignment(const LValue& value, const Expression& expr) {
    return expr.Accept(*this);
  }
  virtual bool
  VisitFunctionCall(const std::string& id,
                    const std::vector<std::shared_ptr<Expression>>& args) { 
                      //save the size of pushables for later - will make argument counting easier.
    for (const auto& a : args) a->Accept(*this);

    if (id == "print") {
      // hard coding for now to get something, eventually be an unordered set of
      // the stdlib functions
      if (auto f = program_.LookupLibraryFunction(id); f) {
        assert(!instruction_streams_.empty());
        assert(!pushables_.empty());
        auto& os = **instruction_streams_.rbegin();

        const Pushable* arg = *pushables_.rbegin();
        pushables_.pop_back();
        arg->Push(os);
        f->Invoke(os);
      }
    }
    return true;
  }
  virtual bool
  VisitBlock(const std::vector<std::shared_ptr<Expression>>& exprs) {
    return std::all_of(exprs.begin(), exprs.end(),
                       [this](const auto& arg) { return arg->Accept(*this); });
  }
  virtual bool VisitRecord(const std::string& type_id,
                           const std::vector<FieldValue>& field_values) {
    return true;
  }
  virtual bool VisitArray(const std::string& type_id, const Expression& size,
                          const Expression& value) {
    return size.Accept(*this) && value.Accept(*this);
  }
  virtual bool VisitIfThen(const Expression& condition,
                           const Expression& expr) {
    return condition.Accept(*this) && expr.Accept(*this);
  }
  virtual bool VisitIfThenElse(const Expression& condition,
                               const Expression& then_expr,
                               const Expression& else_expr) {
    return condition.Accept(*this) && then_expr.Accept(*this) &&
           else_expr.Accept(*this);
  }
  virtual bool VisitWhile(const Expression& condition, const Expression& body) {
    return condition.Accept(*this) && body.Accept(*this);
  }
  virtual bool VisitFor(const std::string& id, const Expression& first,
                        const Expression& last, const Expression& body) {
    return first.Accept(*this) && last.Accept(*this) && body.Accept(*this);
  }
  virtual bool VisitBreak() { return true; }
  virtual bool
  VisitLet(const std::vector<std::shared_ptr<Declaration>>& declarations,
           const std::vector<std::shared_ptr<Expression>>& body) {
    return std::all_of(body.begin(), body.end(),
                       [this](const auto& arg) { return arg->Accept(*this); });
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
