#include "emit.h"
#include <functional>
#include <optional>
#include <unordered_map>

// Implementation following
// https://docs.oracle.com/javase/specs/jvms/se7/html/jvms-4.html and example
// testdata/Main.class.
namespace emit {
namespace {
#include "instruction.h"

using u1 = uint8_t;
using u2 = uint16_t;
using u4 = uint32_t;

// Put two bytes MSB order
inline void Put2(std::ostream& os, u2 v) {
  os.put(v >> 8);
  os.put(v & 255);
}

// Put 4 bytes MSB order
inline void Put4(std::ostream& os, u4 v) {
  Put2(os, v >> 16);
  Put2(os, v & 0xffff);
}

struct CodeAttribute;

// https://docs.oracle.com/javase/specs/jvms/se7/html/jvms-4.html#jvms-4.7
struct AttributeInfo {
  enum Tag {
    kConstantValue,
    kCode,
    kStackMapTable,
    kExceptions,
    kInnerClasses,
    kEnclosingMethod,
    kSynthetic,
    kSignature,
    kSourceFile,
    kSourceDebugExtension,
    kLineNumberTable,
    kLocalVariableTable,
    kLocalVariableTypeTable,
    kDeprecated,
    kRuntimeVisibleAnnotations,
    kRuntimeInvisibleAnnotations,
    kRuntimeVisibleParameterAnnotations,
    kRuntimeInvisibleParameterAnnotations,
    kAnnotationDefault,
    kBootstrapMethods
  };
  u2 attribute_name_index;
  void Emit(std::ostream& os) const {
    Put2(os, attribute_name_index);
    std::string info_bytes = InfoBytes();
    Put4(os, info_bytes.length());
    os.write(info_bytes.data(), info_bytes.length());
  }
  // Emit length and info bytes
  virtual std::string InfoBytes() const = 0;
  virtual Tag tag() const = 0;
  virtual std::optional<CodeAttribute*> code() { return {}; }
};

struct CodeAttribute : AttributeInfo {
  u2 max_stack;
  u2 max_locals;
  std::string code_bytes;
  std::vector<std::unique_ptr<AttributeInfo>> attributes;

  std::string InfoBytes() const override {
    std::ostringstream os;
    Put2(os, max_stack);
    Put2(os, max_locals);
    Put4(os, code_bytes.length());
    os.write(code_bytes.data(), code_bytes.length());
    Put2(os, 0); // exception table length
    Put2(os, attributes.size());
    for (const auto& a : attributes) a->Emit(os);
    return os.str();
  }
  virtual Tag tag() const override { return AttributeInfo::kCode; }
  virtual std::optional<CodeAttribute*> code() { return this; }
};

// https://docs.oracle.com/javase/specs/jvms/se7/html/jvms-4.html#jvms-4.6
struct MethodInfo {
  u2 access_flags;
  u2 name_index;
  u2 descriptor_index;
  std::vector<AttributeInfo> attributes;
  void Emit(std::ostream& os) const {
    Put2(os, access_flags);
    Put2(os, name_index);
    Put2(os, descriptor_index);
    Put2(os, attributes.size());
    for (const auto& a : attributes) a.Emit(os);
  }
};

struct Utf8Constant;
struct StringConstant;
struct ClassConstant;
struct MethodRefConstant;
struct NameAndTypeConstant;

// Items in constant pool
// https://docs.oracle.com/javase/specs/jvms/se7/html/jvms-4.html#jvms-4.4.
struct Constant {
  u2 index;
  virtual ~Constant() = default;
  virtual void Emit(std::ostream& os) const = 0;
  enum Tag {
    kUtf8 = 1,
    kInteger = 3,
    kFloat = 4,
    kLong = 5,
    kDouble = 6,
    kClass = 7,
    kString = 8,
    kFieldref = 9,
    kMethodref = 10,
    kInterfaceMethodref = 11,
    kNameAndType = 12,
    kMethodHandle = 15,
    kMethodType = 16,
    kInvokeDynamic = 18
  };
  virtual Tag tag() const = 0;

  // Match methods help find an instance based on member data
  virtual bool Matches(std::string_view text) const { return false; }
  virtual bool Matches(u2 index) const { return false; }
  virtual bool Matches(u2 first, u2 second) const { return false; }

