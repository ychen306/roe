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
#define wOr(a, b) make("||", a, b)
#define wAnd(a, b) make("&&", a, b)

#define a var("a")
#define b var("b")
#define c var("c")

#define x var("x")
#define y var("y")
#define z var("z")

// Add
REWRITE(HalideTRS, AddAssoc, mAdd(mAdd(a, b), c), wAdd(a, wAdd(b, c)))
REWRITE(HalideTRS, AddComm, mAdd(a, b), wAdd(b, a))
REWRITE(HalideTRS, AddZero, mAdd(a, mConst(0)), a)
REWRITE(HalideTRS, AddDistMul, mMul(a, mAdd(b, c)), wAdd(wMul(a, b), wMul(a, c)))
REWRITE(HalideTRS, AddFactMul, mAdd(mMul(a, b), mMul(a, c)), wMul(a, wAdd(b, c)))
REWRITE(HalideTRS, AddDenomMul, mAdd(mDiv(a, b), c), wDiv(wAdd(a, wMul(b, c)), b))
REWRITE(HalideTRS, AddDenomDiv, mDiv(mAdd(a, mMul(b, c)), b), wAdd(wDiv(a, b), c))
REWRITE(HalideTRS, AddDivMod, mAdd(mDiv(x, mConst(2)), mMod(x, mConst(2))),
                              wDiv(wAdd(x, constant(1)), constant(2)))

// Sub
REWRITE(HalideTRS, SubToAdd, mSub(a, b), wAdd(a, wMul(constant(-1), b)))

// Mul
REWRITE(HalideTRS, MulAssoc, mMul(mMul(a, b), c), wMul(a, wMul(b, c)))
REWRITE(HalideTRS, MulComm, mMul(a, b), wMul(b, a))
REWRITE(HalideTRS, MulZero, mMul(a, mConst(0)), constant(0))
REWRITE(HalideTRS, MulOne, mMul(a, mConst(1)), a)
REWRITE(HalideTRS, MulCancelDiv, mMul(mDiv(a, b), b), wSub(a, wMod(a, b)))
REWRITE(HalideTRS, MulMaxMin, mMul(mMax(a, b), mMin(a, b)), wMul(a, b))
REWRITE(HalideTRS, DivCancelMul, mDiv(mMul(y, x), x), y)


// Eq
REWRITE(HalideTRS, EqComm, mEq(a, b), wEq(b, a))
REWRITE(HalideTRS, EqSub0, mEq(x, y), wEq(wSub(x, y), constant(0)))
REWRITE(HalideTRS, EqSwap, mEq(mAdd(x, y), z), wEq(x, wSub(z, y)))
REWRITE(HalideTRS, EqRefl, mEq(x, x), constant(1))
REWRITE(HalideTRS, EqMul0, mEq(mMul(x, y), mConst(0)), wOr(wEq(x, constant(0)), wEq(y, constant(0))))
REWRITE(HalideTRS, EqMaxLt, mEq(mMax(x, y), y), wLte(x, y))
REWRITE(HalideTRS, EqMinLt, mEq(mMin(x, y), y), wLte(y, x))

std::vector<std::unique_ptr<Rewrite<HalideTRS>>> getRewrites(HalideTRS &h) {
  std::vector<std::unique_ptr<Rewrite<HalideTRS>>> rewrites;
  rewrites.emplace_back(new AddAssoc(h));
  rewrites.emplace_back(new AddComm(h));
  rewrites.emplace_back(new AddZero(h));
  rewrites.emplace_back(new AddDistMul(h));
  rewrites.emplace_back(new AddFactMul(h));
  rewrites.emplace_back(new AddDenomMul(h));
  rewrites.emplace_back(new AddDenomDiv(h));
  rewrites.emplace_back(new AddDivMod(h));
  rewrites.emplace_back(new SubToAdd(h));
  rewrites.emplace_back(new MulAssoc(h));
  rewrites.emplace_back(new MulComm(h));
  rewrites.emplace_back(new MulZero(h));
  rewrites.emplace_back(new MulOne(h));
  rewrites.emplace_back(new MulCancelDiv(h));
  rewrites.emplace_back(new MulMaxMin(h));
  rewrites.emplace_back(new DivCancelMul(h));
  rewrites.emplace_back(new EqComm(h));
  rewrites.emplace_back(new EqSub0(h));
  rewrites.emplace_back(new EqSwap(h));
  rewrites.emplace_back(new EqRefl(h));
  rewrites.emplace_back(new EqMul0(h));
  rewrites.emplace_back(new EqMaxLt(h));
  rewrites.emplace_back(new EqMinLt(h));
  return rewrites;
}
