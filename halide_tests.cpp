#include "Halide.h"
#include "Extractor.h"
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
  ASSERT_EQ(std::distance(h.class_begin(), h.class_end()), 2);
}

TEST(HalideTest, simplify) {
  HalideTRS h;

  auto *t1 = h.add(h.add(h.var("x"), h.constant(1)), h.constant(1));
  auto *t2 = h.add(h.var("x"), h.constant(2));
  ASSERT_FALSE(h.isEquivalent(t1, t2));

  saturate<HalideTRS>(getRewrites(h), h);
  ASSERT_TRUE(h.isEquivalent(t1, t2));
}

TEST(HalideTest, add_zero) {
  HalideTRS h;
  auto *t = h.add(h.constant(0), h.var("x"));
  saturate<HalideTRS>(getRewrites(h), h);
  ASSERT_TRUE(h.isEquivalent(t, h.var("x")));
}

#if 1
TEST(HalideTest, caviar_test) {
  HalideTRS h;
  auto *v0 = h.var("v0");
  auto *v1 = h.var("v1");
  auto *a = h.sub(h.add(v0, v1), h.constant(16));
  auto *b = h.sub(h.add(h.sub(h.add(v0, v1), h.constant(16)), h.constant(143)), h.constant(1));
  auto *t = h.eq(a, b);
  saturate<HalideTRS>(getRewrites(h), h, 10);
  ASSERT_TRUE(h.isEquivalent(t, h.constant(0)));
}
#endif

TEST(HalideTest, extract_simple) {
  HalideTRS h;
  auto *t = h.add(h.constant(0), h.var("x"));
  saturate<HalideTRS>(getRewrites(h), h);
  auto extracted = Extractor().extract(t);
  auto *node = extracted.lookup(t->getLeader());
  ASSERT_TRUE(node);
  ASSERT_EQ(node->getOperands().size(), 0);
  ASSERT_EQ(node->getOpcode(), h.getVariableOpcode("x"));
}
