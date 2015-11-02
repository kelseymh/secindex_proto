#ifndef MAP_INDEX_HH
#define MAP_INDEX_HH 1
// $Id$
// MapIndex.hh -- Exercise performance of std::map<> as lookup table.
//
// 20151028  Michael Kelsey

#include "IndexTester.hh"
#include <map>


class MapIndex : public IndexTester {
public:
  MapIndex(int verbose=0) : IndexTester("stdmap",verbose) {;}
  virtual ~MapIndex() {;}

protected:
  virtual void create(unsigned long long asize);
  virtual int value(unsigned long long index);

private:
  std::map<unsigned long long, int> map;
};

#endif	/* MAP_INDEX_HH */
