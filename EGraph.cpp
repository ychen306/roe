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

NodeKey EGraphBase::canonicalize(Opcode opcode, llvm::ArrayRef<EClass *> operands) {
  NodeKey key;
  key.opcode = opcode;
  for (EClass *c : operands)
    key.operands.push_back(getLeader(c));
  return key;
}

ENode *EGraphBase::findNode(NodeKey key) {
  auto [it, inserted] = nodes.try_emplace(key);
  if (inserted) {
    assert(!it->second);
    it->second.reset(new ENode(key.opcode, key.operands));
  }
  return it->second.get();
}

ENode *EGraphBase::findNode(Opcode opcode, llvm::ArrayRef<EClass *> operands) {
  return findNode(canonicalize(opcode, operands));
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
