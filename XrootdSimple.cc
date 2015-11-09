// $Id$
// XrootdSimple.cc -- Exercise performance of XRootD providing flat files
//		      as blocked lookup tables, 100M entries per file.
//
// 20151103  Michael Kelsey

#include "XrootdSimple.hh"
#include "XrdCl/XrdClFile.hh"
#include "XrdCl/XrdClXRootDResponses.hh"
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <string>
using namespace std;


// Constructor and destructor

XrootdSimple::XrootdSimple(int verbose)
  : IndexTester("xrootd", verbose), entriesPerFile(10000000),
    dirName("/tmp/xrootd_simple"), xrdConfigName("xrootd.conf"),
    xrdServerPid(0), cmsdServerPid(0), xrdManagerPid(0), cmsdManagerPid(0) {;}

XrootdSimple::~XrootdSimple() {
  killServer();
  killManager();
  //*** deleteTempFiles();	// Do we really want to do this?
}


// Interface to base class for constructing and accessing servers

void XrootdSimple::create(unsigned long long asize) {
  if (verboseLevel) cout << "XrootdSimple::create " << asize << endl;

  if (!writeXrdConfigFile()) {
    cerr << "Unable to create XRootD configuration file!" << endl;
    ::exit(1);
  }

  if (!createTempFiles(asize/entriesPerFile+1)) {
    cerr << "Index file creation failed!" << endl;
    ::exit(1);
  }

  if (!(launchServer() && launchManager())) {
    cerr << "Unable to launch XRootD services!" << endl;
    ::exit(1);
  }
}

int XrootdSimple::value(unsigned long long index) { 
  if (verboseLevel) cout << "XrootdSimple::value " << index << endl;

  size_t ifile = index / entriesPerFile;
  size_t offset = index % entriesPerFile * sizeof(int);

  string xrdFile = dirName+"/"+getTempFilename(ifile);
  string xrdPath = "localhost:1094/"+xrdFile;

  if (verboseLevel>1) cout << "... accessing " << xrdPath << endl;

  XrdCl::File blockFile;
  if (!blockFile.Open(xrdPath,XrdCl::OpenFlags::Read).IsOK()) {
    cerr << "Unable to access " << xrdPath << endl;
    return -1;
  }

  if (verboseLevel>1) cout << "... reading at offset " << offset << endl;

  int readValue = 0;			// Input buffer from XRD
  uint32_t readLen = 0;
  if (!blockFile.Read(offset, sizeof(int), &readValue, readLen).IsOK()) {
    cerr << "Unable to read " << xrdPath << " at offset " << offset << endl;
    return -1;
  }

  if (verboseLevel>1) cout << "... closing file" << endl;

  if (!blockFile.Close().IsOK()) {
    cerr << "Unable to properly close " << xrdPath << endl;
  }

  return readValue;
}


// Create flat files locally (before XRootD starts) for lookup tables

bool XrootdSimple::createTempFiles(size_t nfiles) {
  if (verboseLevel) {
    cout << "XrootdSimple::createTempFiles " << nfiles << " in " << dirName
	 << endl;
  }

  for (size_t ifile=0; ifile<nfiles; ifile++) {
    if (!createTempFile(ifile)) return false;	// Abandon if file fails
  }

  return true;
}

bool XrootdSimple::createTempFile(size_t ifile) {
  string fname = dirName+"/"+getTempFilename(ifile);

  // FIXME:  We should be creating actual lookup files!
  static const int zero=0;		// Passed to fwrite as pointer

  FILE* outf = fopen(fname.c_str(), "w");
  for (size_t i=0; i<entriesPerFile; i++) {
    fwrite(&zero,sizeof(int),1,outf);
  }

  fclose(outf);		// Close and reopen for future access

  return true;
}


// Create configuration file used to launch XRootD service processes

