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
    if (is_constant(opcode, x)) {
#ifndef NDEBUG
      if (auto y = getData(node->getClass()))
        assert(x == *y);
#endif
      return x;
    }

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

  AnalysisData join(AnalysisData a, AnalysisData b) { 
    assert(!a || !b || *a == *b);
    return a ? a : b;
  }

  void modify(EClassBase *c) {
    if (auto x = getData(c))
      merge(c, constant(*x));
  }

  void printIndent(int indent) {
    for (int i = 0; i < indent; i++)
      errs() << '\t';
  }

  void dump(ENode *node) override {
    auto varName = getVarName(node->getOpcode());
    auto opcodeName = getOpcodeName(node->getOpcode());
    if (varName != "") {
      errs() << varName << '\n';
    } else if (opcodeName != "") {
      errs() << "(" << opcodeName;
      for (auto *o : node->getOperands()) {
        errs() << ' ' << o;
        if (auto x = getData(o))
          errs() << "[data=" << *x << ']';
      }
      errs() << ")\n";
    } else {
      int c;
      bool isC = is_constant(node->getOpcode(), c);
      assert(isC);
      errs() << "(const " << c << ")\n";
    }
  }

  void dump(EClassBase *c) override {
    for (auto &nodes : llvm::make_second_range(c->getNodes())) {
      for (auto *node : nodes)
        dump(node);
    }
  }

  void dump() override {
    for (auto *c : llvm::make_range(class_begin(), class_end())) {
      errs() << "CLASS " << c << "{\n";
      dump(c);
      errs() << "}\n";
      errs() << "\t data = ";
      if (auto x = getData(c)) {
        errs() << *x << '\n';
      } else {
        errs() << "null\n";
      }
    }
  }

  void classRep(EClassBase *c) override {
    c = getLeader(c);
    errs() << c << "[data=";
    if (auto x = getData(c)) {
      errs() << *x << ']';
    } else {
      errs() << "null]";
    }
  }
};

std::vector<std::unique_ptr<Rewrite<HalideTRS>>> getRewrites(HalideTRS &h);

#endif // HALIDE_H
