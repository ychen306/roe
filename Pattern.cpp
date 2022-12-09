#include "Pattern.h"
#include "EGraph.h"
#include "llvm/ADT/PointerUnion.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/ScopedHashTable.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/Support/raw_ostream.h"

using llvm::errs;

Pattern::Pattern(Opcode opcode,
                 std::vector<Pattern *> theOperands)
    : isLeaf(false), opcode(opcode), operands(std::move(theOperands)) {
  for (auto item : llvm::enumerate(operands))
    item.value()->addUse(this, item.index());
}

namespace {
class PatternMatcher {
  Pattern *root;
  EGraphBase &g;
  std::vector<Substitution> &matches;
  llvm::SmallVector<Pattern *> patternNodes;
  using ClassOrNode = llvm::PointerUnion<EClassBase *, ENode *>;
  llvm::ScopedHashTable<Pattern *, ClassOrNode> subst;
  using Scope = decltype(subst)::ScopeTy;
  bool runImpl(unsigned level);
  bool runOnVar(Pattern *var, unsigned level);
  bool runOnPattern(Pattern *var, unsigned level);
  auto classes() const { return llvm::make_range(g.class_begin(), g.class_end()); }
  void outputSubstitution();

public:
  PatternMatcher(Pattern *, EGraphBase &g, std::vector<Substitution> &matches);
  void run();
};
} // namespace

PatternMatcher::PatternMatcher(Pattern *pat, EGraphBase &g,
                               std::vector<Substitution> &matches)
    : root(pat), g(g), matches(matches) {
  llvm::SmallPtrSet<Pattern *, 8> visited;
  llvm::SmallVector<Pattern *, 8> worklist{pat};
  while (!worklist.empty()) {
    Pattern *pat = worklist.pop_back_val();
    if (!visited.insert(pat).second)
      continue;
    patternNodes.push_back(pat);
    worklist.append(pat->operand_begin(), pat->operand_end());
  }
}

void PatternMatcher::outputSubstitution() {
  auto &match = matches.emplace_back();
  for (auto *pat : patternNodes) {
    if (pat->isVar())
      match.emplace_back(pat, subst.lookup(pat).get<EClassBase *>());
    else
      match.emplace_back(pat, subst.lookup(pat).get<ENode *>()->getClass());
  }
  assert(subst.lookup(root).get<ENode *>()->getClass());
  //match.emplace_back(root, subst.lookup(root).get<ENode *>()->getClass());
}

bool PatternMatcher::runImpl(unsigned level) {
  if (level == patternNodes.size()) {
    outputSubstitution();
    return true;
  }

  Scope scope(subst);
  Pattern *pat = patternNodes[level];
  if (pat->isVar())
    return runOnVar(pat, level);
  return runOnPattern(pat, level);
}

bool PatternMatcher::runOnVar(Pattern *var, unsigned level) {
  assert(var->isVar());
  llvm::SmallPtrSet<EClassBase *, 1> candidates;
  // Find candidates based on bound parents (users)
  for (auto [userPat, operandId] : var->getUses()) {
    auto *user = subst.lookup(userPat).dyn_cast<ENode *>();
    if (!user)
      continue;
    // `var` has to bind to a node in class `c`
    auto *c = user->getOperands()[operandId];
    candidates.insert(c);
    if (candidates.size() > 1)
      break;
  }

  // Impossible: backtrack
  if (candidates.size() > 1)
    return false;

  if (candidates.size() == 1) {
    assert(subst.lookup(var).isNull());
    subst.insert(var, *candidates.begin());
    // Recursively match the rest of the pattern nodes
    return runImpl(level + 1);
  }

  assert(candidates.empty());
  // No constraints on which class we have to bind. Try all of them!
  bool matched = false;
  for (auto *c : classes()) {
    subst.insert(var, c);
    matched |= runImpl(level + 1);
  }
  return matched;
}

static std::vector<ENode *> intersect(std::vector<llvm::DenseSet<ENode *> *> srcs) {
  // Sort the sets by size
  llvm::sort(srcs, [](auto *set1, auto *set2) { return set1->size() < set2->size(); });
  auto *set0 = srcs.front();
  std::vector<ENode *> intersection;
  for (auto *n : *set0) {
    bool intersected = true;
    for (auto *s : llvm::drop_begin(srcs)) {
      if (!s->count(n)) {
        intersected = false;
        break;
      }
    }
    if (intersected)
      intersection.push_back(n);
  }
  return intersection;
}

bool PatternMatcher::runOnPattern(Pattern *pat, unsigned level) {
  assert(!pat->isVar());
  std::vector<llvm::DenseSet<ENode *> *> candidates;
  // Find candidates based on bound parents (users)
  for (auto [userPat, operandId] : pat->getUses()) {
    auto *user = subst.lookup(userPat).dyn_cast<ENode *>();
    if (!user)
      continue;
    // `pat` has to bind to a node in class `c`
    auto *c = g.getLeader(user->getOperands()[operandId]);
    auto *nodes = c->getNodesByOpcode(pat->getOpcode());
    // Backtrack if stuck
    if (!nodes || nodes->empty())
      return false;
    candidates.push_back(nodes);
  }
  // Find candidates based on bound operands
  for (auto item : llvm::enumerate(pat->getOperands())) {
    Pattern *operandPat = item.value();
    unsigned operandId = item.index();
    auto boundValue = subst.lookup(operandPat);
    if (boundValue.isNull())
      continue;
    EClassBase *operandClass = nullptr;
    if (operandPat->isVar())
      operandClass = boundValue.get<EClassBase *>();
    else
      operandClass = boundValue.get<ENode *>()->getClass();
    assert(operandClass);
    auto *nodes = g.getLeader(operandClass)->getUsersByUses(pat->getOpcode(), operandId);
    // Backtrack if stuck
    if (!nodes || nodes->empty())
      return false;
    assert(all_of(*nodes, [&](auto *node) {
          return operandId < node->getOperands().size() &&
          g.isEquivalent(node->getOperands()[operandId], operandClass);
          }));
    candidates.push_back(nodes);
  }

  if (!candidates.empty()) {
    bool matched = false;
    for (auto *node : intersect(std::move(candidates))) {
      subst.insert(pat, node);
      matched |= runImpl(level+1);
    }
    return matched;
  }
  
  // Try everything
  bool matched = false;
  for (auto *c : classes()) {
    assert(g.getLeader(c) == c);
    auto *nodes = c->getNodesByOpcode(pat->getOpcode());
    if (!nodes)
      continue;
    for (auto *node : *nodes) {
      subst.insert(pat, node);
      matched |= runImpl(level+1);
    }
  }
  return matched;
}

void PatternMatcher::run() { runImpl(0); }

std::vector<Substitution> match(Pattern *pat, EGraphBase &g) {
  std::vector<Substitution> matches;
  PatternMatcher matcher(pat, g, matches);
  matcher.run();
  return matches;
}
