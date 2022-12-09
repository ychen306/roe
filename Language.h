#ifndef LANGUAGE_H
#define LANGUAGE_H

#include "EGraph.h"
#include "Pattern.h"
#include "llvm/ADT/StringMap.h"

#include "llvm/Support/raw_ostream.h"

using llvm::errs;

// ValueType is the type of a constant (e.g., int)
template <typename ValueType, typename EGraphT>
class Language : public EGraph<EGraphT> {
public:
  unsigned counter;
  llvm::StringMap<unsigned> opcodeMap;
  llvm::StringMap<unsigned> varMap;
  llvm::DenseMap<ValueType, unsigned> constMap;

  llvm::DenseMap<unsigned, std::string> invVarMap;
  llvm::DenseMap<unsigned, std::string> invOpcodeMap;
  llvm::DenseMap<unsigned, ValueType> invConstMap;

  unsigned newId() { return counter++; }

  using Base = EGraph<EGraphT>;

public:
  Language(llvm::ArrayRef<std::string> opcodes) : counter(0) {
    for (auto &op : opcodes) {
      opcodeMap[op] = newId();
      invOpcodeMap[opcodeMap[op]] = op;
    }
  }

  unsigned getOpcode(std::string opcode) const {
    assert(opcodeMap.count(opcode));
    return opcodeMap.lookup(opcode);
  }

  std::string getVarName(unsigned id) {
    return invVarMap.lookup(id);
  }

  std::string getOpcodeName(unsigned id) {
    return invOpcodeMap.lookup(id);
  }

  unsigned getConstOpcode(ValueType val) {
    auto [it, inserted] = constMap.try_emplace(val);
    if (inserted) {
      it->second = newId();
      invConstMap[it->second] = val;
    }
    return it->second;
  }

  unsigned getVariableOpcode(std::string var) const {
    assert(varMap.count(var));
    return varMap.lookup(var);
  }

  EClassBase *make(std::string opcode, llvm::ArrayRef<EClassBase *> operands) {
    assert(opcodeMap.count(opcode));
    return Base::make(opcodeMap.lookup(opcode), operands);
  }

  EClassBase *var(std::string var) {
    auto [it, inserted] = varMap.try_emplace(var);
    if (inserted) {
      it->setValue(newId());
      invVarMap[it->getValue()] = var;
    }
    return Base::make(it->getValue(), {});
  }

  EClassBase *constant(ValueType val) {
    return Base::make(getConstOpcode(val), {});
  }

  bool is_constant(unsigned id, ValueType &val) {
    auto it = invConstMap.find(id);
    if (it != invConstMap.end()) {
      val = it->second;
      return true;
    }
    return false;
  }
};

template <typename LanguageT>
class LanguageRewrite : public Rewrite<LanguageT> {
  LanguageT &l;
  // mappign variable name -> pattern var
  llvm::StringMap<Pattern *> varMap;

protected:
  Pattern *var(std::string name) {
    if (varMap.count(name))
      return varMap.lookup(name);
    return varMap[name] = Rewrite<LanguageT>::var();
  }

  template <typename... ArgTypes>
  Pattern *match(std::string opcode, ArgTypes... args) {
    return Rewrite<LanguageT>::match(l.getOpcode(opcode),
                                     std::forward<ArgTypes>(args)...);
  }

  // match a constant
  template <typename T>
  Pattern *mConst(T x) {
    return Rewrite<LanguageT>::match(l.getConstOpcode(x));
  }

  template <typename... ArgTypes>
  EClassBase *make(std::string opcode, ArgTypes... args) {
    return l.make(opcode, {std::forward<ArgTypes>(args)...});
  }

  template <typename T>
  EClassBase *constant(T x) { return l.constant(x); }

public:
  LanguageRewrite(LanguageT &l) : l(l) {}
  using LookupFuncTy = std::function<EClassBase *(llvm::StringRef)>;
  virtual EClassBase *rhs(LookupFuncTy var) = 0;
  EClassBase *apply(const PatternToClassMap &m, LanguageT &l) override {
    return rhs(
        [&](llvm::StringRef name) { return m.lookup(varMap.lookup(name)); });
  }
};

#define REWRITE(LANG, RW, LHS, RHS)                                            \
  struct RW : public LanguageRewrite<LANG> {                                   \
    RW(LANG &l) : LanguageRewrite<LANG>(l) { root = LHS; name = #RW; }                     \
    EClassBase *rhs(LookupFuncTy var) override { return RHS; }                 \
  };

#endif // LANGUAGE_H
