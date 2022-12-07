#ifndef EGRAPH_H
#define EGRAPH_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
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
  EClassBase *leader;
  // For union by rank
  unsigned rank;

protected:
  // Mapping <user opcode, operand id> -> <sorted array of of user>
  llvm::DenseMap<std::pair<Opcode, unsigned>, llvm::DenseSet<ENode *>> uses;
  llvm::DenseSet<ENode *> users;
  // Partitioning the nodes by opcode
  llvm::DenseMap<Opcode, llvm::DenseSet<ENode *>> opcodeToNodesMap;

  void repairUserSets(const llvm::DenseMap<ENode *, ENode *> &oldToNewUserMap);

public:
  EClassBase() : rank(0), leader(this) {}
  EClassBase &operator=(EClassBase &&) = default;

  bool isLeader() const { return leader == this; }

  unsigned getRank() const { return rank; }

  EClassBase *getLeader() {
    if (isLeader() || leader->isLeader())
      return leader;
    // Path compression
    leader = leader->getLeader();
    return leader;
  }

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

template <typename EGraphT> class EGraph;
template <typename EGraphT> class EClass : public EClassBase {
  friend class EGraph<EGraphT>;
  typename EGraphT::AnalysisData data;

public:
  void repair(EGraph<EGraphT> *g);
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
  std::vector<std::unique_ptr<EClassBase>> classes;
  // List of e-classs that require repair
  std::vector<EClassBase *> repairList;

  using ec_iterator = decltype(classes)::iterator;
  using class_ptr = EClassBase *;

public:
  class class_iterator;
  using class_iterator_base =
      llvm::iterator_adaptor_base<class_iterator, ec_iterator,
                                  std::forward_iterator_tag, class_ptr,
                                  std::ptrdiff_t, class_ptr *, class_ptr &>;

  class class_iterator : public class_iterator_base {
    std::vector<std::unique_ptr<EClassBase>> &classes;

    void skipEmptyClasses() {
      while (I != classes.end() && !(*I)->isLeader())
        ++I;
    }

  public:
    class_iterator(std::vector<std::unique_ptr<EClassBase>> &classes,
                   ec_iterator it)
        : class_iterator_base(it), classes(classes) {
      skipEmptyClasses();
    }
    class_ptr operator*() const { return I->get(); }
    class_iterator &operator++() {
      ++I;
      skipEmptyClasses();
      return *this;
    }
  };

  class_iterator class_begin() {
    return class_iterator(classes, classes.begin());
  }

  class_iterator class_end() { return class_iterator(classes, classes.end()); }

  NodeKey canonicalize(Opcode opcode, llvm::ArrayRef<EClassBase *> operands);
  EClassBase *getLeader(EClassBase *c) const { return c->getLeader(); }
  ENode *findNode(Opcode opcode, llvm::ArrayRef<EClassBase *> operands);
  ENode *findNode(NodeKey);
  bool isEquivalent(EClassBase *c1, EClassBase *c2) const {
    return getLeader(c1) == getLeader(c2);
  }
  unsigned numNodes() const { return nodes.size(); }
};

template <typename EGraphT> class EGraph : public EGraphBase {
  EClassBase *newClass() {
    return classes.emplace_back(new EClass<EGraphT>()).get();
  }

protected:
  template <typename T> void setData(EClassBase *c, T data) {
    static_cast<EClass<EGraphT> *>(c)->data = data;
  }

  auto getData(EClassBase *c) {
    return static_cast<EClass<EGraphT> *>(c)->data;
  }

  EGraphT *analysis() { return static_cast<EGraphT *>(this); }

public:
  EClassBase *make(Opcode opcode,
                   llvm::ArrayRef<EClassBase *> operands = llvm::None) {
    ENode *node = findNode(opcode, operands);
    if (auto *c = node->getClass())
      return c;

    EClassBase *c = newClass();
    node->setClass(c);
    c->addNode(node);
    for (auto item : llvm::enumerate(node->getOperands()))
      item.value()->addUse(node, item.index());

    // Run analysis on the new node
    setData(c, analysis()->analyze(node));
    analysis()->modify(c);

    return getLeader(c);
  }

  EClassBase *merge(EClassBase *c1, EClassBase *c2) {
    c1 = getLeader(c1);
    c2 = getLeader(c2);
    if (c1 == c2)
      return c1;

    assert(c1->isLeader() && c2->isLeader());

    if (c1->getRank() < c2->getRank()) {
      std::swap(c1, c2);
    }

    assert(c1 != c2);
    // Join the analysis lattice
    auto newData = analysis()->join(getData(c1), getData(c2));
    // Merge everything into c1
    c1->absorb(c2);
    // See if we can merge some of `c`'s users later
    repairList.push_back(c1);
    // Update the joined analysis result
    setData(c1, newData);
    return c1;
  }

  void rebuild() {
    while (!repairList.empty()) {
      llvm::DenseSet<EClassBase *> todo;
      for (auto *c : repairList)
        todo.insert(getLeader(c));
      repairList.clear();
      for (auto *c : todo) {
        static_cast<EClass<EGraphT> *>(getLeader(c))->repair(this);

        analysis()->modify(c);
        for (ENode *user : c->getUsers()) {
          auto *userClass = user->getClass();
          auto oldData = getData(userClass);
          auto newData = analysis()->join(oldData, analysis()->analyze(user));
          if (newData != oldData) {
            setData(userClass, newData);
            repairList.push_back(userClass);
          }
        }
      }
    }
  }
};

template <typename EGraphT> void EClass<EGraphT>::repair(EGraph<EGraphT> *g) {
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
      static_cast<EClass<EGraphT> *>(g->getLeader(operand))
          ->repairUserSets(oldToNewUserMap);
  }
}

struct NullAnalysis {
  using AnalysisData = char;
  AnalysisData analyze(ENode *) { return 0; }
  AnalysisData join(AnalysisData, AnalysisData) { return 0; }
  void modify(EClassBase *) {}
};

// EGraph without analysis
struct BasicEGraph : public EGraph<BasicEGraph>, NullAnalysis {};

#endif // EGRAPH_H
