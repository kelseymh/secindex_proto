#define _FILE_OFFSET_BITS 64	/* Enables large-file support */
#define _LARGEFILE64_SOURCE

#include "FileIndex.hh"
#include <iostream>
#include <stdlib.h>


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

  std::cout << "Flat file " << arraySize << " elements, " << queryTrials
	    << " trials" << std::endl;

  FileIndex afile(1);		// Verbosity
  afile.CreateTable(arraySize);
  afile.ExerciseTable(queryTrials);
}
