#ifndef EGRAPH_H
#define EGRAPH_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/EquivalenceClasses.h"
#include "llvm/ADT/SmallVector.h"
#include <vector>

using Opcode = unsigned;

class EClassBase;
class ENode {
  friend class ENodeHashInfo;
  Opcode opcode;
  llvm::SmallVector<EClassBase *, 3> operands;
  EClassBase *cls;

public:
  ENode(Opcode opcode, llvm::ArrayRef<EClassBase *> operands)
      : opcode(opcode), operands(operands.begin(), operands.end()),
        cls(nullptr) {}
  llvm::ArrayRef<EClassBase *> getOperands() const { return operands; }
  Opcode getOpcode() const { return opcode; }
  void setClass(EClassBase *cls2) { cls = cls2; }
  EClassBase *getClass() const { return cls; }
};

class EClassBase {
protected:
  // Mapping <user opcode, operand id> -> <sorted array of of user>
  llvm::DenseMap<std::pair<Opcode, unsigned>, llvm::DenseSet<ENode *>> uses;
  llvm::DenseSet<ENode *> users;
  // Partitioning the nodes by opcode
  llvm::DenseMap<Opcode, llvm::DenseSet<ENode *>> opcodeToNodesMap;

  void repairUserSets(const llvm::DenseMap<ENode *, ENode *> &oldToNewUserMap);

public:
  EClassBase() = default;
  EClassBase &operator=(EClassBase &&) = default;

  void addNode(ENode *node);
  // Record that fact that `user`'s `i`th operand is `this` EClassBase
  void addUse(ENode *user, unsigned i);
  // Aborb `other` into `this` class and empther `other`
  void absorb(EClassBase *other);
  void swap(EClassBase *other) {
    uses.swap(other->uses);
    opcodeToNodesMap.swap(other->opcodeToNodesMap);
  }
  llvm::iterator_range<decltype(users)::iterator> getUsers() {
    return llvm::make_range(users.begin(), users.end());
  }
  llvm::DenseSet<ENode *> *getUsersByUses(Opcode opcode, unsigned operandId);
  llvm::DenseSet<ENode *> *getNodesByOpcode(Opcode opcode);
};

template <typename AnalysisT> class EGraph;
template <typename AnalysisT> class EClass : public EClassBase {
  typename AnalysisT::Data data;

public:
  void repair(EGraph<AnalysisT> *g);
};

struct NodeKey {
  Opcode opcode;
  llvm::SmallVector<EClassBase *, 3> operands;
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

class EGraphBase {
protected:
  llvm::DenseMap<NodeKey, std::unique_ptr<ENode>, NodeHashInfo> nodes;
  llvm::EquivalenceClasses<EClassBase *> ec;
  std::vector<std::unique_ptr<EClassBase>> classes;
  // List of e-classs that require repair
  std::vector<EClassBase *> repairList;

  void repair(EClassBase *);

  using ec_iterator = decltype(ec)::iterator;
  using class_ptr = EClassBase *;

public:
  class class_iterator;
  using class_iterator_base = llvm::iterator_adaptor_base<
      class_iterator, ec_iterator,
      typename std::iterator_traits<ec_iterator>::iterator_category, class_ptr,
      std::ptrdiff_t, class_ptr *, class_ptr &>;

  class class_iterator : public class_iterator_base {
    llvm::EquivalenceClasses<EClassBase *> &ec;

    void skipEmptyClasses() {
      while (I != ec.end() && ec.member_begin(I) == ec.member_end())
        ++I;
    }

  public:
    class_iterator(llvm::EquivalenceClasses<EClassBase *> &ec, ec_iterator it)
        : class_iterator_base(it), ec(ec) {
      skipEmptyClasses();
    }
    class_ptr operator*() const { return *ec.member_begin(I); }
    class_iterator &operator++() {
      ++I;
      skipEmptyClasses();
      return *this;
    }
  };

  class_iterator class_begin() { return class_iterator(ec, ec.begin()); }

  class_iterator class_end() { return class_iterator(ec, ec.end()); }

  NodeKey canonicalize(Opcode opcode, llvm::ArrayRef<EClassBase *> operands);
  EClassBase *getLeader(EClassBase *c) const { return ec.getLeaderValue(c); }
  ENode *findNode(Opcode opcode, llvm::ArrayRef<EClassBase *> operands);
  ENode *findNode(NodeKey);
  bool isEquivalent(EClassBase *c1, EClassBase *c2) const {
    return ec.isEquivalent(c1, c2);
  }
  unsigned numNodes() const { return nodes.size(); }
};

struct NullAnalysis {
  using Data = char;
};

template <typename AnalysisT = NullAnalysis> class EGraph : public EGraphBase {
  EClassBase *newClass() {
    auto *c = classes.emplace_back(new EClass<AnalysisT>()).get();
    ec.insert(c);
    return c;
  }

public:
  EClassBase *make(Opcode opcode, llvm::ArrayRef<EClassBase *> operands = llvm::None) {
    ENode *node = findNode(opcode, operands);
    if (auto *c = node->getClass())
      return c;
    EClassBase *c = newClass();
    node->setClass(c);
    c->addNode(node);
    for (auto item : llvm::enumerate(node->getOperands()))
      item.value()->addUse(node, item.index());
    return c;
  }

  EClassBase *merge(EClassBase *c1, EClassBase *c2) {
    c1 = getLeader(c1);
    c2 = getLeader(c2);
    if (c1 == c2)
      return c1;
    assert(c1 != c2);
    // Merge everything into c1
    c1->absorb(c2);
    auto *c = *ec.unionSets(c1, c2);
    if (c != c1)
      *c = std::move(*c1);
    // See if we can merge some of `c`'s users later
    repairList.push_back(c);
    return c;
  }

  void rebuild() {
    while (!repairList.empty()) {
      std::set<EClassBase *> todo;
      for (auto *c : repairList)
        todo.insert(getLeader(c));
      repairList.clear();
      for (auto *c : todo)
        static_cast<EClass<AnalysisT> *>(getLeader(c))
            ->repair(this);
    }
  }
};

template <typename AnalysisT>
void EClass<AnalysisT>::repair(EGraph<AnalysisT> *g) {
  // Group users together by their canonical representation
  llvm::DenseMap<NodeKey, std::vector<ENode *>, NodeHashInfo> uniqueUsers;
  for (ENode *user : users) {
    auto key = g->canonicalize(user->getOpcode(), user->getOperands());
    uniqueUsers[key].push_back(user);
  }

  // Remember the nodes that we are replacing
  llvm::DenseMap<ENode *, ENode *> oldToNewUserMap;
  // Merge and remove the duplicated users
  for (auto kv : uniqueUsers) {
    NodeKey key = kv.first;
    auto &nodes = kv.second;
    if (nodes.size() <= 1)
      continue;

    ENode *node0 = nodes.front();
    auto *c = node0->getClass();
    assert(c);
    ENode *user = g->findNode(key);

    oldToNewUserMap.try_emplace(node0, user);
    for (auto *node : llvm::drop_begin(nodes)) {
      assert(node->getClass());
      g->merge(c, node->getClass());
      oldToNewUserMap.try_emplace(node, user);
    }

    user->setClass(c);
    users.insert(user);
    for (auto *operand : user->getOperands())
      static_cast<EClass<AnalysisT> *>(g->getLeader(operand))
          ->repairUserSets(oldToNewUserMap);
  }
}

#endif // EGRAPH_H
