#include "Halide.h"

// Match
#define mAdd(a, b) match("+", a, b)
#define mSub(a, b) match("-", a, b)
#define mMul(a, b) match("*", a, b)
#define mDiv(a, b) match("/", a, b)
#define mMod(a, b) match("%", a, b)
#define mMax(a, b) match("max", a, b)
#define mMin(a, b) match("min", a, b)
#define mLt(a, b) match("<", a, b)
#define mGt(a, b) match(">", a, b)
#define mLte(a, b) match("<=", a, b)
#define mGte(a, b) match(">=", a, b)
#define mEq(a, b) match("==", a, b)
#define mNe(a, b) match("!=", a, b)
#define mOr(a, b) match("||", a, b)
#define mAnd(a, b) match("&&", a, b)

// Write
#define wAdd(a, b) make("+", a, b)
#define wSub(a, b) make("-", a, b)
#define wMul(a, b) make("*", a, b)
#define wDiv(a, b) make("/", a, b)
#define wMod(a, b) make("%", a, b)
#define wMax(a, b) make("max", a, b)
#define wMin(a, b) make("min", a, b)
#define wLt(a, b) make("<", a, b)
#define wGt(a, b) make(">", a, b)
#define wLte(a, b) make("<=", a, b)
#define wGte(a, b) make(">=", a, b)
#define wEq(a, b) make("==", a, b)
#define wNe(a, b) make("!=", a, b)
#define wOr_(a, b) make("||", a, b)
#define wAnd_(a, b) make("&&", a, b)

#define lx var("x")
#define ly var("y")
#define lz var("z")
#define rx var("x")
#define ry var("y")
#define rz var("z")

REWRITE(HalideTRS, AddAssoc, mAdd(mAdd(lx, ly), lz),
        wAdd(rx, wAdd(ry, rz)))

std::vector<std::unique_ptr<Rewrite<HalideTRS>>> getRewrites(HalideTRS &h) {
  std::vector<std::unique_ptr<Rewrite<HalideTRS>>> rewrites;
  rewrites.emplace_back(new AddAssoc(h));
  return rewrites;
}
