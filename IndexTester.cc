// $Id$
// IndexTester.hh -- Interface base class for creating, exercising and
// recording performance of different two-column lookup tables.
//
// 20151023  Michael Kelsey
// 20151102  Add missing #includes reported by GCC 4.8.2
// 20160216  Add interface and optional subclass function for bulk updates

#include "IndexTester.hh"
#include <limits.h>
#include <stdlib.h>
#include <iostream>


// Constructor

IndexTester::IndexTester(const char* name, int verbose) :
  verboseLevel(verbose), tableSize(0ULL), indexStep(1), tableName(name),
  lastTrials(0L) {;}


// Generate random index spanning full size of table

objectId_t IndexTester::randomIndex() const {
  objectId_t rval = 0ULL;

  if (tableSize < LONG_MAX) rval = random()%tableSize;	// random() uses LONG
  else rval = (random()*ULONG_MAX + random()) % tableSize;

  return rval*indexStep;	// Sparsify random values
}


// Initialize new table via subclass, collecting performance statistics

void IndexTester::CreateTable(objectId_t asize) {
  if (verboseLevel) std::cout << "CreateTable " << asize << std::endl;

  tableSize = asize;		// Store value for random generation
  lastTrials = 0;

  usage.zero();
  usage.start();
  create(asize);
  usage.end();

  if (verboseLevel) std::cout << "Initialization " << usage << std::endl;
}


// Extend an existing table via subclass, collecting performance statistics

void IndexTester::UpdateTable(const char* datafile) {
  if (verboseLevel)
    std::cout << "UpdateTable " << (datafile?datafile:"") << std::endl;

  usage.zero();
  usage.start();
  update(datafile);
  usage.end();

  if (verboseLevel) std::cout << "Bulk update " << usage << std::endl;
}


// Multiple random accesses on table, collecting performance statistics

void IndexTester::ExerciseTable(long ntrials) {
  if (verboseLevel) std::cout << "ExerciseTable " << ntrials << std::endl;

  objectId_t idx;
  chunkId_t val;

  usage.zero();
  usage.start();
  for (long i=0; i<ntrials; i++) {
    idx = randomIndex();
    val = value(idx);
  }
  usage.end();

  lastTrials = ntrials;		// Store for later reporting

  if (verboseLevel) std::cout << "Total Accesses " << usage << std::endl;
}


// Generate test and print comma-separated data; asize=0 for column headings

void IndexTester::TestAndReport(objectId_t asize, long ntrials,
				std::ostream& csv) {
  if (asize == 0) {		// Special case: print column headings
    csv << "Type, Size (1e6), Init CPU (s), Init Clock (s)"
	<< ", Accesses (1e6), Run CPU (s), Run Clock (s)"
	<< ", Memory (MB), Page fault, Input op" << std::endl;
    return;
  }

  CreateTable(asize);
  csv << tableName << ", " << tableSize/1e6 << ", "
      << usage.cpuTime() << ", " << usage.elapsed()
      << std::flush;

  ExerciseTable(ntrials);
  csv << ", " << lastTrials/1e6 << ", " << usage.cpuTime()
      << ", " << usage.elapsed() << ", " << usage.maxMemory()/1e6 << ", "
      << usage.pageFaults() << ", " << usage.ioInput()
      << std::endl;

  cleanup();			// Remove job-specific data before next pass
}
