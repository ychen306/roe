#include "EGraph.h"
#include "Pattern.h"
#include "gtest/gtest.h"

TEST(MakeTest, simple) {
  EGraph g;
  auto *x = g.make(0);
  auto *y = g.make(1);
  ASSERT_NE(g.getLeader(x), g.getLeader(y));
  g.merge(x, y);
  ASSERT_EQ(g.getLeader(x), g.getLeader(y));
}

TEST(MakeTest, hashcons) {
  EGraph g;
  ASSERT_EQ(g.make(0), g.make(0));
}

TEST(MakeTest, rebuild) {
  EGraph g;
  auto *x = g.make(0);
  auto *y = g.make(1);
  auto *fx = g.make(2, {x});
  auto *fy = g.make(2, {y});
  g.merge(x, y);
  ASSERT_EQ(g.getLeader(x), g.getLeader(y));
  ASSERT_NE(g.getLeader(fx), g.getLeader(fy));
  g.rebuild();
  ASSERT_EQ(g.getLeader(fx), g.getLeader(fy));
}

TEST(MakeTest, rebuild2) {
  EGraph g;
  auto *x = g.make(0);
  auto *y = g.make(1);
  auto *a = g.make(2);
  auto *b = g.make(3);

  auto *fxy = g.make(4, {x,y});
  auto *fab = g.make(4, {a,b});
  ASSERT_NE(g.getLeader(fxy), g.getLeader(fab));
  g.merge(x, a);
  g.rebuild();
  ASSERT_NE(g.getLeader(fxy), g.getLeader(fab));
  g.merge(y, b);
  g.rebuild();
  ASSERT_EQ(g.getLeader(fxy), g.getLeader(fab));
}

TEST(MakeTest, rebuild_nested) {
  EGraph g;
  auto *x = g.make(0);
  auto *y = g.make(1);
  auto *a = g.make(2);
  auto *b = g.make(3);

  auto *fxy = g.make(4, {x,y});
  auto *fab = g.make(4, {a,b});
  auto h0 = g.make(5, {fxy, x});
  auto h1 = g.make(5, {fab, a});
  ASSERT_NE(g.getLeader(h0), g.getLeader(h1));
  g.merge(x, a);
  g.merge(y, b);
  g.rebuild();
  ASSERT_EQ(g.getLeader(h0), g.getLeader(h1));
}

TEST(PatternTest, make) {
  auto x = Pattern::var();
  auto y = Pattern::var();
  auto add = Pattern::make(0, {x, y});
  ASSERT_TRUE(x->isVar());
  ASSERT_TRUE(y->isVar());
  ASSERT_FALSE(add->isVar());
  ASSERT_EQ(add->getOperands()[0], x);
  ASSERT_EQ(add->getOperands()[1], y);
}
