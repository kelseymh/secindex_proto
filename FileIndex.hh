#ifndef FILE_INDEX_HH
#define FILE_INDEX_HH 1
// $Id$
// FileIndex.hh -- Exercise performance of large flat file as lookup table.
//
// 20151023  Michael Kelsey
// 20160224  Move destructor action to cleanup() function

#include "IndexTester.hh"

class FileIndex : public IndexTester {
public:
  FileIndex(int verbose=0) : IndexTester("file",verbose),
			     fname("/tmp/index-file.dat"), afile(0) {;}
  virtual ~FileIndex() { cleanup(); }

protected:
  virtual void create(objectId_t asize);
  virtual chunkId_t value(objectId_t index);
  virtual void cleanup();

private:
  const char* fname;
  FILE* afile;
};

#endif	/* FILE_INDEX_HH */
