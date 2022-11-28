#ifndef PATTERN_H
#define PATTERN_H

#include "EGraph.h"
#include <memory>
#include <vector>

class Pattern {
  bool isLeaf;
  Opcode opcode;
  std::vector<Pattern *> operands;

  std::vector<std::pair<Pattern *, unsigned>> uses;

  Pattern() : isLeaf(true) {}
  Pattern(Opcode opcode, std::vector<Pattern *> operands);

public:
  static Pattern *make(Opcode opcode, std::vector<Pattern *> operands) {
    return new Pattern(opcode, operands);
  }

  static Pattern *var() {
    return new Pattern();
  }

  bool isVar() const { return isLeaf; }

  Opcode getOpcode() const { return opcode; }

  llvm::ArrayRef<Pattern *> getOperands() const {
    return operands;
  }

  decltype(operands)::const_iterator operand_begin() const { return operands.begin(); }
  decltype(operands)::const_iterator operand_end() const { return operands.end(); }

  void addUse(Pattern *user, unsigned operandId) {
    uses.emplace_back(user, operandId);
  }

  llvm::ArrayRef<std::pair<Pattern *, unsigned>> getUses() const {
    return uses;
  }
};

using Substitution = llvm::SmallVector<std::pair<Pattern *, EClass *>, 4>;

std::vector<Substitution> match(Pattern *, EGraph &);

class Rewrite {
  std::vector<Pattern *> patternNodes;

protected:
  Pattern *root;
  template <typename... ArgTypes> Pattern *make(Opcode op, ArgTypes... args) {
    patternNodes.push_back(
        Pattern::make(op, {std::forward<ArgTypes>(args)...}));
    return patternNodes.back();
  }
  Pattern *var() {
    patternNodes.push_back(Pattern::var());
    return patternNodes.back();
  }

  using PatternToClassMap = llvm::SmallDenseMap<Pattern *, EClass *, 4>;

  // Apply the rewrite given a matched pattern
  virtual EClass *apply(const PatternToClassMap &, EGraph &) = 0;

public:
  virtual ~Rewrite();
  // The left-hand side
  Pattern *sourcePattern() const { return root; }
  void applyMatches(llvm::ArrayRef<Substitution> matches, EGraph &);
};

void saturate(llvm::ArrayRef<std::unique_ptr<Rewrite>>, EGraph &);

#endif // PATTERN_H