bool XrootdSimple::writeXrdConfigFile() {
  string xrdConfigPath = dirName+"/"+xrdConfigName;

  if (verboseLevel)
    cout << "XrootdSimple::writeXrdConfigFile " << xrdConfigPath << endl;

  if (!createTempDir()) return false;

  ofstream confFile(xrdConfigPath);
  if (!confFile) {
    cerr << "Failed to create " << xrdConfigPath << endl;
    return false;
  }

  confFile << "all.role server\n"
	   << "all.manager localhost:10940\n"
	   << "all.export " << dirName << "\n"
	   << "cms.delay startup 5\n\n"
	   << "all.role manager\n"
	   << "all.manager localhost:1094\n"
	   << "all.export " << dirName << "\n"
	   << "cms.delay startup 5\n"
	   << endl;

  return true;
}

bool XrootdSimple::launchServer() {
  if (verboseLevel) cout << "XrootdSimple::launchServer" << endl;

  xrdServerPid = launchService("xrootd", "server");
  cmsdServerPid = launchService("cmsd", "server");
  sleep(10);

  return (xrdServerPid > 0 && cmsdServerPid > 0);
}

bool XrootdSimple::launchManager() {
  if (verboseLevel) cout << "XrootdSimple::launchManager" << endl;

  xrdManagerPid = launchService("xrootd", "manager");
  cmsdManagerPid = launchService("cmsd", "manager");
  sleep(10);

  return (xrdManagerPid > 0 && cmsdManagerPid > 0);
}

pid_t XrootdSimple::launchService(const char* svcExec, const char* svcName) {
  if (verboseLevel>1) 
    cout << "XrootdSimple::launchService " << svcExec << " " << svcName << endl;

  pid_t svcPid = fork();
  if (svcPid > 0) {
    cout << "Successfully forked child " << svcExec << " [" << svcName << "]"
	      << " (PID " << svcPid << ")" << endl;
  } else if (svcPid < 0) {
    cerr << "Service creation ("  << svcExec << " [" << svcName
	      << "] failed!" << endl;
  } else if (svcPid == 0) {		// This is the child, start the service
    if (verboseLevel>1) {
      cout << "Child launching " << svcExec << " [" << svcName << "] ..."
		<< endl;
    }

    string log = dirName + "/" + svcExec + "-" + svcName + ".log";
    execl(svcExec, svcExec, "-n", svcName, "-l", log.c_str(), "-d",
	  dirName.c_str(), 0);
    ::exit(0);
  }

  return svcPid;
}


// Kill temporary XRootD services

void XrootdSimple::killServer() {
  killService(xrdServerPid);
  killService(cmsdServerPid);
}

void XrootdSimple::killManager() {
  killService(xrdManagerPid);
  killService(cmsdManagerPid);
}

void XrootdSimple::killService(pid_t& svcPid) {
  if (svcPid <= 0) return;		// Avoid unnecessary work

  // FIXME:  Expand error handling to identify what happened and why
  if (kill(svcPid,SIGKILL) < 0) perror("kill");
  if (waitpid(svcPid,0,0) < 0)  perror ("waitpidd");
  svcPid = 0;
}


// Remove flat files fom local file system (must do after killing services!)

void XrootdSimple::deleteTempFiles() {;}


// Create temporary directory if necessary

bool XrootdSimple::createTempDir() {
  if (verboseLevel>1) cout << "XrootdSimple::createTempDir " << dirName << endl;

  // Create directory, returns EEXIST if already present
  if (mkdir(dirName.c_str(), 0755)==0 || errno==EEXIST) return true;

  cerr << "Error creating temp directory: ";
  perror("mkdir");
  return false;
}


// Generate full filename for given file (block) index
// Format is <temp-directory>/objectChunk_NNNNN.idx

string XrootdSimple::getTempFilename(size_t ifile) {
  char fname[22];
  snprintf(fname, sizeof(fname), "objectChunk_%05lu.idx", ifile);

  return string(fname);
}
