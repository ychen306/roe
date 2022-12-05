#ifndef LANGUAGE_H
#define LANGUAGE_H

#include "EGraph.h"
#include "Pattern.h"
#include "llvm/ADT/StringMap.h"

// ValueType is the type of a constant (e.g., int)
template <typename ValueType, typename EGraphT>
class Language : public EGraph<EGraphT> {
  unsigned counter;
  llvm::StringMap<unsigned> opcodeMap;
  llvm::StringMap<unsigned> varMap;
  llvm::DenseMap<ValueType, unsigned> constMap;
  llvm::DenseMap<unsigned, ValueType> invConstMap;

  unsigned newId() { return counter++; }

  using Base = EGraph<EGraphT>;

public:
  Language(llvm::ArrayRef<std::string> opcodes) : counter(0) {
    for (auto &op : opcodes) {
      opcodeMap[op] = newId();
    }
  }

  unsigned getOpcode(std::string opcode) const {
    assert(opcodeMap.count(opcode));
    return opcodeMap.lookup(opcode);
  }

  EClassBase *make(std::string opcode, llvm::ArrayRef<EClassBase *> operands) {
    assert(opcodeMap.count(opcode));
    return Base::make(opcodeMap.lookup(opcode), operands);
  }

  EClassBase *var(std::string var) {
    auto [it, inserted] = varMap.try_emplace(var);
    if (inserted)
      it->setValue(newId());
    return Base::make(it->getValue(), {});
  }

  EClassBase *constant(ValueType val) {
    auto [it, inserted] = constMap.try_emplace(val);
    if (inserted) {
      it->second = newId();
      invConstMap[val] = it->second;
    }
    return Base::make(it->second, {});
  }

  bool is_constant(unsigned id, ValueType &val) {
    auto it = constMap.find(id);
    if (it != constMap.end()) {
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

  template <typename... ArgTypes>
  EClassBase *make(std::string opcode, ArgTypes... args) {
    return l.make(opcode, {std::forward<ArgTypes>(args)...});
  }

public:
  LanguageRewrite(LanguageT &l) : l(l) {}
  using LookupFuncTy = std::function<EClassBase *(llvm::StringRef)>;
  virtual EClassBase *rhs(LookupFuncTy var) = 0;
  EClassBase *apply(const PatternToClassMap &m, LanguageT &) override {
    return rhs(
        [&](llvm::StringRef name) { return m.lookup(varMap.lookup(name)); });
  }
};

#define REWRITE(LANG, RW, LHS, RHS)                                            \
  struct RW : public LanguageRewrite<LANG> {                                   \
    RW(LANG &l) : LanguageRewrite<LANG>(l) { root = LHS; }                     \
    EClassBase *rhs(LookupFuncTy var) override { return RHS; }                 \
  };

#endif // LANGUAGE_H
