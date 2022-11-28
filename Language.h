#ifndef LANGUAGE_H
#define LANGUAGE_H

#include "EGraph.h"
#include "llvm/ADT/StringMap.h"

// ValueType is the type of a constant (e.g., int)
template<typename ValueType>
class Language {
  EGraph &g;
  unsigned counter;
  llvm::StringMap<unsigned> opcodeMap;
  llvm::StringMap<unsigned> varMap;
  llvm::DenseMap<ValueType, unsigned> constMap;

  unsigned newId() { return counter++; }

public:
  Language(EGraph &g) : g(g), counter(0) {}

  EClass *make(std::string opcode, llvm::ArrayRef<EClass *> operands) {
    auto [it, inserted] = opcodeMap.try_emplace(opcode);
    if (inserted)
      it->setValue(newId());
    return g.make(it->getValue(), operands);
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
};

#endif // LANGUAGE_H
