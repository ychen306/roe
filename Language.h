#ifndef LANGUAGE_H
#define LANGUAGE_H

#include "EGraph.h"
#include "Pattern.h"
#include "llvm/ADT/StringMap.h"

// ValueType is the type of a constant (e.g., int)
template <typename ValueType, typename AnalysisT> class Language {
  EGraph<AnalysisT> &g;
  unsigned counter;
  llvm::StringMap<unsigned> opcodeMap;
  llvm::StringMap<unsigned> varMap;
  llvm::DenseMap<ValueType, unsigned> constMap;

  unsigned newId() { return counter++; }

public:
  using AnalysisType = AnalysisT;
  Language(EGraph<AnalysisT> &g, llvm::ArrayRef<std::string> opcodes)
      : g(g), counter(0) {
    for (auto &op : opcodes) {
      opcodeMap[op] = newId();
    }
  }

  unsigned getOpcode(std::string opcode) const {
    assert(opcodeMap.count(opcode));
    return opcodeMap.lookup(opcode);
  }

  EClass *make(std::string opcode, llvm::ArrayRef<EClass *> operands) {
    assert(opcodeMap.count(opcode));
    return g.make(opcodeMap.lookup(opcode), operands);
  }

  EClass *var(std::string var) {
    auto [it, inserted] = varMap.try_emplace(var);
    if (inserted)
      it->setValue(newId());
    return g.make(it->getValue(), {});
  }

  EClass *constant(ValueType val) {
    auto [it, inserted] = varMap.try_emplace(val);
    if (inserted)
      it.second = newId();
    return g.make(it.second, {});
  }

  EGraph<AnalysisT> &graph() { return g; }
};

template <typename LanguageT>
class LanguageRewrite : public Rewrite<typename LanguageT::AnalysisType> {
  LanguageT l;
  // mappign variable name -> pattern var
  llvm::StringMap<Pattern *> varMap;

public:
  using AnalysisType = typename LanguageT::AnalysisType;

protected:
  Pattern *var(std::string name) {
    if (varMap.count(name))
      return varMap.lookup(name);
    return varMap[name] = Rewrite<AnalysisType>::var();
  }

  template <typename... ArgTypes>
  Pattern *match(std::string opcode, ArgTypes... args) {
    return Rewrite<AnalysisType>::match(l.getOpcode(opcode),
                                        std::forward<ArgTypes>(args)...);
  }

  template <typename... ArgTypes>
  EClass *make(std::string opcode, ArgTypes... args) {
    return l.make(opcode, {std::forward<ArgTypes>(args)...});
  }

public:
  LanguageRewrite(LanguageT &l) : l(l) {}
  using LookupFuncTy = std::function<EClass *(llvm::StringRef)>;
  virtual EClass *rhs(LookupFuncTy var) = 0;
  EClass *apply(const PatternToClassMap &m, EGraph<AnalysisType> &) override {
    return rhs(
        [&](llvm::StringRef name) { return m.lookup(varMap.lookup(name)); });
  }
};

#define REWRITE(LANG, RW, LHS, RHS)                                            \
  struct RW : public LanguageRewrite<LANG> {                                   \
    RW(LANG &l) : LanguageRewrite<LANG>(l) { root = LHS; }                     \
    EClass *rhs(LookupFuncTy var) override { return RHS; }                     \
  };

#endif // LANGUAGE_H
