#include "Pattern.h"
#include "llvm/ADT/STLExtras.h"

Pattern::Pattern(Opcode opcode,
                 std::vector<std::shared_ptr<Pattern>> theOperands)
    : isLeaf(false), opcode(opcode), operands(std::move(theOperands)) {
  for (auto item : llvm::enumerate(operands))
    item.value()->addUse(this, item.index());
}
