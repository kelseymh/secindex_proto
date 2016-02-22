// $Id$
// MysqlUpdate.hh -- Exercise performance of MysqlDB with bulk updating
//
// 20160217  Michael Kelsey -- sets bulk data file automatically

#include "MysqlUpdate.hh"
#include "UsageTimer.hh"
#include <algorithm>
#include <fstream>
#include <stdlib.h>
#include <unistd.h>


// Constructor and destructor

MysqlUpdate::MysqlUpdate(int verbose) :
  MysqlIndex(verbose), bulkfile("bulk.dat") {;}

MysqlUpdate::~MysqlUpdate() {
  unlink(bulkfile);
}


// Do base class database creation, then construct bulk-update file

void MysqlUpdate::create(objectId_t asize) {
  MysqlIndex::create(asize);

  // Create the 10% bulk-update file here, with interleaving
  unsigned shortStep = std::max(indexStep/3U, 1U);

  createLoadFile(bulkfile, 1e7, 0ULL, shortStep);
}


// Generate test and print comma-separated data; asize=0 for column headings
// NOTE:  THIS IS A HACK WITH COPY AND MODIFIED CODE FROM BASE

void MysqlUpdate::TestAndReport(objectId_t asize, long /*ntrials*/,
				std::ostream& csv) {
  if (asize == 0) {		// Special case: print column headings
    csv << "Type, Size (1e6), Init CPU (s), Init Clock (s)"
	<< ", Update (1e6), Run CPU (s), Run Clock (s)"
	<< ", Memory (MB), Page fault, Input op" << std::endl;
    return;
  }

  CreateTable(asize);
  csv << GetName() << ", " << tableSize/1e6 << ", "
      << GetUsage().cpuTime() << ", " << GetUsage().elapsed()
      << std::flush;

  UpdateTable(bulkfile);	// Fixed 10M entry bulk-update file
  csv << ", " << 10 << ", " << GetUsage().cpuTime()
      << ", " << GetUsage().elapsed() << ", " << GetUsage().maxMemory()/1e6
      << ", " << GetUsage().pageFaults() << ", " << GetUsage().ioInput()
      << std::endl;
}

