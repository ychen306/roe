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

TEST(PatternTest, make) {
  auto x = Pattern::var();
  auto y = Pattern::var();
  auto add = Pattern::make(0, {x, y});
}
