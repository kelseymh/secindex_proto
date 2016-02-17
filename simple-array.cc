#include "ArrayIndex.hh"
#include <stdlib.h>
#include <iostream>


// Get command line arguments for array size (100M) and number of trials (1M)
void arrayArgs(int argc, char* argv[], objectId_t& asize, int& reps) {
  asize = (argc>1) ? strtoull(argv[1], 0, 0) : 100000000;
  reps  = (argc>2) ? strtol(argv[2], 0, 0)   : 1000000;
}


// Main program goes here

int main(int argc, char* argv[]) {
  objectId_t arraySize;
  int queryTrials;
  arrayArgs(argc, argv, arraySize, queryTrials);

  std::cout << "Simple array " << arraySize << " elements, " << queryTrials
	    << " trials" << std::endl;

  ArrayIndex array(1);		// Verbosity
  array.CreateTable(arraySize);
  array.ExerciseTable(queryTrials);
}
