#ifndef PATTERN_H
#define PATTERN_H

#include "EGraph.h"
#include <memory>
#include <vector>
#include "llvm/Support/raw_ostream.h"

extern int dontPrint;

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

std::vector<Substitution> match(Pattern *, EGraphBase &, int limit=-1);

using PatternToClassMap = llvm::SmallDenseMap<Pattern *, EClassBase *, 4>;

template<typename EGraphT>
class Rewrite {
  std::vector<Pattern *> patternNodes;

protected:
  std::string name;
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
  std::string getName() const { return name; }
};

template<typename EGraphT>
void saturate(llvm::ArrayRef<std::unique_ptr<Rewrite<EGraphT>>> rewrites, EGraphT &g, int iters = 10000) {
  unsigned size;
  struct Stat {
    int numBans;
    int bannedUntil;
    Stat() : numBans(0), bannedUntil(-1) {}
  };

  const unsigned banLen = 5;
  const unsigned matchLimit = 1000;

  llvm::DenseMap<Rewrite<EGraphT> *, Stat> stats;
  for (auto &rw : rewrites)
    stats.try_emplace(rw.get());

  for (int i = 0; i < iters; i++) {
    size = g.numNodes();
    std::vector<std::vector<Substitution>> matches;
    for (auto &rw : rewrites) {
      // Skip banned rewrite
      auto &stat = stats[rw.get()];
      if (stat.bannedUntil > 0 && stat.bannedUntil < i) {
        matches.emplace_back();
        continue;
      }

      unsigned threshold = matchLimit << stat.numBans;
      auto ms = match(rw->sourcePattern(), g, threshold);
      unsigned totalSize = 0;
      for (auto &m : ms)
        totalSize += m.size();
      if (totalSize > threshold) {
        stat.bannedUntil = i + (banLen << stat.numBans);
        stat.numBans++;
        ms.clear();
      }
      matches.push_back(std::move(ms));
    }

    for (unsigned i = 0, n = rewrites.size(); i < n; i++)
      rewrites[i]->applyMatches(matches[i], g);

    g.rebuild();
    if (size == g.numNodes())
      break;
    llvm::errs() << "???? " << i << ", num classes = " << std::distance(g.class_begin(), g.class_end()) << '\n';
  }
}

#endif // PATTERN_H
