#ifndef REWRITE_H
#define REWRITE_H

#include "Pattern.h"

struct Rewrite {
  // The left-hand side
  virtual Pattern *sourcePattern() = 0;
  // Apply the rewrite given a matched pattern
  virtual void apply(const Substitution &, EGraph &);
};

#endif // REWRITE_H
