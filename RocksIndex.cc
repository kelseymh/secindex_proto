// $Id$
// RocksIndex.hh -- Exercise performance of RocksDB interface as lookup table.
//
// 20151124  Michael Kelsey

#include "RocksIndex.hh"
#include "rocksdb/db.h"
#include "rocksdb/table.h"
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sstream>
#include <iostream>
using namespace std;

// Constructor and destructor

RocksIndex::RocksIndex(int verbose)
  : IndexTester("rocksdb",verbose), rocksDB(0) {;}

RocksIndex::~RocksIndex() {
  if (rocksDB) delete rocksDB;
}


// Create new database for testing
void RocksIndex::create(objectId_t asize) {
  if (asize==0) return;		// Don't create a null object!

  if (rocksDB) delete rocksDB;

  rocksDB = 0;

  // Buffer for all RocksDB activity, to report errors
  rocksdb::Status dbstat;

  // Use Bloom filter with a large block size to improve lookup speed
  rocksdb::Options options;
  options.create_if_missing = true;
  options.stats_dump_period_sec = 30;		// Lots of dumps!
  options.max_background_flushes = 4;		// Improves writing efficiency
  options.max_background_compactions = 4;
  options.OptimizeForPointLookup(10);		// Bloom filter, units are MB
  options.memtable_prefix_bloom_bits = 8000000;
  //*** options.max_open_files = 150;		// Avoids "too many files" error
  options.max_open_files = -1;
  options.target_file_size_base = 64e6;		// Allow all files to stay open
  //*** options.target_file_size_multiplier = 2;

  rocksdb::BlockBasedTableOptions tblopt;
  tblopt.checksum = rocksdb::kxxHash;		// Default crc32 doesn't work?!?
  options.table_factory.reset(NewBlockBasedTableFactory(tblopt));

  dbstat = rocksdb::DB::Open(options, "/tmp/rocksdb", &rocksDB);
  if (!dbstat.ok()) {
    cerr << "Failed to create RocksDB database in /tmp/rocksdb\n"
	 << dbstat.ToString() << endl;
    rocksDB = 0;
    return;
  }

  if (verboseLevel>1) cout << "Filling " << asize << " keys" << endl;

  // Do batch-based filling here for maximum efficiency
  static const int zero=0;		// Write the same value every time
  rocksdb::WriteBatch batch;
  for (objectId_t i=0; i<asize; i++) {
    rocksdb::Slice key((const char*)&i, sizeof(objectId_t));
    rocksdb::Slice val((const char*)&zero, sizeof(int));
    batch.Put(key, val);

    if (i%1000000 == 999999) {		// Flush buffer every million objects
      dbstat = rocksDB->Write(rocksdb::WriteOptions(), &batch);
      if (!dbstat.ok()) {
	cerr << "Failed to write data block " << i/1000000 << ": "
	     << dbstat.ToString() << endl;
	batch.Clear();
	break;
      }
      batch.Clear();
    }
  }

  if (batch.Count() > 0) {		// Flush any residual objects
    dbstat = rocksDB->Write(rocksdb::WriteOptions(), &batch);
    if (!dbstat.ok()) {
      cerr << "Failed to write final data block: " << dbstat.ToString() << endl;
    }
  }
}


// Query database to get requested index entry

chunkId_t RocksIndex::value(objectId_t index) {
  if (!rocksDB) return 0xdeadbeef;	// Include sanity check

  if (verboseLevel>1) cout << "Looking for " << index;

  // NOTE:  RocksDB only returns std::string values; must use as buffer
  std::string valbuf(sizeof(chunkId_t), '\0');
  rocksdb::Slice key((const char*)&index, sizeof(objectId_t));

  rocksdb::Status dbstat = rocksDB->Get(rocksdb::ReadOptions(), key, &valbuf);
  if (!dbstat.ok()) {
    cerr << "\nFailed to read " << index << ": " << dbstat.ToString() << endl;
    return 0xdeadbeef;
  }

  // Interpret byte contents of string as integer value
  if (verboseLevel>1)
    cout << " got value " << *(const chunkId_t*)(valbuf.data()) << endl;

  return *(const chunkId_t*)(valbuf.data());
}
