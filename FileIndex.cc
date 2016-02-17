// $Id$
// FileIndex.hh -- Exercise performance of large flat file as lookup table.
//
// 20151023  Michael Kelsey

#define _FILE_OFFSET_BITS 64	/* Enables large-file support */
#define _LARGEFILE64_SOURCE

#include "FileIndex.hh"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

// NOTE:  MacOSX does not have "off64_t" type!  Why not?
#if __APPLE__ && __MACH__
typedef off_t off64_t;
#endif


// Create gigantic flat file full of index entries, then re-open for reading

void FileIndex::create(objectId_t asize) {
  static const int zero=0;		// Passed to fwrite as pointer

  FILE* outf = fopen(fname, "w");
  for (objectId_t i=0; i<asize; i++) {
    fwrite(&zero,sizeof(int),1,outf);
  }

  fclose(outf);		// Close and reopen for future access
  afile = fopen(fname, "r");
}
  

// Access requested array element with existence check

chunkId_t FileIndex::value(objectId_t index) {
  off64_t offset = (off64_t)(sizeof(chunkId_t)*index);
  if (0 != fseeko(afile, offset, SEEK_SET)) return 0xdeadbeef;
  
  static chunkId_t val;	   		// Avoid allocation overhead
  fread(&val, sizeof(chunkId_t), 1, afile);
  return val;
}


// Close and delete file from filesystem

void FileIndex::close() {
  if (afile) fclose(afile);
  unlink(fname);
}
