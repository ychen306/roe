#include "Halide.h"
#include "gtest/gtest.h"

TEST(HalideTest, simple) {
  HalideTRS h;

  auto *t1 = h.add(h.var("x"), h.var("y"));
  auto *t2 = h.add(h.var("x"), h.var("y"));
  ASSERT_EQ(t1, t2);
}

TEST(HalideTest, one_plus_one) {
  HalideTRS h;

  ASSERT_TRUE(
      h.isEquivalent(h.add(h.constant(1), h.constant(1)), h.constant(2)));
}
