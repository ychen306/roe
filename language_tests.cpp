#include "EGraph.h"
#include "Pattern.h"
#include "Language.h"
#include "gtest/gtest.h"

#include "llvm/Support/raw_ostream.h"
using llvm::errs;

using Arith = Language<int>;
struct MyCommute : public LanguageRewrite<Arith> {
  MyCommute(Arith &l) : LanguageRewrite<Arith>(l) {
    root = match("add", var("x"), var("y"));
  }
  EClass *rhs(LookupFuncTy var) override {
    return make("add", var("y"), var("x"));
  };
};

TEST(LanguageRewriteTest, simple) {
  EGraph g;
  Arith arith(g, {"add"});
  auto *xy = arith.make("add", {arith.var("x"), arith.var("y")});
  auto *yx = arith.make("add", {arith.var("y"), arith.var("x")});
  ASSERT_NE(g.getLeader(xy), g.getLeader(yx));
}

TEST(LanguageRewriteTest, use_commute) {
  EGraph g;
  Arith arith(g, {"add"});
  auto *xy = arith.make("add", {arith.var("x"), arith.var("y")});
  auto *yx = arith.make("add", {arith.var("y"), arith.var("x")});

  std::vector<std::unique_ptr<Rewrite>> rewrites;
  rewrites.emplace_back(new MyCommute(arith));
  saturate(rewrites, g);
  ASSERT_EQ(g.getLeader(xy), g.getLeader(yx));
}

