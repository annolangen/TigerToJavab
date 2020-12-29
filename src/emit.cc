#include "emit.h"
#include <functional>
#include <optional>
#include <unordered_map>

// Implementation following
// https://docs.oracle.com/javase/specs/jvms/se7/html/jvms-4.html and example
// testdata/Main.class.
namespace emit {
namespace {

using u1 = uint8_t;
using u2 = uint16_t;
using u4 = uint32_t;

// See https://en.wikipedia.org/wiki/Java_bytecode_instruction_listings
enum Code {
  _illegal = -1,

  // Java bytecodes
  _nop = 0,               // 0x00
  _aconst_null = 1,       // 0x01
  _iconst_m1 = 2,         // 0x02
  _iconst_0 = 3,          // 0x03
  _iconst_1 = 4,          // 0x04
  _iconst_2 = 5,          // 0x05
  _iconst_3 = 6,          // 0x06
  _iconst_4 = 7,          // 0x07
  _iconst_5 = 8,          // 0x08
  _lconst_0 = 9,          // 0x09
  _lconst_1 = 10,         // 0x0a
  _fconst_0 = 11,         // 0x0b
  _fconst_1 = 12,         // 0x0c
  _fconst_2 = 13,         // 0x0d
  _dconst_0 = 14,         // 0x0e
  _dconst_1 = 15,         // 0x0f
  _bipush = 16,           // 0x10
  _sipush = 17,           // 0x11
  _ldc = 18,              // 0x12
  _ldc_w = 19,            // 0x13
  _ldc2_w = 20,           // 0x14
  _iload = 21,            // 0x15
  _lload = 22,            // 0x16
  _fload = 23,            // 0x17
  _dload = 24,            // 0x18
  _aload = 25,            // 0x19
  _iload_0 = 26,          // 0x1a
  _iload_1 = 27,          // 0x1b
  _iload_2 = 28,          // 0x1c
  _iload_3 = 29,          // 0x1d
  _lload_0 = 30,          // 0x1e
  _lload_1 = 31,          // 0x1f
  _lload_2 = 32,          // 0x20
  _lload_3 = 33,          // 0x21
  _fload_0 = 34,          // 0x22
  _fload_1 = 35,          // 0x23
  _fload_2 = 36,          // 0x24
  _fload_3 = 37,          // 0x25
  _dload_0 = 38,          // 0x26
  _dload_1 = 39,          // 0x27
  _dload_2 = 40,          // 0x28
  _dload_3 = 41,          // 0x29
  _aload_0 = 42,          // 0x2a
  _aload_1 = 43,          // 0x2b
  _aload_2 = 44,          // 0x2c
  _aload_3 = 45,          // 0x2d
  _iaload = 46,           // 0x2e
  _laload = 47,           // 0x2f
  _faload = 48,           // 0x30
  _daload = 49,           // 0x31
  _aaload = 50,           // 0x32
  _baload = 51,           // 0x33
  _caload = 52,           // 0x34
  _saload = 53,           // 0x35
  _istore = 54,           // 0x36
  _lstore = 55,           // 0x37
  _fstore = 56,           // 0x38
  _dstore = 57,           // 0x39
  _astore = 58,           // 0x3a
  _istore_0 = 59,         // 0x3b
  _istore_1 = 60,         // 0x3c
  _istore_2 = 61,         // 0x3d
  _istore_3 = 62,         // 0x3e
  _lstore_0 = 63,         // 0x3f
  _lstore_1 = 64,         // 0x40
  _lstore_2 = 65,         // 0x41
  _lstore_3 = 66,         // 0x42
  _fstore_0 = 67,         // 0x43
  _fstore_1 = 68,         // 0x44
  _fstore_2 = 69,         // 0x45
  _fstore_3 = 70,         // 0x46
  _dstore_0 = 71,         // 0x47
  _dstore_1 = 72,         // 0x48
  _dstore_2 = 73,         // 0x49
  _dstore_3 = 74,         // 0x4a
  _astore_0 = 75,         // 0x4b
  _astore_1 = 76,         // 0x4c
  _astore_2 = 77,         // 0x4d
  _astore_3 = 78,         // 0x4e
  _iastore = 79,          // 0x4f
  _lastore = 80,          // 0x50
  _fastore = 81,          // 0x51
  _dastore = 82,          // 0x52
  _aastore = 83,          // 0x53
  _bastore = 84,          // 0x54
  _castore = 85,          // 0x55
  _sastore = 86,          // 0x56
  _pop = 87,              // 0x57
  _pop2 = 88,             // 0x58
  _dup = 89,              // 0x59
  _dup_x1 = 90,           // 0x5a
  _dup_x2 = 91,           // 0x5b
  _dup2 = 92,             // 0x5c
  _dup2_x1 = 93,          // 0x5d
  _dup2_x2 = 94,          // 0x5e
  _swap = 95,             // 0x5f
  _iadd = 96,             // 0x60
  _ladd = 97,             // 0x61
  _fadd = 98,             // 0x62
  _dadd = 99,             // 0x63
  _isub = 100,            // 0x64
  _lsub = 101,            // 0x65
  _fsub = 102,            // 0x66
  _dsub = 103,            // 0x67
  _imul = 104,            // 0x68
  _lmul = 105,            // 0x69
  _fmul = 106,            // 0x6a
  _dmul = 107,            // 0x6b
  _idiv = 108,            // 0x6c
  _ldiv = 109,            // 0x6d
  _fdiv = 110,            // 0x6e
  _ddiv = 111,            // 0x6f
  _irem = 112,            // 0x70
  _lrem = 113,            // 0x71
  _frem = 114,            // 0x72
  _drem = 115,            // 0x73
  _ineg = 116,            // 0x74
  _lneg = 117,            // 0x75
  _fneg = 118,            // 0x76
  _dneg = 119,            // 0x77
  _ishl = 120,            // 0x78
  _lshl = 121,            // 0x79
  _ishr = 122,            // 0x7a
  _lshr = 123,            // 0x7b
  _iushr = 124,           // 0x7c
  _lushr = 125,           // 0x7d
  _iand = 126,            // 0x7e
  _land = 127,            // 0x7f
  _ior = 128,             // 0x80
  _lor = 129,             // 0x81
  _ixor = 130,            // 0x82
  _lxor = 131,            // 0x83
  _iinc = 132,            // 0x84
  _i2l = 133,             // 0x85
  _i2f = 134,             // 0x86
  _i2d = 135,             // 0x87
  _l2i = 136,             // 0x88
  _l2f = 137,             // 0x89
  _l2d = 138,             // 0x8a
  _f2i = 139,             // 0x8b
  _f2l = 140,             // 0x8c
  _f2d = 141,             // 0x8d
  _d2i = 142,             // 0x8e
  _d2l = 143,             // 0x8f
  _d2f = 144,             // 0x90
  _i2b = 145,             // 0x91
  _i2c = 146,             // 0x92
  _i2s = 147,             // 0x93
  _lcmp = 148,            // 0x94
  _fcmpl = 149,           // 0x95
  _fcmpg = 150,           // 0x96
  _dcmpl = 151,           // 0x97
  _dcmpg = 152,           // 0x98
  _ifeq = 153,            // 0x99
  _ifne = 154,            // 0x9a
  _iflt = 155,            // 0x9b
  _ifge = 156,            // 0x9c
  _ifgt = 157,            // 0x9d
  _ifle = 158,            // 0x9e
  _if_icmpeq = 159,       // 0x9f
  _if_icmpne = 160,       // 0xa0
  _if_icmplt = 161,       // 0xa1
  _if_icmpge = 162,       // 0xa2
  _if_icmpgt = 163,       // 0xa3
  _if_icmple = 164,       // 0xa4
  _if_acmpeq = 165,       // 0xa5
  _if_acmpne = 166,       // 0xa6
  _goto = 167,            // 0xa7
  _jsr = 168,             // 0xa8
  _ret = 169,             // 0xa9
  _tableswitch = 170,     // 0xaa
  _lookupswitch = 171,    // 0xab
  _ireturn = 172,         // 0xac
  _lreturn = 173,         // 0xad
  _freturn = 174,         // 0xae
  _dreturn = 175,         // 0xaf
  _areturn = 176,         // 0xb0
  _return = 177,          // 0xb1
  _getstatic = 178,       // 0xb2
  _putstatic = 179,       // 0xb3
  _getfield = 180,        // 0xb4
  _putfield = 181,        // 0xb5
  _invokevirtual = 182,   // 0xb6
  _invokespecial = 183,   // 0xb7
  _invokestatic = 184,    // 0xb8
  _invokeinterface = 185, // 0xb9
  _invokedynamic = 186,   // 0xba     // if EnableInvokeDynamic
  _new = 187,             // 0xbb
  _newarray = 188,        // 0xbc
  _anewarray = 189,       // 0xbd
  _arraylength = 190,     // 0xbe
  _athrow = 191,          // 0xbf
  _checkcast = 192,       // 0xc0
  _instanceof = 193,      // 0xc1
  _monitorenter = 194,    // 0xc2
  _monitorexit = 195,     // 0xc3
  _wide = 196,            // 0xc4
  _multianewarray = 197,  // 0xc5
  _ifnull = 198,          // 0xc6
  _ifnonnull = 199,       // 0xc7
  _goto_w = 200,          // 0xc8
  _jsr_w = 201,           // 0xc9
  _breakpoint = 202,      // 0xca
};
struct Utf8Constant;
struct StringConstant;
struct ClassConstant;
struct MethodRefConstant;
struct NameAndTypeConstant;

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
  virtual bool Matches(std::string_view text) const { return false; }
  virtual bool Matches(u2 index) const { return false; }
  virtual bool Matches(u2 first, u2 second) const { return false; }
  virtual std::optional<Utf8Constant*> utf8() { return {}; }
  virtual std::optional<StringConstant*> string() { return {}; }
  virtual std::optional<ClassConstant*> clazz() { return {}; }
  virtual std::optional<MethodRefConstant*> methodRef() { return {}; }
  virtual std::optional<NameAndTypeConstant*> nameAndType() { return {}; }
};

inline void Put2(std::ostream& os, u2 v) {
  os.put(v >> 8);
  os.put(v & 255);
}
inline void Put4(std::ostream& os, u4 v) {
  Put2(os, v >> 16);
  Put2(os, v & 0xffff);
}

struct AttributeInfo {
  u2 attribute_name_index;
  std::string info;
  void Emit(std::ostream& os) const {
    Put2(os, attribute_name_index);
    Put4(os, info.length());
    os.write(info.data(), info.length());
  }
};

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
  void Push(CodeBlock& block) const override {
    if (string_index < 256) {
      block.bytes.put(Code::_ldc);
      block.bytes.put(string_index);
    } else {
      block.bytes.put(Code::_ldc_w);
      Put2(block.bytes, string_index);
    }
  }
  Tag tag() const override { return kString; }
  bool Matches(u2 index) const override { return index == string_index; }
  void Emit(std::ostream& os) const override {
    os.put(tag());
    Put2(os, string_index);
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
class FunctionCodeBlock : public CodeBlock {};
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

  void Emit(std::ostream& os) override {
    u2 this_class = classConstant("Main")->index;
    u2 super_class = classConstant("java/lang/Object")->index;
    methodRefConstant("java/lang/Object", "<init>", "()V");

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
  Constant* FindRef(ClassConstant::Tag tag, u2 class_index,
                    u2 name_and_type_index) {
    for (auto& c : constant_pool) {
      if (c->tag() == tag && c->Matches(class_index, name_and_type_index)) {
        return c.get();
      }
    }
    return nullptr;
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
  std::unique_ptr<CodeBlock> main;
  std::vector<std::unique_ptr<Constant>> constant_pool;
  std::vector<MethodInfo> methods;
}; // namespace
} // namespace

std::unique_ptr<Program> Program::JavaProgram() {
  return std::make_unique<JvmProgram>();
}

} // namespace emit
