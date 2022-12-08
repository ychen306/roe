#include "Extractor.h"
#include "EGraph.h"

Extractor::Result Extractor::extract(EClassBase *c) {
  llvm::DenseMap<ENode *, Cost> totalCosts;
  // Figure out the total cost of using a given node;
  std::function<Cost (ENode *)> totalCostOf = [&](ENode *node) -> Cost {
    // Insert a null cost to avoid infinite loop for circular nodes/classes
    auto [it, inserted] = totalCosts.try_emplace(node, 0);
    if (!inserted)
      return it->second;
    Cost cost = costOf(node);
    for (auto *o : node->getOperands()) {
      Cost bestCost = -1;
      for (auto &childNodes : llvm::make_second_range(o->getNodes())) {
        for (auto *n2 : childNodes) {
          Cost childCost = totalCostOf(n2);
          if (bestCost < 0 || childCost < bestCost)
            bestCost = childCost;
        }
      }
      cost += bestCost;
    }
    return totalCosts[node] = cost;
  };

  llvm::SmallVector<EClassBase *> worklist {c};
  Result result;
  while (!worklist.empty()) {
    auto *c = worklist.pop_back_val();
    Cost bestCost;
    ENode *bestNode = nullptr;
    for (auto &nodes : llvm::make_second_range(c->getNodes())) {
      for (auto *node : nodes) {
        Cost cost = totalCostOf(node);
        if (!bestNode || cost < bestCost) {
          bestNode = node;
          bestCost = cost;
        }
      }
    }
    assert(bestNode);
    result[c] = bestNode;
    worklist.append(bestNode->operand_begin(), bestNode->operand_end());
  }
  return result;
}
