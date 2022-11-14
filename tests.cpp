#include "EGraph.h"
#include "Pattern.h"
#include "gtest/gtest.h"

#include "llvm/Support/raw_ostream.h"

TEST(MakeTest, simple) {
  EGraph g;
  auto *x = g.make(0);
  auto *y = g.make(1);
  ASSERT_NE(g.getLeader(x), g.getLeader(y));
  g.merge(x, y);
  ASSERT_EQ(g.getLeader(x), g.getLeader(y));
  ASSERT_EQ(std::distance(g.class_begin(), g.class_end()), 1);
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
  auto *usersOfX = g.getLeader(x)->getUsersByUses(2, 0);
  ASSERT_NE(usersOfX, nullptr);
  ASSERT_EQ(usersOfX->size(), 1);
}

TEST(MakeTest, rebuild2) {
  EGraph g;
  auto *x = g.make(0);
  auto *y = g.make(1);
  auto *a = g.make(2);
  auto *b = g.make(3);

  auto *fxy = g.make(4, {x, y});
  auto *fab = g.make(4, {a, b});
  ASSERT_NE(g.getLeader(fxy), g.getLeader(fab));
  g.merge(x, a);
  g.rebuild();
  ASSERT_NE(g.getLeader(fxy), g.getLeader(fab));
  g.merge(y, b);
  g.rebuild();
  ASSERT_EQ(g.getLeader(fxy), g.getLeader(fab));
  auto *usersOfX = g.getLeader(x)->getUsersByUses(4, 0);
  ASSERT_EQ(usersOfX->size(), 1);
  auto *usersOfB = g.getLeader(b)->getUsersByUses(4, 1);
  ASSERT_EQ(usersOfB->size(), 1);
  ASSERT_EQ(std::distance(g.class_begin(), g.class_end()), 3);
}

TEST(MakeTest, rebuild_nested) {
  EGraph g;
  auto *x = g.make(0);
  auto *y = g.make(1);
  auto *a = g.make(2);
  auto *b = g.make(3);

  auto *fxy = g.make(4, {x, y});
  auto *fab = g.make(4, {a, b});
  auto h0 = g.make(5, {fxy, x});
  auto h1 = g.make(5, {fab, a});
  ASSERT_NE(g.getLeader(h0), g.getLeader(h1));
  g.merge(x, a);
  g.merge(y, b);
  g.rebuild();
  ASSERT_EQ(g.getLeader(h0), g.getLeader(h1));
  ASSERT_EQ(std::distance(g.class_begin(), g.class_end()), 4);
}

TEST(MakeTest, rebuild_nested2) {
  EGraph g;
  auto *x = g.make(0);
  auto *y = g.make(1);
  auto *z = g.make(2);
  auto *a = g.make(3);
  auto *b = g.make(4);
  auto *c = g.make(5);

  auto *fxy = g.make(6, {x, y});
  auto *fab = g.make(6, {a, b});
  auto h0 = g.make(7, {fxy, z});
  auto h1 = g.make(7, {fab, c});
  ASSERT_NE(g.getLeader(h0), g.getLeader(h1));
  g.merge(x, a);
  g.merge(y, b);
  g.rebuild();
  ASSERT_NE(g.getLeader(h0), g.getLeader(h1));
  g.merge(z, c);
  g.rebuild();
  ASSERT_EQ(g.getLeader(h0), g.getLeader(h1));
}

TEST(PatternTest, make) {
  auto x = Pattern::var();
  auto y = Pattern::var();
  auto f = Pattern::make(42, {x, y});
  ASSERT_TRUE(x->isVar());
  ASSERT_TRUE(y->isVar());
  ASSERT_FALSE(f->isVar());
  ASSERT_EQ(f->getOperands()[0], x);
  ASSERT_EQ(f->getOperands()[1], y);
}

TEST(MatchTest, simple) {
  auto px = Pattern::var();
  auto py = Pattern::var();
  auto pf = Pattern::make(42, {px, py});

  EGraph g;
  auto x = g.make(0);
  auto y = g.make(1);
  auto f = g.make(42, {x, y});
  auto matches = match(pf.get(), g);
  ASSERT_EQ(matches.size(), 1);

  auto &m = matches.front();
  llvm::DenseMap<Pattern *, EClass *> subst(m.begin(), m.end());
  ASSERT_EQ(subst[px.get()], x);
  ASSERT_EQ(subst[py.get()], y);
}

TEST(MatchTest, simple2) {
  auto px = Pattern::var();
  auto py = Pattern::var();
  auto pf = Pattern::make(42, {px, py});

  EGraph g;
  auto x = g.make(0);
  auto y = g.make(1);
  auto a = g.make(2);
  auto b = g.make(3);
  g.make(42, {x, y});
  g.make(42, {a, b});
  auto matches = match(pf.get(), g);
  ASSERT_EQ(matches.size(), 2);

  auto &m0 = matches[0];
  auto &m1 = matches[1];

  llvm::DenseMap<Pattern *, EClass *> subst1(m0.begin(), m0.end());
  llvm::DenseMap<Pattern *, EClass *> subst2(m1.begin(), m1.end());
  if (subst1[px.get()] == x) {
    ASSERT_EQ(subst1[px.get()], x);
    ASSERT_EQ(subst1[py.get()], y);
    ASSERT_EQ(subst2[px.get()], a);
    ASSERT_EQ(subst2[py.get()], b);
  } else {
    ASSERT_EQ(subst2[px.get()], x);
    ASSERT_EQ(subst2[py.get()], y);
    ASSERT_EQ(subst1[px.get()], a);
    ASSERT_EQ(subst1[py.get()], b);
  }
}
