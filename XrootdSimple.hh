#ifndef XROOTD_SIMPLE_HH
#define XROOTD_SIMPLE_HH 1
// $Id$
// XrootdSimple.hh -- Exercise performance of XRootD providing flat files
//		      as blocked lookup tables.
//
// 20151103  Michael Kelsey
// 20151113  Add buffer for local hostname, used in configuring XRootD

#include "IndexTester.hh"
#include <sys/types.h>
#include <string>


class XrootdSimple : public IndexTester {
public:
  XrootdSimple(int verbose=0);
  virtual ~XrootdSimple();

protected:
  virtual void create(objectId_t asize);
  virtual chunkId_t value(objectId_t index);

  bool createTempFiles(size_t nfiles);
  bool writeXrdConfigFile();
  bool launchServer();
  bool launchManager();

  void deleteTempFiles();	// Do we want to do this, or keep them?
  void killServer();
  void killManager();

  bool createTempDir();		// Create temporary directory if necessary

  std::string getTempFilename(size_t ifile);	// Generate index filename
  bool createTempFile(size_t ifile);
  pid_t launchService(const char* svcExec, const char* svcName);
  void killService(pid_t& svcPid);		// Pass-by-value to set to zero

private:
  size_t entriesPerFile;	// Number of index entries per flat file
  std::string localHostName;	// Name of host, to pass to XRootD services
  std::string dirName;		// Directory path to hold flat files
  std::string xrdConfigName;	// Configuration file for XRootD services

  pid_t xrdServerPid;		// Process IDs of XRootD services
  pid_t cmsdServerPid;
  pid_t xrdManagerPid;
  pid_t cmsdManagerPid;
};

#endif	/* XROOTD_SIMPLE_HH */
