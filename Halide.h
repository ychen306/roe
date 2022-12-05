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

// Match
#define mAdd(a, b) match("+", a, b)
#define mSub(a, b) match("-", a, b)
#define mMul(a, b) match("*", a, b)
#define mDiv(a, b) match("/", a, b)
#define mMod(a, b) match("%", a, b)
#define mMax(a, b) match("max", a, b)
#define mMin(a, b) match("min", a, b)
#define mLt(a, b) match("<", a, b)
#define mGt(a, b) match(">", a, b)
#define mLte(a, b) match("<=", a, b)
#define mGte(a, b) match(">=", a, b)
#define mEq(a, b) match("==", a, b)
#define mNe(a, b) match("!=", a, b)
#define mOr(a, b) match("||", a, b)
#define mAnd(a, b) match("&&", a, b)

// Write
#define wAdd(a, b) make("+", a, b)
#define wSub(a, b) make("-", a, b)
#define wMul(a, b) make("*", a, b)
#define wDiv(a, b) make("/", a, b)
#define wMod(a, b) make("%", a, b)
#define wMax(a, b) make("max", a, b)
#define wMin(a, b) make("min", a, b)
#define wLt(a, b) make("<", a, b)
#define wGt(a, b) make(">", a, b)
#define wLte(a, b) make("<=", a, b)
#define wGte(a, b) make(">=", a, b)
#define wEq(a, b) make("==", a, b)
#define wNe(a, b) make("!=", a, b)
#define wOr_(a, b) make("||", a, b)
#define wAnd_(a, b) make("&&", a, b)

REWRITE(HalideTRS, HAssoc, mAdd(mAdd(var("x"), var("y")), var("z")),
        wAdd(var("x"), wAdd(var("y"), var("z"))))

#endif // HALIDE_H
