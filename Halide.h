#ifndef HALIDE_H
#define HALIDE_H

#include "Language.h"

// Halide TRS
class HalideTRS : public Language<int, HalideTRS> {
public:
  using AnalysisData =  int;
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
  // TODO: implement this
  AnalysisData analyze(ENode *) { return 0; }
};

#endif // HALIDE_H
