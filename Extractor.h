#ifndef EXTRACTOR_H
#define EXTRACTOR_H

#include "llvm/ADT/DenseMap.h"

class EGraphBase;
class ENode;
class EClassBase;

class Extractor {
public:
  using Result = llvm::DenseMap<EClassBase *, ENode *>;
  using Cost = float;
private:
  EGraphBase *g;
public:
  Extractor(EGraphBase *g) : g(g) {}
  // Trivial cost using ast size
  virtual Cost costOf(ENode *) { return 1; }
  Result extract(EClassBase *);
};

#endif // EXTRACTOR_H
