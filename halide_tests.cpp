#include "Halide.h"
#include "gtest/gtest.h"

TEST(HalideTest, simple) {
  HalideTRS halide;

  auto *t1 = halide.add(halide.var("x"), halide.var("y"));
  auto *t2 = halide.add(halide.var("x"), halide.var("y"));
  ASSERT_EQ(t1, t2);
}

TEST(HalideTest, one_plus_one) {
  HalideTRS halide;

  auto *t = halide.add(halide.constant(1), halide.constant(1));
  ASSERT_TRUE(halide.isEquivalent(t, halide.constant(2)));
}
