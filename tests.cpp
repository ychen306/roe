#include "EGraph.h"
#include "gtest/gtest.h"

TEST(MakeTest, simple) {
  EGraph g;
  auto *x = g.make(0);
  auto *y = g.make(1);
  ASSERT_FALSE(g.isEquivalent(x, y));
  g.merge(x, y);
  ASSERT_TRUE(g.isEquivalent(x, y));
}
