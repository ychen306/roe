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

using Substitution = llvm::SmallVector<std::pair<Pattern *, EClassBase *>, 4>;

std::vector<Substitution> match(Pattern *, EGraphBase &);

using PatternToClassMap = llvm::SmallDenseMap<Pattern *, EClassBase *, 4>;

template<typename EGraphT>
class Rewrite {
  std::vector<Pattern *> patternNodes;

protected:
  Pattern *root;
  template <typename... ArgTypes> Pattern *match(Opcode op, ArgTypes... args) {
    patternNodes.push_back(
        Pattern::make(op, {std::forward<ArgTypes>(args)...}));
    return patternNodes.back();
  }
  Pattern *var() {
    patternNodes.push_back(Pattern::var());
    return patternNodes.back();
  }

  // Apply the rewrite given a matched pattern
  virtual EClassBase *apply(const PatternToClassMap &, EGraphT &) = 0;

public:
  virtual ~Rewrite() {}
  // The left-hand side
  Pattern *sourcePattern() const { return root; }
  void applyMatches(llvm::ArrayRef<Substitution> matches, EGraphT &g) {
    for (auto &m : matches) {
      PatternToClassMap subst(m.begin(), m.end());
      auto *c = apply(subst, g);
      g.merge(c, subst.lookup(root));
    }
  }
};

template<typename EGraphT>
void saturate(llvm::ArrayRef<std::unique_ptr<Rewrite<EGraphT>>> rewrites, EGraphT &g) {
  unsigned size;
  do {
    size = g.numNodes();
    std::vector<std::vector<Substitution>> matches;
    for (auto &rw : rewrites)
      matches.push_back(match(rw->sourcePattern(), g));
    for (unsigned i = 0, n = rewrites.size(); i < n; i++)
      rewrites[i]->applyMatches(matches[i], g);
    g.rebuild();
  } while (size != g.numNodes());
}

#endif // PATTERN_H
