// $Id$
// RocksIndex.hh -- Exercise performance of RocksDB interface as lookup table.
//
// 20151124  Michael Kelsey

#include "RocksIndex.hh"
#include "rocksdb/db.h"
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
void RocksIndex::create(unsigned long long asize) {
  if (asize==0) return;		// Don't create a null object!

  if (rocksDB) delete rocksDB;

  rocksDB = 0;
  rocksdb::Options options;
  options.create_if_missing = true;
  if (!rocksdb::DB::Open(options, "/tmp/rocksdb", &rocksDB).ok()) {
    cerr << "Failed to create RocksDB file /tmp/rocksdb" << endl;
    rocksDB = 0;
    return;
  }

  if (verboseLevel>1) cout << "Filling " << asize << " keys" << endl;

  // Do batch-based filling here for maximum efficiency
  static const int zero=0;		// Write the same value every time
  rocksdb::WriteBatch batch;
  rocksdb::Status dbstat;
  for (unsigned long long i=0; i<asize; i++) {
    rocksdb::Slice key((const char*)&i, sizeof(unsigned long long));
    rocksdb::Slice val((const char*)&zero, sizeof(int));
    batch.Put(key, val);

    if (i%1000000 == 999999) {		// Flush buffer every million objects
      if (!rocksDB->Write(rocksdb::WriteOptions(), &batch).ok()) {
	cerr << "Failed to write data block " << i/1000000 << endl;
	batch.Clear();
	break;
      }
      batch.Clear();
    }
  }

  if (batch.Count() > 0) {		// Flush any residual objects
    if (!rocksDB->Write(rocksdb::WriteOptions(), &batch).ok()) {
      cerr << "Failed to write final data block" << endl;
    }
  }
}


// Query database to get requested index entry

int RocksIndex::value(unsigned long long index) {
  if (!rocksDB) return 0xdeadbeef;	// Include sanity check

  if (verboseLevel>1) cout << "Looking for " << index;

  // NOTE:  RocksDB only returns std::string values; must use as buffer
  std::string valbuf(sizeof(int), '\0');
  rocksdb::Slice key((const char*)&index, sizeof(unsigned long long));
  if (!rocksDB->Get(rocksdb::ReadOptions(), key, &valbuf).ok()) {
    cerr << "Failed to read " << index << endl;
    return 0xdeadbeef;
  }

  // Interpret byte contents of string as integer value
  if (verboseLevel>1)
    cout << " got value " << *(const int*)(valbuf.data()) << endl;

  return *(const int*)(valbuf.data());
}
