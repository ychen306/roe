#include "EGraph.h"

namespace {

template <typename KeyType, typename ValueType>
void mergeMap(llvm::DenseMap<KeyType, std::vector<ValueType>> &dst,
              llvm::DenseMap<KeyType, std::vector<ValueType>> &src) {
  for (auto kv : src) {
    auto key = kv.first;

    auto [it, inserted] = dst.try_emplace(key);
    // If this key is not seen in this class, just steal the values from src
    if (inserted) {
      it->second = std::move(kv.second);
      continue;
    }

    // Merge the sorted values
    std::vector<ValueType> merged;
    merged.reserve(it->second.size() + kv.second.size());
    std::merge(it->second.begin(), it->second.end(), kv.second.begin(),
               kv.second.end(), std::back_inserter(merged));
    it->second = std::move(merged);
  }
}

} // namespace

void EClass::addNode(ENode *node) {
  opcodeToNodesMap[node->getOpcode()].push_back(node);
}

void EClass::addUse(ENode *user, unsigned i) {
  uses[std::make_pair(user->getOpcode(), i)].push_back(user);
}

void EClass::absorb(EClass *other) {
  mergeMap(opcodeToNodesMap, other->opcodeToNodesMap);
  mergeMap(uses, other->uses);
}

EClass *EGraph::make(Opcode opcode, llvm::ArrayRef<EClass *> operands) {
  ENode *node = findNode(opcode, operands);
  if (auto *c = node->getClass())
    return c;
  EClass *c = classes.emplace_back(new EClass()).get();
  node->setClass(c);
  c->addNode(node);
  return c;
}

ENode *EGraph::findNode(Opcode opcode, llvm::ArrayRef<EClass *> operands) {
  NodeKey key;
  key.opcode = opcode;
  for (EClass *operand : operands)
    key.operands.push_back(getLeader(operand));
  auto [it, inserted] = nodes.try_emplace(key);
  if (inserted) {
    assert(!it->second);
    it->second.reset(new ENode(opcode, operands));
  }
  return it->second.get();
}

EClass *EGraph::merge(EClass *c1, EClass *c2) {
  c1 = getLeader(c1);
  c2 = getLeader(c2);
  if (c1 == c2)
    return c1;
  assert(c1 != c2);
  // Merge everything into c1
  c1->absorb(c2);
  auto *c = *ec.unionSets(c1, c2);
  // If union-find set c2 as leader swap the content
  if (c != c1)
    c->swap(c1);
  // See if we can merge some of `c`'s users later
  repairList.push_back(c);
  return c;
}
