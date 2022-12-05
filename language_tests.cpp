#include "EGraph.h"
#include "Language.h"
#include "Pattern.h"
#include "gtest/gtest.h"

#include "llvm/Support/raw_ostream.h"
using llvm::errs;

struct Arith : public Language<int, Arith>, NullAnalysis {
  Arith() : Language<int, Arith>({ "add" }) {}
};

REWRITE(Arith, ArithCommute, match("add", var("x"), var("y")),
        make("add", var("y"), var("x")))

TEST(LanguageRewriteTest, simple) {
  Arith arith;
  auto *xy = arith.make("add", {arith.var("x"), arith.var("y")});
  auto *yx = arith.make("add", {arith.var("y"), arith.var("x")});
  ASSERT_NE(arith.getLeader(xy), arith.getLeader(yx));
}

TEST(LanguageRewriteTest, use_commute) {
  Arith arith;
  auto *xy = arith.make("add", {arith.var("x"), arith.var("y")});
  auto *yx = arith.make("add", {arith.var("y"), arith.var("x")});

  std::vector<std::unique_ptr<Rewrite<Arith>>> rewrites;
  rewrites.emplace_back(new ArithCommute(arith));
  saturate<Arith>(rewrites, arith);
  ASSERT_EQ(arith.getLeader(xy), arith.getLeader(yx));
}
