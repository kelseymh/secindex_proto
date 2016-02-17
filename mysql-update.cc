#include "MysqlIndex.hh"
#include <stdlib.h>
#include <fstream>
#include <iostream>


// Main program goes here

int main(int argc, char* argv[]) {
  objectId_t arraySize;
  const char* updFile;

  // Get command line arguments:  table size and external input file
  arraySize = (argc>1) ? strtoull(argv[1], 0, 0) : 100000000;
  updFile = (argc>2) ? argv[2] : (char*)0;

  std::cout << "MySQL (InnoDB) Table " << arraySize << " elements, "
	    << " bulk update (10%) from " << updFile << std::endl;

  // Create the bulk-update file here, using steps-of-three
  objectId_t bulksize = arraySize / 10;
  std::ofstream bulkdata(updFile, std::ios::trunc);
  for (objectId_t i=0; i<bulksize; i++) {
    bulkdata << i*3 << "\t" << 5 << std::endl;
  }
  bulkdata.close();

  // Get absolute path to bulk-update file, to be passed to mysqld
  char * updAbsoluteFile = realpath(updFile, NULL);

  MysqlIndex theDB(2);		// Verbosity
  theDB.setIndexStep(10);	// Leave gaps for later update
  theDB.CreateTable(arraySize);
  theDB.UpdateTable(updAbsoluteFile);

  delete updAbsoluteFile;	// Clean up memory before exit
}
