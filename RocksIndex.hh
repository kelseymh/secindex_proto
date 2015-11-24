#ifndef ROCKS_INDEX_HH
#define ROCKS_INDEX_HH 1
// $Id$
// RocksIndex.hh -- Exercise performance of RocksDB for lookup table.
//
// 20151124  Michael Kelsey

#include "IndexTester.hh"

namespace rocksdb { class DB; }


class RocksIndex : public IndexTester {
public:
  RocksIndex(int verbose=0);
  virtual ~RocksIndex();

protected:
  virtual void create(unsigned long long asize);
  virtual int value(unsigned long long index);

private:
  rocksdb::DB* rocksDB;		// Database to store secondary index
};

#endif	/* ROCKS_INDEX_HH */
