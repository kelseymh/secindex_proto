#ifndef BLOCK_ARRAYS_HH
#define BLOCK_ARRAYS_HH 1
// $Id$
// ArrayIndex.hh -- Exercise performance of blocked-array in memory as
// lookup table.
//
// 20151024  Michael Kelsey

#include "IndexTester.hh"

class BlockArrays : public IndexTester {
public:
  BlockArrays(int verbose=0) : IndexTester("blocks",verbose), 
			       blockSize(1000000), blockCount(0), blocks(0) {;}
  virtual ~BlockArrays() { clearBlocks(); }

protected:
  virtual void create(unsigned long long asize);
  virtual int value(unsigned long long index);
  void clearBlocks();

private:
  const size_t blockSize;
  unsigned blockCount;
  int** blocks;
};

#endif	/* BLOCK_ARRAYS_HH */
