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
public:
  virtual ~Extractor();
  // Trivial cost using ast size
  virtual Cost costOf(ENode *) { return 1; }
  Result extract(EClassBase *);
};

#endif // EXTRACTOR_H
