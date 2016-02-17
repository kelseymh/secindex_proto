#ifndef ARRAY_INDEX_HH
#define ARRAY_INDEX_HH 1
// $Id$
// ArrayIndex.hh -- Exercise performance of full memory array as lookup table.
//
// 20151023  Michael Kelsey

#include "IndexTester.hh"


class ArrayIndex : public IndexTester {
public:
  ArrayIndex(int verbose=0) : IndexTester("array",verbose), array(0) {;}
  virtual ~ArrayIndex() { delete[] array; }

protected:
  virtual void create(objectId_t asize);
  virtual chunkId_t value(objectId_t index);

private:
  chunkId_t* array;
};

#endif	/* ARRAY_INDEX_HH */