  // Safe cast by virtual method
  virtual std::optional<Utf8Constant*> utf8() { return {}; }
  virtual std::optional<StringConstant*> string() { return {}; }
  virtual std::optional<ClassConstant*> clazz() { return {}; }
  virtual std::optional<MethodRefConstant*> methodRef() { return {}; }
  virtual std::optional<NameAndTypeConstant*> nameAndType() { return {}; }
};

struct Ref : Constant {
  u2 class_index;
  u2 name_and_type_index;
  bool Matches(u2 first, u2 second) const override {
    return class_index == first && name_and_type_index == second;
  }
  void Emit(std::ostream& os) const override {
    os.put(tag());
    Put2(os, class_index);
    Put2(os, name_and_type_index);
  }
};

struct MethodRefConstant : Ref {
  Tag tag() const override { return kMethodref; }
  std::optional<MethodRefConstant*> methodRef() override { return this; }
};

struct StringConstant : Constant, Pushable {
  u2 string_index;
  Tag tag() const override { return kString; }
  bool Matches(u2 index) const override { return index == string_index; }
  void Emit(std::ostream& os) const override {
    os.put(tag());
    Put2(os, string_index);
  }

  void Push(CodeBlock& block) const override {
    if (string_index < 256) {
      block.bytes.put(Instruction::_ldc);
      block.bytes.put(string_index);
    } else {
      block.bytes.put(Instruction::_ldc_w);
      Put2(block.bytes, string_index);
    }
  }
};

struct ClassConstant : Constant {
  u2 name_index;
  Tag tag() const override { return kClass; }
  bool Matches(u2 index) const override { return index == name_index; }
  void Emit(std::ostream& os) const override {
    os.put(tag());
    Put2(os, name_index);
  }
};

struct Utf8Constant : Constant {
  std::string text;
  Tag tag() const override { return kUtf8; }
  void Emit(std::ostream& os) const override {
    os.put(tag());
    u2 length = text.length();
    Put2(os, length);
    os.write(text.data(), length);
  }
};

struct NameAndTypeConstant : Constant {
  u2 name_index;
  u2 descriptor_index;
  Tag tag() const override { return kNameAndType; }
  bool Matches(u2 first, u2 second) const override { return false; }
  std::optional<NameAndTypeConstant*> nameAndType() override { return this; }
  void Emit(std::ostream& os) const override {
    os.put(tag());
    Put2(os, name_index);
    Put2(os, descriptor_index);
  }
};

class FunctionCodeBlock : public CodeBlock {
  CodeAttribute codeAttribute(u2 code_name_index) {
    CodeAttribute result;
    result.attribute_name_index = code_name_index;
    result.code_bytes = bytes.str();
    // TODO compute from code_bytes
    result.max_stack = 10;
    result.max_locals = 10;
    return result;
  }
};

class LibraryFunction : public Invocable {};

const std::unordered_map<std::string_view, const char*>
    kTypeByLibraryFunctionName = {{"print", "(Ljava/lang/String;)V"}};

std::optional<const char*> LibraryFunctionType(std::string_view name) {
  if (auto found = kTypeByLibraryFunctionName.find(name);
      found != kTypeByLibraryFunctionName.end()) {
    return found->second;
  }
  return {};
}

class JvmFunction : public Function {
public:
  JvmFunction() : code_block_(std::make_unique<FunctionCodeBlock>()) {}
  void Invoke(CodeBlock& block) const override {}
  CodeBlock* codeBlock() override { return code_block_.get(); }
  std::unique_ptr<FunctionCodeBlock> code_block_;
};

struct JvmProgram : Program {
  JvmProgram() : main(std::make_unique<FunctionCodeBlock>()) {}
  ~JvmProgram() = default;
  CodeBlock* GetMainCodeBlock() override { return main.get(); }
  const Pushable* DefineStringConstant(std::string_view text) override {
    return stringConstant(text);
  }

  const Invocable* LookupLibraryFunction(std::string_view name) override {
    return nullptr;
  }

  JvmFunction* DefineFunction(u2 flags, std::string_view name,
                              std::string_view descriptor) override {
    methods.push_back(methodInfo(flags, name, descriptor));
    functions.emplace_back();
    return &*functions.rbegin();
  };

