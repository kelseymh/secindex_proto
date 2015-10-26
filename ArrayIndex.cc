// $Id$
// ArrayIndex.hh -- Exercise performance of full memory array as lookup table.
//
// 20151023  Michael Kelsey

#include "ArrayIndex.hh"


// Construct single massive array in memory

void ArrayIndex::create(unsigned long long asize) {
  if (array) delete[] array;			// Avoid memory leaks
  if (asize>0) array = new int[asize]();	// Fill with zeroes
}


// Access requested array element with existence check

int ArrayIndex::value(unsigned long long index) {
  return (array ? array[index] : 0xdeadbeef);
}
