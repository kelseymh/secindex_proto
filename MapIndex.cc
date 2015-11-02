// $Id$
// MapIndex.cc -- Exercise performance of std::map<> as lookup table.
//
// 20151028  Michael Kelsey

#include "MapIndex.hh"
#include <map>


// Populate map with full range of keys, all values zero

void MapIndex::create(unsigned long long asize) {
  map.clear();
  if (asize == 0) return;		// Avoid unnecessary work

  for (unsigned long long i=0; i<asize; map[i++]=0) {;}
}


// Return chunk only if index was registered

int MapIndex::value(unsigned long long index) {
  return (map.find(index)!=map.end()) ? map[index] : 0xdeadbeef;
}
