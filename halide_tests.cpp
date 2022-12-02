#include "Halide.h"
#include "gtest/gtest.h"

TEST(HalideTest, simple) {
  EGraph<> g;
  HalideTRS halide(g);

  auto *t1 = halide.add(halide.var("x"), halide.var("y"));
  auto *t2 = halide.add(halide.var("x"), halide.var("y"));
  ASSERT_EQ(t1, t2);
}
