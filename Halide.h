#ifndef HALIDE_H
#define HALIDE_H

#include "Language.h"
#include <optional>

// Halide TRS
class HalideTRS : public Language<int, HalideTRS> {
public:
  using AnalysisData = std::optional<int>;
  HalideTRS()
      : Language<int, HalideTRS>({
            "+",
            "-",
            "*",
            "/",
            "%",
            "max",
            "min",
            "<",
            ">",
            "<=",
            ">=",
            "==",
            "!=",
            "||",
            "&&",
        }) {}

  EClassBase *add(EClassBase *a, EClassBase *b) { return make("+", {a, b}); }
  EClassBase *sub(EClassBase *a, EClassBase *b) { return make("-", {a, b}); }
  EClassBase *mul(EClassBase *a, EClassBase *b) { return make("*", {a, b}); }
  EClassBase *div(EClassBase *a, EClassBase *b) { return make("/", {a, b}); }
  EClassBase *mod(EClassBase *a, EClassBase *b) { return make("%", {a, b}); }
  EClassBase *max(EClassBase *a, EClassBase *b) { return make("max", {a, b}); }
  EClassBase *min(EClassBase *a, EClassBase *b) { return make("min", {a, b}); }
  EClassBase *lt(EClassBase *a, EClassBase *b) { return make("<", {a, b}); }
  EClassBase *gt(EClassBase *a, EClassBase *b) { return make(">", {a, b}); }
  EClassBase *lte(EClassBase *a, EClassBase *b) { return make("<=", {a, b}); }
  EClassBase *gte(EClassBase *a, EClassBase *b) { return make(">=", {a, b}); }
  EClassBase *eq(EClassBase *a, EClassBase *b) { return make("==", {a, b}); }
  EClassBase *ne(EClassBase *a, EClassBase *b) { return make("!=", {a, b}); }
  EClassBase *and_(EClassBase *a, EClassBase *b) { return make("&&", {a, b}); }
  EClassBase *or_(EClassBase *a, EClassBase *b) { return make("||", {a, b}); }

  // e-class analysis
  AnalysisData analyze(ENode *node) {
    auto opcode = node->getOpcode();
    int x;
    if (is_constant(opcode, x))
      return x;

    auto operands = node->getOperands();
    if (operands.size() != 2)
      return std::nullopt;

    auto a = getData(operands[0]);
    auto b = getData(operands[1]);
    if (!a || !b)
      return std::nullopt;

    if (opcode == getOpcode("+"))
      return *a + *b;
    if (opcode == getOpcode("-"))
      return *a - *b;
    if (opcode == getOpcode("*"))
      return *a * *b;
    if (opcode == getOpcode("/") && *b != 0)
      return *a / *b;
    if (opcode == getOpcode("%") && *b != 0)
      return *a % *b;
    if (opcode == getOpcode("max"))
      return *a > *b ? *a : *b;
    if (opcode == getOpcode("min"))
      return *a > *b ? *a : *b;
    if (opcode == getOpcode("<"))
      return *a < *b;
    if (opcode == getOpcode(">"))
      return *a > *b;
    if (opcode == getOpcode("<="))
      return *a <= *b;
    if (opcode == getOpcode(">="))
      return *a >= *b;
    if (opcode == getOpcode("=="))
      return *a == *b;
    if (opcode == getOpcode("!="))
      return *a != *b;
    if (opcode == getOpcode("&&"))
      return *a && *b;
    if (opcode == getOpcode("||"))
      return *a || *b;
    return std::nullopt;
  }

  AnalysisData join(AnalysisData a, AnalysisData b) { return a ? a : b; }

  void modify(EClassBase *c) {
    if (auto x = getData(c))
      merge(c, constant(*x));
  }
};

REWRITE(HalideTRS, HAssoc, match("+", match("+", var("x"), var("y")), var("z")),
        make("+", var("x"), make("+", var("y"), var("z"))))

#endif // HALIDE_H
