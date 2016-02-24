#include "MysqlUpdate.hh"
#include <stdlib.h>
#include <fstream>
#include <iostream>


// Get command line arguments for array size (100M) and bulk-update size (10M)
void arrayArgs(int argc, char* argv[], objectId_t& asize, objectId_t& usize) {
  asize = (argc>1) ? strtoull(argv[1], 0, 0) : 100000000;
  usize = (argc>2) ? strtoull(argv[2], 0, 0) : 10000000;
}


// Main program goes here

int main(int argc, char* argv[]) {
  objectId_t arraySize;
  objectId_t updateSize;
  arrayArgs(argc, argv, arraySize, updateSize);

  std::cout << "MySQL (InnoDB) Table " << arraySize << " elements, "
	    << " bulk update " << updateSize << " elements" << std::endl;

  MysqlUpdate theDB(3);			// Verbosity
  theDB.SetIndexSpacing(10);		// Leave gaps for later update
  theDB.setTableSize(arraySize/10);	// Very small tables to test splitting
  theDB.setUpdateSize(updateSize);

  theDB.CreateTable(arraySize);
  theDB.UpdateTable();		// Use default name from constructor
}