  void DefineConstructor() {
    JvmFunction* init_function = DefineFunction(0, "<init>", "()V");
    FunctionCodeBlock* block = init_function->code_block_.get();
    std::ostream& os=    block->bytes;
    os.put(Instruction::_aload_0);
    os.put(Instruction::_invokespecial);
    Put2(os, methodRefConstant("java/lang/Object", "<init>", "()V")->index);
    os.put(Instruction::_return);    
  }

  void Emit(std::ostream& os) override {
    DefineConstructor();
    u2 this_class = classConstant("Main")->index;
    u2 super_class = classConstant("java/lang/Object")->index;


    Put4(os, 0xcafebabe);
    Put2(os, 0);  // minor version
    Put2(os, 55); // major version
    Put2(os, constant_pool.size() + 1);
    for (const auto& c : constant_pool) c->Emit(os);
    Put2(os, 0x20); // flags
    Put2(os, this_class);
    Put2(os, super_class);
    Put2(os, 0); // interfaces count
    Put2(os, 0); // field count
    Put2(os, methods.size());
    for (const auto& m : methods) m.Emit(os);
    Put2(os, 0); // attributes count
  }

  template <class T> T* Adopt(T* t) {
    t->index = 1 + constant_pool.size();
    constant_pool.emplace_back(t);
    return t;
  }

  Utf8Constant* utf8Constant(std::string_view text) {
    for (auto& c : constant_pool) {
      if (c->tag() == ClassConstant::kUtf8 && c->Matches(text))
        return *c->utf8();
    }
    Utf8Constant* result = Adopt(new Utf8Constant());
    result->text = text;
    return result;
  }

  StringConstant* stringConstant(std::string_view text) {
    Utf8Constant* utf8 = utf8Constant(text);
    for (auto& c : constant_pool) {
      if (c->tag() == ClassConstant::kString && c->Matches(utf8->index)) {
        return *c->string();
      }
    }
    StringConstant* result = Adopt(new StringConstant());
    result->string_index = utf8->index;
    return result;
  }

  ClassConstant* classConstant(std::string_view class_name) {
    u2 name_index = utf8Constant(class_name)->index;
    for (auto& c : constant_pool) {
      if (c->tag() == ClassConstant::kClass && c->Matches(name_index)) {
        return *c->clazz();
      }
    }
    ClassConstant* result = Adopt(new ClassConstant());
    result->name_index = name_index;
    return result;
  }

  NameAndTypeConstant* nameAndTypeConstant(std::string_view name,
                                           std::string_view descriptor) {
    u2 name_index = utf8Constant(name)->index;
    u2 descriptor_index = utf8Constant(descriptor)->index;
    for (auto& c : constant_pool) {
      if (c->tag() == ClassConstant::kNameAndType &&
          c->Matches(name_index, descriptor_index)) {
        return *c->nameAndType();
      }
    }
    NameAndTypeConstant* result = Adopt(new NameAndTypeConstant());
    result->name_index = name_index;
    result->descriptor_index = descriptor_index;
    return result;
  }

  MethodRefConstant* methodRefConstant(std::string_view class_name,
                                       std::string_view name,
                                       std::string_view type) {
    u2 class_index = classConstant(class_name)->index;
    u2 name_and_type_index = nameAndTypeConstant(name, type)->index;
    for (auto& c : constant_pool) {
      if (c->tag() == ClassConstant::kMethodref &&
          c->Matches(class_index, name_and_type_index)) {
        return *c->methodRef();
      }
    }
    MethodRefConstant* result = Adopt(new MethodRefConstant());
    result->class_index = class_index;
    result->name_and_type_index = name_and_type_index;
    return result;
  }

  MethodInfo methodInfo(u2 flags, std::string_view name,
                        std::string_view descriptor) {
    return {flags, stringConstant(name)->index,
            stringConstant(descriptor)->index};
  }

  std::unique_ptr<CodeBlock> main;
  std::vector<std::unique_ptr<Constant>> constant_pool;
  std::vector<MethodInfo> methods;
  std::vector<JvmFunction> functions;
};
} // namespace emit

std::unique_ptr<Program> Program::JavaProgram() {
  return std::make_unique<JvmProgram>();
}

} // namespace emit
