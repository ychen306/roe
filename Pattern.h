#ifndef PATTERN_H
#define PATTERN_H

#include "EGraph.h"
#include <memory>
#include <vector>

class Pattern {
  bool isLeaf;
  Opcode opcode;
  std::vector<std::shared_ptr<Pattern>> operands;

  std::vector<std::pair<Pattern *, unsigned>> uses;

  Pattern() : isLeaf(true) {}
  Pattern(Opcode opcode, std::vector<std::shared_ptr<Pattern>> operands);

public:
  std::shared_ptr<Pattern>
  makePattern(Opcode opcode, std::vector<std::shared_ptr<Pattern>> operands) {
    return std::shared_ptr<Pattern>(new Pattern(opcode, operands));
  }

  std::shared_ptr<Pattern> makeVar() { return std::shared_ptr<Pattern>(); }

  bool isVar() const { return isLeaf; }

  Opcode getOpcode() const { return opcode; }

  llvm::ArrayRef<std::shared_ptr<Pattern>> getOperands() const {
    return operands;
  }

  void addUse(Pattern *user, unsigned operandId) {
    uses.emplace_back(user, operandId);
  }
};

#endif // PATTERN_H
