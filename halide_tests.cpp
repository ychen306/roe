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

  auto *t1 = h.add(h.constant(1), h.constant(1));
  auto *t2 = h.constant(2);
  ASSERT_TRUE(h.isEquivalent(t1, t2));
}

TEST(HalideTest, simplify) {
  HalideTRS h;

  auto *t1 = h.add(h.add(h.var("x"), h.constant(1)), h.constant(1));
  auto *t2 = h.add(h.var("x"), h.constant(2));
  ASSERT_FALSE(h.isEquivalent(t1, t2));

  std::vector<std::unique_ptr<Rewrite<HalideTRS>>> rewrites;
  rewrites.emplace_back(new HAssoc(h));
  saturate<HalideTRS>(rewrites, h);
  ASSERT_TRUE(h.isEquivalent(t1, t2));
}
