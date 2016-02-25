// $Id$
// ArrayIndex.hh -- Exercise performance of full memory array as lookup table.
//
// 20151023  Michael Kelsey
// 20160217  Force sequential indices, overriding user setting
// 20160224  Move destructor action to cleanup() function

#include "ArrayIndex.hh"


void ArrayIndex::cleanup() {
  delete[] array;
  array = 0;
}

// Construct single massive array in memory

void ArrayIndex::create(objectId_t asize) {
  SetIndexSpacing(1);			// Ensure that indices are dense

  if (array) cleanup();			// Avoid memory leaks
  if (asize>0) array = new chunkId_t[asize]();	// Fill with zeroes
}


// Access requested array element with existence check

chunkId_t ArrayIndex::value(objectId_t index) {
  return (array ? array[index] : 0xdeadbeef);
}
