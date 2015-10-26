// $Id$
// IndexTester.hh -- Interface base class for creating, exercising and
// recording performance of different two-column lookup tables.
//
// 20151023  Michael Kelsey

#include "IndexTester.hh"
#include <iostream>



// Generate random index spanning full size of table

unsigned long long IndexTester::randomIndex() const {
  if (tableSize < LONG_MAX) return random()%tableSize;	// random() uses LONG
  return (random()*ULONG_MAX + random()) % tableSize;	// or construct LLONG
}


// Initialize new table via subclass, collecting performance statistics

void IndexTester::CreateTable(unsigned long long asize) {
  if (verboseLevel>1) std::cout << "CreateTable " << asize << std::endl;

  tableSize = asize;		// Store value for random generation
  lastTrials = 0;

  usage.zero();
  usage.start();
  create(asize);
  usage.end();

  if (verboseLevel) std::cout << "Initialization " << usage << std::endl;
}


// Multiple random accesses on table, collecting performance statistics

void IndexTester::ExerciseTable(long ntrials) {
  if (verboseLevel>1) std::cout << "ExerciseTable " << ntrials << std::endl;

  int idx, val;
  usage.zero();
  for (int i=0; i<ntrials; i++) {
    idx = randomIndex();	// Don't count random() in performance

    usage.start();
    val = value(idx);
    usage.end();
  }

  lastTrials = ntrials;		// Store for later reporting

  if (verboseLevel) std::cout << "Total Accesses " << usage << std::endl;
}


// Generate test and print comma-separated data; asize=0 for column headings

void IndexTester::TestAndReport(unsigned long long asize, long ntrials,
				std::ostream& csv) {
  if (asize == 0) {		// Special case: print column headings
    csv << "Type, Size (1e6), Init uTime (s), Init sTime (s), Init Clock (s)"
	<< ", Accesses (1e6), Run uTime (s), Run sTime (s), Run Clock (s)"
	<< ", Memory (MB), Page fault, Input op" << std::endl;
    return;
  }

  CreateTable(asize);
  csv << tableName << ", " << tableSize/1e6 << ", " << usage.userTime()
      << ", " << usage.sysTime() << ", " << usage.elapsed();

  ExerciseTable(ntrials);
  csv << ", " << lastTrials/1e6 << ", " << usage.userTime() << ", "
      << usage.sysTime() << ", " << usage.elapsed()
      << ", " << usage.maxMemory()/1e6 << ", " << usage.pageFaults()
      << ", " << usage.ioInput()
      << std::endl;
}
