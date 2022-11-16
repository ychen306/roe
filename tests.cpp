#include "EGraph.h"
#include "Pattern.h"
#include "gtest/gtest.h"

#include "llvm/Support/raw_ostream.h"
using llvm::errs;

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

// Example from the relational e-matching paper
TEST(MatchTest, ex1) {
  EGraph g;

  int n = 100;
  int opcode_f = n + 1;
  int opcode_h = n + 2;

  std::vector<EClass *> xs;
  for (int i = 0; i < n; i++)
    xs.push_back(g.make(i));

  std::vector<EClass *> hs;
  for (int i = 0; i < n; i++)
    hs.push_back(g.make(opcode_h, {xs[i]}));

  std::vector<EClass *> fs;
  for (int i = 0; i < n; i++)
    fs.push_back(g.make(opcode_f, {xs[i], hs[i]}));

  ASSERT_EQ(std::distance(g.class_begin(), g.class_end()), 3 * n);

  for (int i = 0; i < n; i++) {
    g.merge(fs[0], fs[i]);
    g.merge(hs[0], hs[i]);
  }

  ASSERT_EQ(std::distance(g.class_begin(), g.class_end()), 2 + n);
  for (int i = 1; i < n; i++)
    ASSERT_NE(g.getLeader(xs[i-1]), g.getLeader(xs[i]));

  auto alpha = Pattern::var();
  auto ph = Pattern::make(opcode_h, {alpha});
  auto pf = Pattern::make(opcode_f, {alpha, ph});
  auto matches = match(pf.get(), g);
  ASSERT_EQ(matches.size(), n);
}

// Copied from egg's unit test
TEST(MatchTest, nonlinear) {
  EGraph g;
  auto a = g.make(0);
  auto b = g.make(1);
  int zero = 2, one = 3;
  auto x = g.make(zero);
  auto y = g.make(one);
  int f_opcode = 100, h_opcode = 200, g_opcode = 300, foo = 400;

  g.make(f_opcode, {a, a});
  g.make(f_opcode, {a, g.make(g_opcode, {a})});
  g.make(f_opcode, {a, g.make(g_opcode, {b})});
  g.make(h_opcode, {g.make(foo, {a, b}), x, y});
  g.make(h_opcode, {g.make(foo, {a, b}), y, x});
  g.make(h_opcode, {g.make(foo, {a, b}), x, x});

  {
    auto x = Pattern::var();
    auto y = Pattern::var();
    auto p_fxy = Pattern::make(f_opcode, {x, y});
    ASSERT_EQ(match(p_fxy.get(), g).size(), 3);
  }

  {
    auto x = Pattern::var();
    auto p_fxx = Pattern::make(f_opcode, {x, x});
    ASSERT_EQ(match(p_fxx.get(), g).size(), 1);
  }

  {
    auto x = Pattern::var();
    auto y = Pattern::var();
    auto p_gy = Pattern::make(g_opcode, {y});
    auto p_fxgy = Pattern::make(f_opcode, {x, p_gy});
    ASSERT_EQ(match(p_fxgy.get(), g).size(), 2);
  }

  {
    auto x = Pattern::var();
    auto p_gx = Pattern::make(g_opcode, {x});
    auto p_fxgx = Pattern::make(f_opcode, {x, p_gx});
    ASSERT_EQ(match(p_fxgx.get(), g).size(), 1);
  }

  {
    auto x = Pattern::var();
    auto p_zero = Pattern::make(zero, {});
    auto p = Pattern::make(h_opcode, {x, p_zero, p_zero});
    ASSERT_EQ(match(p.get(), g).size(), 1);
  }

  {
    auto x = Pattern::var();
    auto y = Pattern::var();
    auto p_zero = Pattern::make(zero, {});
    auto p = Pattern::make(h_opcode, {x, p_zero, y});
    ASSERT_EQ(match(p.get(), g).size(), 2);
  }
}

struct Commute : public Rewrite {
  PatternPtr x, y;
  Opcode opcode;

  Commute(Opcode opcode) : opcode(opcode) {
    x = var();
    y = var();
    root = make(opcode, x, y);
  }

  EClass *apply(const PatternToClassMap &m, EGraph &g) override {
    return g.make(opcode, {m.lookup(y.get()), m.lookup(x.get())});
  }
};

struct Assoc : public Rewrite {
  PatternPtr x, y, z;
  Opcode opcode;

  Assoc(Opcode opcode) : opcode(opcode) {
    x = var();
    y = var();
    z = var();
    root = make(opcode, make(opcode, x, y), z);
  }

  EClass *apply(const PatternToClassMap &m, EGraph &g) override {
    auto yz = g.make(opcode, {m.lookup(y.get()), m.lookup(z.get())});
    return g.make(opcode, {m.lookup(x.get()), yz});
  }
};

TEST(RewriteTest, commute) {
  EGraph g;
  int add = 100;
  auto a = g.make(0);
  auto b = g.make(1);
  auto ab = g.make(add, {a, b});
  auto ba = g.make(add, {b, a});
  ASSERT_NE(g.getLeader(ab), g.getLeader(ba));

  Commute commute(add);
  auto matches = match(commute.sourcePattern(), g);
  commute.applyMatches(matches, g);
  g.rebuild();
  ASSERT_EQ(g.getLeader(ab), g.getLeader(ba));
}

TEST(RewriteTest, assoc) {
  EGraph g;
  int add = 100;
  auto a = g.make(0);
  auto b = g.make(1);
  auto c = g.make(2);
  auto ab = g.make(add, {a, b});
  auto bc = g.make(add, {b, c});
  auto ab_c = g.make(add, {ab, c});
  auto a_bc = g.make(add, {a, bc});
  ASSERT_NE(g.getLeader(ab_c), g.getLeader(a_bc));

  Assoc assoc(add);
  auto matches = match(assoc.sourcePattern(), g);
  assoc.applyMatches(matches, g);
  g.rebuild();
  ASSERT_EQ(g.getLeader(ab_c), g.getLeader(a_bc));
}
