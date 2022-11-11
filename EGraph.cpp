#include "EGraph.h"

void EClass::addNode(ENode *node) {
  opcodeToNodesMap[node->getOpcode()].push_back(node);
}

void EClass::addUse(ENode *user, unsigned i) {
  uses[std::make_pair(user->getOpcode(), i)].push_back(user);
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
