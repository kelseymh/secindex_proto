// $Id$
// MysqlUpdate.hh -- Exercise performance of MysqlDB with bulk updating
//
// 20160217  Michael Kelsey -- sets bulk data file automatically
// 20160224  Add parameter for size of bulk-update file

#include "MysqlUpdate.hh"
#include "UsageTimer.hh"
#include <algorithm>
#include <fstream>
#include <stdlib.h>
#include <unistd.h>


// Constructor and destructor

MysqlUpdate::MysqlUpdate(int verbose) :
  MysqlIndex(verbose), bulkfile("bulk.dat"), bulksize(1e7) {;}

MysqlUpdate::~MysqlUpdate() {
  unlink(bulkfile);
}


// Do base class database creation, then construct bulk-update file

void MysqlUpdate::create(objectId_t asize) {
  MysqlIndex::create(asize);

  // Create the 10M bulk-update file here, with interleaving
  unsigned shortStep = std::max(indexStep/3U, 1U);

  createLoadFile(bulkfile, bulksize, 0ULL, shortStep);
}


// Generate test and print comma-separated data; asize=0 for column headings
// NOTE:  THIS IS A HACK WITH COPY AND MODIFIED CODE FROM BASE

void MysqlUpdate::TestAndReport(objectId_t asize, long usize,
				std::ostream& csv) {
  if (asize == 0) {		// Special case: print column headings
    csv << "Type, Size (1e6), Init CPU (s), Init Clock (s)"
	<< ", Update (1e6), Update CPU (s), Update Clock (s)"
	<< ", Memory (MB), Page fault, Input op" << std::endl;
    return;
  }

  CreateTable(asize);
  csv << GetName() << ", " << tableSize/1e6 << ", "
      << GetUsage().cpuTime() << ", " << GetUsage().elapsed()
      << std::flush;

  setUpdateSize(usize);
  UpdateTable(bulkfile);
  csv << ", " << bulksize/1e6 << ", " << GetUsage().cpuTime()
      << ", " << GetUsage().elapsed() << ", " << GetUsage().maxMemory()/1e6
      << ", " << GetUsage().pageFaults() << ", " << GetUsage().ioInput()
      << std::endl;
}

