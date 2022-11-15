#ifndef PATTERN_H
#define PATTERN_H

#include "EGraph.h"
#include <memory>
#include <vector>

class Pattern {
  bool isLeaf;
  Opcode opcode;
  std::vector<std::shared_ptr<Pattern>> operands;

  std::vector<std::pair<Pattern *, unsigned>> uses;

  Pattern() : isLeaf(true) {}
  Pattern(Opcode opcode, std::vector<std::shared_ptr<Pattern>> operands);

public:
  static std::shared_ptr<Pattern>
  make(Opcode opcode, std::vector<std::shared_ptr<Pattern>> operands) {
    return std::shared_ptr<Pattern>(new Pattern(opcode, operands));
  }

  static std::shared_ptr<Pattern> var() {
    return std::shared_ptr<Pattern>(new Pattern());
  }

  bool isVar() const { return isLeaf; }

  Opcode getOpcode() const { return opcode; }

  llvm::ArrayRef<std::shared_ptr<Pattern>> getOperands() const {
    return operands;
  }

  void addUse(Pattern *user, unsigned operandId) {
    uses.emplace_back(user, operandId);
  }

  llvm::ArrayRef<std::pair<Pattern *, unsigned>> getUses() const {
    return uses;
  }
};

using Substitution = llvm::SmallVector<std::pair<Pattern *, EClass *>, 4>;

std::vector<Substitution> match(Pattern *, EGraph &);

using PatternPtr = std::shared_ptr<Pattern>;

class Rewrite {
  std::vector<PatternPtr> patternNodes;

protected:
  PatternPtr root;
  template <typename... ArgTypes> PatternPtr make(Opcode op, ArgTypes... args) {
    patternNodes.push_back(
        Pattern::make(op, {std::forward<ArgTypes>(args)...}));
    return patternNodes.back();
  }
  PatternPtr var() {
    patternNodes.push_back(Pattern::var());
    return patternNodes.back();
  }

  using PatternToClassMap = llvm::SmallDenseMap<Pattern *, EClass *, 4>;

  // Apply the rewrite given a matched pattern
  virtual EClass *apply(const PatternToClassMap &, EGraph &) = 0;

public:
  virtual ~Rewrite();
  // The left-hand side
  Pattern *sourcePattern() const { return root.get(); }
  void applyMatches(llvm::ArrayRef<Substitution> matches, EGraph &);
};

void saturate(llvm::ArrayRef<std::unique_ptr<Rewrite>>, EGraph &);

#endif // PATTERN_H
