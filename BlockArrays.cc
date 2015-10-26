// $Id$
// ArrayIndex.hh -- Exercise performance of blocked-array in memory as
// lookup table.
//
// 20151024  Michael Kelsey

#include "BlockArrays.hh"


// Create a set of subarrays to cover the whole index range

void BlockArrays::create(unsigned long long asize) {
  if (blocks) clearBlocks();		// Avoid memory leaks
  blockCount = 0;
  
  if (asize==0) return;
  
  blockCount = asize / blockSize;
  blocks = new int* [blockCount]();
  for (unsigned i=0; i<blockCount; i++) {
    blocks[i] = new int[blockSize]();		// Fill with zeroes
  }
}


// Access requested array element with existence check

int BlockArrays::value(unsigned long long index) {
  if (blockCount == 0 || blocks == 0) return 0xdeadbeef;
  return blocks[index/blockSize][index%blockSize];
}


// Delete array block-by-block first, then the top level
void BlockArrays::clearBlocks() {
  if (blocks) {
    for (unsigned i=0; i<blockCount; i++) {
      delete[] blocks[i];
      blocks[i] = 0;
    }
    delete[] blocks;
  }
  blockCount = 0;
}
