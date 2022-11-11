#ifndef EGRAPH_H
#define EGRAPH_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/EquivalenceClasses.h"
#include "llvm/ADT/SmallVector.h"
#include <vector>

using Opcode = unsigned;

class EClass;

class ENode {
  friend class ENodeHashInfo;
  Opcode opcode;
  llvm::SmallVector<EClass *, 3> operands;
  EClass *cls;

public:
  ENode(Opcode opcode, llvm::ArrayRef<EClass *> operands)
      : opcode(opcode), operands(operands.begin(), operands.end()),
        cls(nullptr) {}
  llvm::ArrayRef<EClass *> getOperands() const { return operands; }
  Opcode getOpcode() const { return opcode; }
  void setClass(EClass *cls2) { cls = cls2; }
  EClass *getClass() const { return cls; }
};

class EClass {};

struct NodeKey {
  Opcode opcode;
  llvm::SmallVector<EClass *, 3> operands;
};

struct NodeHashInfo {
  static NodeKey getEmptyKey() { return NodeKey{~0U, {}}; }
  static NodeKey getTombstoneKey() { return NodeKey{~1U, {}}; }
  static bool isEqual(const NodeKey &node1, const NodeKey &node2) {
    if (node1.opcode != node2.opcode)
      return false;
    if (node1.operands.size() != node2.operands.size())
      return false;
    for (auto [o1, o2] : llvm::zip(node1.operands, node2.operands)) {
      if (o1 != o2)
        return false;
    }
    return true;
  }
  static unsigned getHashValue(const NodeKey &node) {
    return llvm::hash_combine(
        llvm::hash_value(node.opcode),
        llvm::hash_combine_range(node.operands.begin(), node.operands.end()));
  }
};

class EGraph {
  llvm::DenseMap<NodeKey, std::unique_ptr<ENode>, NodeHashInfo> nodes;
  llvm::EquivalenceClasses<EClass *> ec;
  std::vector<std::unique_ptr<EClass>> classes;

public:
  EClass *make(Opcode opcode, llvm::ArrayRef<EClass *> operands) {
    ENode *node = getNode(opcode, operands);
    if (auto *c = node->getClass())
      return c;
    EClass *c = classes.emplace_back().get();
    node->setClass(c);
    // FIXME: record use of the operands
    return c;
  }

  EClass *getLeader(EClass *c) const { return ec.getLeaderValue(c); }

  ENode *getNode(Opcode opcode, llvm::ArrayRef<EClass *> operands) {
    NodeKey key;
    key.opcode = opcode;
    for (EClass *operand : operands)
      key.operands.push_back(getLeader(operand));
    return nullptr;
  }
};

#endif // EGRAPH_H
