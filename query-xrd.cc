#include "XrootdSimple.hh"
#include <stdlib.h>
#include <iostream>


// Get command line arguments for array size (100M) and number of trials (1M)
void arrayArgs(int argc, char* argv[], objectId_t& asize, int& reps) {
  asize = 0;
  reps  = (argc>1) ? strtol(argv[2], 0, 0)   : 1000000;
}


// Main program goes here

int main(int argc, char* argv[]) {
  objectId_t arraySize;
  int queryTrials;
  arrayArgs(argc, argv, arraySize, queryTrials);

  std::cout << "XRootD Table query " << queryTrials
	    << " trials" << std::endl;

  XrootdSimple xrdidx(2);		// Verbosity
  xrdidx.ExerciseTable(queryTrials);
}
