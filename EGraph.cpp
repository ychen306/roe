#include "EGraph.h"
#include "Pattern.h"
#include "llvm/Support/raw_ostream.h"

using llvm::errs;

int dontPrint = false;
bool print = false;

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

void EClassBase::addNode(ENode *node) {
  opcodeToNodesMap[node->getOpcode()].insert(node);
}

void EClassBase::addUse(ENode *user, unsigned i) {
  uses[std::make_pair(user->getOpcode(), i)].insert(user);
  users.insert(user);
}

void EClassBase::absorb(EClassBase *other) {
  absorbMap(opcodeToNodesMap, other->opcodeToNodesMap);
  absorbMap(uses, other->uses);
  users.insert(other->users.begin(), other->users.end());
  other->users.clear();
  other->leader = this;
  if (rank == other->rank)
    rank++;
  assert(other->getLeader() == this);
}

llvm::DenseSet<ENode *> *EClassBase::getUsersByUses(Opcode opcode,
                                                unsigned operandId) {
  auto it = uses.find({opcode, operandId});
  if (it != uses.end())
    return &it->second;
  return nullptr;
}

llvm::DenseSet<ENode *> *EClassBase::getNodesByOpcode(Opcode opcode) {
  auto it = opcodeToNodesMap.find(opcode);
  if (it != opcodeToNodesMap.end())
    return &it->second;
  return nullptr;
}

NodeKey EGraphBase::canonicalize(Opcode opcode, llvm::ArrayRef<EClassBase *> operands) {
  NodeKey key;
  key.opcode = opcode;
  for (EClassBase *c : operands)
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

ENode *EGraphBase::findNode(Opcode opcode, llvm::ArrayRef<EClassBase *> operands) {
  return findNode(canonicalize(opcode, operands));
}

void EClassBase::repairUserSets(llvm::ArrayRef<Replacement> repls) {
  // Fix the use index by replacing the duplicated user
  // with their new canonical user
  for (auto &repl : repls) {
    bool erased = false;
    for (auto *node : repl.from)
      erased |= users.erase(node);
    if (erased)
      users.insert(repl.to);
  }

  for (auto &nodes : llvm::make_second_range(uses)) {
    for (auto &repl : repls) {
      bool erased = false;
      for (auto *node : repl.from)
        erased |= nodes.erase(node);
      if (erased)
        nodes.insert(repl.to);
    }
  }
}
