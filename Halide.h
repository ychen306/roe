#ifndef HALIDE_H
#define HALIDE_H

#include "Language.h"

// Halide TRS
class HalideTRS : public Language<int> {
public:
  HalideTRS(EGraph &g) : Language<int>(g, {
      "+", "-", "*", "/", "%", 
      "max", "min", "<", ">",
      "<=", ">=", "==", "!=", "||", "&&",
      }) {}

  EClass *add(EClass *a, EClass *b) {
    return make("+", {a, b});
  }
  EClass *sub(EClass *a, EClass *b) {
    return make("-", {a, b});
  }
  EClass *mul(EClass *a, EClass *b) {
    return make("*", {a, b});
  }
  EClass *div(EClass *a, EClass *b) {
    return make("/", {a, b});
  }
  EClass *mod(EClass *a, EClass *b) {
    return make("%", {a, b});
  }
  EClass *max(EClass *a, EClass *b) {
    return make("max", {a, b});
  }
  EClass *min(EClass *a, EClass *b) {
    return make("min", {a, b});
  }
  EClass *lt(EClass *a, EClass *b) {
    return make("<", {a, b});
  }
  EClass *gt(EClass *a, EClass *b) {
    return make(">", {a, b});
  }
  EClass *lte(EClass *a, EClass *b) {
    return make("<=", {a, b});
  }
  EClass *gte(EClass *a, EClass *b) {
    return make(">=", {a, b});
  }
  EClass *eq(EClass *a, EClass *b) {
    return make("==", {a, b});
  }
  EClass *ne(EClass *a, EClass *b) {
    return make("!=", {a, b});
  }
  EClass *and_(EClass *a, EClass *b) {
    return make("&&", {a, b});
  }
  EClass *or_(EClass *a, EClass *b) {
    return make("||", {a, b});
  }
};

#endif // HALIDE_H
