#include "MysqlIndex.hh"
#include <stdlib.h>
#include <iostream>


// Get command line arguments for array size (100M) and number of trials (1M)
void arrayArgs(int argc, char* argv[], unsigned long long& asize, int& reps) {
  asize = (argc>1) ? strtoull(argv[1], 0, 0) : 100000000;
  reps  = (argc>2) ? strtol(argv[2], 0, 0)   : 1000000;
}


// Main program goes here

int main(int argc, char* argv[]) {
  unsigned long long arraySize;
  int queryTrials;
  arrayArgs(argc, argv, arraySize, queryTrials);

  std::cout << "MySQL (InnoDB) Table " << arraySize << " elements, "
	    << queryTrials << " trials" << std::endl;

  MysqlIndex theDB(2);		// Verbosity
  theDB.setTableSize(40e6);	// Use smaller blocks of data for efficiency

  theDB.CreateTable(arraySize);
  theDB.ExerciseTable(queryTrials);
}
