// $Id$
// MemCDIndex.hh -- Exercise performance of memcached interface as lookup table.
//
// 20151023  Michael Kelsey

#include "MemCDIndex.hh"
#include <libmemcached/memcached.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sstream>
#include <iostream>


// Constructor and destructor

MemCDIndex::MemCDIndex(int verbose)
  : IndexTester("memcached",verbose), mcdsv(0), memcd(0) {;}

MemCDIndex::~MemCDIndex() {
  if (mcdsv) killServer();
  if (memcd) killClient();
}

// Create new server as child process (any previous server is killed)

bool MemCDIndex::launchServer(unsigned long long asize) {
  if (mcdsv) killServer();
  
  long needmem = asize/1000000LL * 48 + 1;	// 48 bytes/entry, in MB

  mcdsv = fork();
  if (mcdsv < 0) {
    std::cerr << "Server creation failed!" << std::endl;
    return false;
  } else if (mcdsv == 0) {		// This is the child, start the server
    // Argument to "-m" is total size in megabytes
    char* args[] = { "memcached", "-m" , 0 };

    int slen = snprintf(0,0,"%ld",needmem);	// Length of string
    args[2] = (char*)malloc(slen+1);
    snprintf(args[2],slen+1,"%ld",needmem);

    if (verboseLevel>1) std::cout << "Launching server..." << std::endl;
    execvp(args[0], args);			// This should never return!
    ::exit(0);
  } else {
    if (verboseLevel>1) {
      std::cout << "Successfully created " << needmem << " MB server"
		<< " (PID " << mcdsv << ")" << std::endl;
    }
    return true;
  }
}


// Create client structure for sending/receiving data with server

bool MemCDIndex::launchClient(unsigned long long asize) {
  if (memcd) killClient();
  if (!mcdsv) {
    std::cerr << "No memcached server!" << std::endl;
    return false;
  }

  std::stringstream mcdstr;
  mcdstr << "--SERVER=localhost" << " --POOL-MAX=" << asize;
  const char* mcdconf = mcdstr.str().c_str();
  memcd = memcached(mcdconf, strlen(mcdconf));
  if (!memcd) {
    std::cerr << "Client creation failed!" << std::endl;
    return false;
  }
  
  if (verboseLevel>1) std::cout << "Successfully created client." << std::endl;
  return true;
}

void MemCDIndex::create(unsigned long long asize) {
  if (asize==0) return;		// Don't create a null object!
  
  launchServer(asize); if (!mcdsv) return;
  launchClient(asize); if (!memcd) return;
  
  static const int zero=0;		// All keys have same dummy value
  for (unsigned long long key=0; key<asize; key++) {
    memcached_set(memcd, (const char*)&key, sizeof(key), (const char*)&zero,
		  sizeof(int), 0, 0);
  }
}


// Query server to get requested index entry

int MemCDIndex::value(unsigned long long index) {
  if (!memcd) return 0xdeadbeef;	// Include sanity check

  size_t blen;
  uint32_t flags;
  memcached_return_t error;
  char* buf = memcached_get(memcd, (const char*)&index, sizeof(index),
			    &blen, &flags, &error);
  if (!buf) return 0xdeadbeef;
  
  int valbuf;			// To copy "byte array" into numeric value
  memcpy(&valbuf, buf, blen);
  delete buf;			// Avoid memory leaks
  
  return valbuf;
}


// Delete server and client for new iteration

void MemCDIndex::killServer() {
  if (!mcdsv) return;		// Avoid unnecessary work

  // FIXME:  Expand error handling to identify what happened and why
  if (kill(mcdsv,SIGKILL) < 0) perror("kill memcached");
  if (waitpid(mcdsv,0,0) < 0)  perror ("waitpid memcached");

  mcdsv = 0;
}

void MemCDIndex::killClient() {
  if (!memcd) return;		// Avoid unnecessary work

  memcached_free(memcd);
  memcd = 0;
}
