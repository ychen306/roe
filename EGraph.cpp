#include "EGraph.h"
#include "Pattern.h"
#include "llvm/Support/raw_ostream.h"

using llvm::errs;

namespace {

// Destructively merge `src` into `dst`.
template <typename KeyType, typename ValueType>
void absorbMap(llvm::DenseMap<KeyType, llvm::DenseSet<ValueType>> &dst,
               llvm::DenseMap<KeyType, llvm::DenseSet<ValueType>> &src) {
  for (auto kv : src) {
    auto key = kv.first;

    auto [it, inserted] = dst.try_emplace(key);
    if (inserted) {
      // If this key is not seen in this class, just steal the values from src
      it->second = std::move(kv.second);
    } else {
      // Merge them otherwise
      it->second.insert(kv.second.begin(), kv.second.end());
    }
  }

  src.clear();
}

} // namespace

void EClass::addNode(ENode *node) {
  opcodeToNodesMap[node->getOpcode()].insert(node);
}

void EClass::addUse(ENode *user, unsigned i) {
  uses[std::make_pair(user->getOpcode(), i)].insert(user);
  users.insert(user);
}

void EClass::absorb(EClass *other) {
  absorbMap(opcodeToNodesMap, other->opcodeToNodesMap);
  absorbMap(uses, other->uses);
  users.insert(other->users.begin(), other->users.end());
}

llvm::DenseSet<ENode *> *EClass::getUsersByUses(Opcode opcode,
                                                unsigned operandId) {
  auto it = uses.find({opcode, operandId});
  if (it != uses.end())
    return &it->second;
  return nullptr;
}

llvm::DenseSet<ENode *> *EClass::getNodesByOpcode(Opcode opcode) {
  auto it = opcodeToNodesMap.find(opcode);
  if (it != opcodeToNodesMap.end())
    return &it->second;
  return nullptr;
}

EClass *EGraph::newClass() {
  auto *c = classes.emplace_back(new EClass()).get();
  ec.insert(c);
  return c;
}

EClass *EGraph::make(Opcode opcode, llvm::ArrayRef<EClass *> operands) {
  ENode *node = findNode(opcode, operands);
  if (auto *c = node->getClass())
    return c;
  EClass *c = newClass();
  node->setClass(c);
  c->addNode(node);
  for (auto item : llvm::enumerate(node->getOperands()))
    item.value()->addUse(node, item.index());
  return c;
}

NodeKey EGraph::canonicalize(Opcode opcode, llvm::ArrayRef<EClass *> operands) {
  NodeKey key;
  key.opcode = opcode;
  for (EClass *c : operands)
    key.operands.push_back(getLeader(c));
  return key;
}

ENode *EGraph::findNode(NodeKey key) {
  auto [it, inserted] = nodes.try_emplace(key);
  if (inserted) {
    assert(!it->second);
    it->second.reset(new ENode(key.opcode, key.operands));
  }
  return it->second.get();
}

ENode *EGraph::findNode(Opcode opcode, llvm::ArrayRef<EClass *> operands) {
  return findNode(canonicalize(opcode, operands));
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
  if (c != c1)
    *c = std::move(*c1);
  // See if we can merge some of `c`'s users later
  repairList.push_back(c);
  return c;
}

void EGraph::rebuild() {
  while (!repairList.empty()) {
    std::set<EClass *> todo;
    for (auto *c : repairList)
      todo.insert(getLeader(c));
    repairList.clear();
    for (auto *c : todo)
      getLeader(c)->repair(this);
  }
}

void EClass::repairUserSets(const llvm::DenseMap<ENode *, ENode *> &oldToNewUserMap) {
  // Fix the use index by replacing the duplicated user
  // with their new canonical user
  for (auto &kv : oldToNewUserMap) {
    if (users.erase(kv.first))
      users.insert(kv.second);
  }

  for (auto &nodes : llvm::make_second_range(uses)) {
    for (auto &kv : oldToNewUserMap) {
      if (nodes.erase(kv.first))
        nodes.insert(kv.second);
    }
  }
}

void EClass::repair(EGraph *g) {
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
      g->getLeader(operand)->repairUserSets(oldToNewUserMap);
  }
}
