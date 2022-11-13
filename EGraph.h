#ifndef EGRAPH_H
#define EGRAPH_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
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

class EGraph;
class EClass {
  // Mapping <user opcode, operand id> -> <sorted array of of user>
  llvm::DenseMap<std::pair<Opcode, unsigned>, llvm::DenseSet<ENode *>> uses;
  llvm::DenseSet<ENode *> users;
  // Partitioning the nodes by opcode
  llvm::DenseMap<Opcode, llvm::DenseSet<ENode *>> opcodeToNodesMap;

  void repairUserSets(const llvm::DenseMap<ENode *, ENode *> &oldToNewUserMap);

public:
  EClass() = default;
  EClass &operator=(EClass &&) = default;

  void addNode(ENode *node);
  // Record that fact that `user`'s `i`th operand is `this` EClass
  void addUse(ENode *user, unsigned i);
  // Aborb `other` into `this` class and empther `other`
  void absorb(EClass *other);
  void swap(EClass *other) {
    uses.swap(other->uses);
    opcodeToNodesMap.swap(other->opcodeToNodesMap);
  }
  llvm::iterator_range<decltype(users)::iterator> getUsers() {
    return llvm::make_range(users.begin(), users.end());
  }
  llvm::DenseSet<ENode *> *getUsersByUses(Opcode opcode, unsigned operandId);
  void repair(EGraph *);
};

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
  // List of e-classs that require repair
  std::vector<EClass *> repairList;

  void repair(EClass *);
  EClass *newClass();

public:
  NodeKey canonicalize(Opcode opcode, llvm::ArrayRef<EClass *> operands);
  EClass *make(Opcode opcode, llvm::ArrayRef<EClass *> operands = llvm::None);
  EClass *getLeader(EClass *c) const { return ec.getLeaderValue(c); }
  ENode *findNode(Opcode opcode, llvm::ArrayRef<EClass *> operands);
  ENode *findNode(NodeKey);
  bool isEquivalent(EClass *c1, EClass *c2) const {
    return ec.isEquivalent(c1, c2);
  }
  EClass *merge(EClass *c1, EClass *c2);
  void rebuild();
};

#endif // EGRAPH_H
