// $Id$
//
// Usage: index-performance <type> [minsize=100M] [maxsize=100B] > <CSV-file>
//
// Measure performance of objectID/chuck indexing options over a range
// of index sizes, both initial filling and for 1M random queries.
// Sizes should be round powers of ten, tests will be done at 10x and
// 30x, up to (and including) the maximum size.
//
// The range of jobs can be omitted, and will default to 100M to 100B.
//
// Results of each test will be written to standard output as comma
// separated values (CSV).  The output may be redirected to a text file
// for input to Excel or GPlot.
//
// There are four different indexing options ([type]) currently defined:
//
// array	Simple C-style array of ints
// blocks	Set of separately allocated 1M int C-style arrays
// stdmap	Use std::map<> as key-value index
// file		Binary file storing ints; index is offset into file
// memcached	Key-value pairs registered to a Memcached server
// xrootd	Binary files storing ints, accessed via XRootD
// rocksdb	Key-value pairs registered to a RocksDB instance
//
// The type may be specified by the first character, if desired.

// 20151024  Michael Kelsey
// 20151028  Add std::map<> option
// 20151110  Add XRootD option, use preprocessor macros for Memcached, XRootD
// 20151125  Add RocksDB option, use preprocessor macro

#include "ArrayIndex.hh"
#include "BlockArrays.hh"
#include "MapIndex.hh"
#include "FileIndex.hh"
#ifdef HAS_MEMCACHED
#include "MemCDIndex.hh"
#endif
#ifdef HAS_XROOTD
#include "XrootdSimple.hh"
#endif
#ifdef HAS_ROCKSDB
#include "RocksIndex.hh"
#endif
#include <stdlib.h>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
using namespace std;


// Abbreviate "unsigned long long" for convenience

typedef unsigned long long ULL;

// Get testing driver based on type name

IndexTester* getTester(const string& type) {
  switch (type[0]) {
  case 'a': return new ArrayIndex; break;
  case 'b': return new BlockArrays; break;
  case 'f': return new FileIndex; break;
  case 's': return new MapIndex; break;
#ifdef HAS_MEMCACHED
  case 'm': return new MemCDIndex; break;
#endif
#ifdef HAS_XROOTD
  case 'x': return new XrootdSimple; break;
#endif
#ifdef HAS_ROCKSDB
  case 'r': return new RocksIndex; break;
#endif
  default:
    cerr << "ERROR: unknown indexing type " << type << endl;
    return 0;
  }
  return 0;
}


// Do logarithmic stepping (1 -> 3, 3 -> 10) of size

ULL NextSizeStep(ULL asize) {
  int oom = (int)floor(log10(asize));	// Order of magnitude of test
  ULL oomval = (ULL)pow(10.,oom);	// Convert back to number

  int step = (asize/oomval<3) ? 3 : 10;
  return step*oomval;
}


// Performance testing

int main(int argc, char* argv[]) {
  // Get command line arguments: type, minsize, maxsize
  if (argc<2) {
    cerr << "ERROR: indexing type must be specified" << endl;
    ::exit(1);
  }

  string type = argv[1];
  ULL minsize = (argc>2) ? strtoull(argv[2],0,0) : 100000000;
  ULL maxsize = (argc>3) ? strtoull(argv[3],0,0) : 100000000000;

  const long trials = 1000000;		// Might make this an argument later

  // Configure indexing test utility per user request
  IndexTester* tester = getTester(type);
  if (!tester) ::exit(2);

  string csvName = tester->GetName();
  csvName += ".csv";

  // Set up comma-separated data
  std::ofstream csv(csvName.c_str());
  tester->TestAndReport(0, 0, csv);

  // Loop over table sizes logarithmically (1, 3, 10, 30, etc.)
  ULL testsize = minsize;
  while (testsize <= maxsize) {
    tester->TestAndReport(testsize, trials, csv);
    if (testsize == maxsize) break;			// Final test exits

    testsize = min(NextSizeStep(testsize), maxsize);	// Don't exceed user max
  }

  // Job finished, clean up and exit
  csv.close();
  delete tester;
}
