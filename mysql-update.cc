#include "MysqlUpdate.hh"
#include <stdlib.h>
#include <fstream>
#include <iostream>


// Main program goes here

int main(int argc, char* argv[]) {
  objectId_t arraySize;
  const char* updFile;

  // Get command line argument:  table size only, no query trials
  arraySize = (argc>1) ? strtoull(argv[1], 0, 0) : 100000000;

  std::cout << "MySQL (InnoDB) Table " << arraySize << " elements, "
	    << " bulk update (10%)" << std::endl;

  MysqlUpdate theDB(2);		// Verbosity
  theDB.SetIndexSpacing(10);	// Leave gaps for later update
  theDB.CreateTable(arraySize);
  theDB.UpdateTable();		// Use default name from constructor
}
