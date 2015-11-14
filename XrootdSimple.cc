// $Id$
// XrootdSimple.cc -- Exercise performance of XRootD providing flat files
//		      as blocked lookup tables, 100M entries per file.
//
// 20151103  Michael Kelsey
// 20151113  Fix server configs per Andy H., move all temp files into data dir
// 20151113  Instead of "localhost", use call to gethostname() to get string

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
    xrdServerPid(0), cmsdServerPid(0), xrdManagerPid(0), cmsdManagerPid(0) {
  const size_t namelen = sysconf(_SC_HOST_NAME_MAX)+1;
  char hostname[namelen];
  gethostname(hostname, namelen);
  localHostName = hostname;
}

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

  sleep(10);		// Wait for services to be ready for access
}

int XrootdSimple::value(unsigned long long index) { 
  if (verboseLevel) cout << "XrootdSimple::value " << index << endl;

  size_t ifile = index / entriesPerFile;
  size_t offset = index % entriesPerFile * sizeof(int);

  string xrdFile = dirName+"/"+getTempFilename(ifile);
  string xrdPath = localHostName+":10940/"+xrdFile;

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

  ofstream confFile(xrdConfigPath.c_str());
  if (!confFile) {
    cerr << "Failed to create " << xrdConfigPath << endl;
    return false;
  }

  confFile << "if named manager\n"
	   << "all.role manager\n"
	   << "xrd.port 10940 if exec xrootd\n"
	   << "else\n"
	   << "all.role server\n"
	   << "xrd.port any\n"
	   << "fi\n"
	   << "all.manager " << localHostName << ":10950\n"
	   << "all.adminpath " << dirName << "\n"
	   << "all.pidpath " << dirName << "\n"
	   << "all.export " << dirName << "\n"
	   << "cms.delay startup 5"
	   << endl;

  return true;
}

bool XrootdSimple::launchServer() {
  if (verboseLevel) cout << "XrootdSimple::launchServer" << endl;

  cmsdServerPid = launchService("cmsd", "server");
  xrdServerPid = launchService("xrootd", "server");

  return (xrdServerPid > 0 && cmsdServerPid > 0);
}

bool XrootdSimple::launchManager() {
  if (verboseLevel) cout << "XrootdSimple::launchManager" << endl;

  cmsdManagerPid = launchService("cmsd", "manager");
  xrdManagerPid = launchService("xrootd", "manager");

  return (xrdManagerPid > 0 && cmsdManagerPid > 0);
}

pid_t XrootdSimple::launchService(const char* svcExec, const char* svcName) {
  if (verboseLevel) 
    cout << "XrootdSimple::launchService " << svcExec << " " << svcName << endl;

  pid_t svcPid = fork();
  if (svcPid > 0) {
    if (verboseLevel>1) {
      cout << "Successfully forked child " << svcExec << " [" << svcName << "]"
	   << " (PID " << svcPid << ")" << endl;
    }
    return svcPid;
  } else if (svcPid < 0) {
    cerr << "fork ("  << svcExec << " [" << svcName << "] failed!" << endl;
    return svcPid;
  }
  
  string cfn = dirName + "/" + xrdConfigName;
  string log = dirName + "/" + svcExec + "-" + svcName + ".log";
  string env = dirName + "/" + svcExec + "." + svcName + ".env";

  if (verboseLevel>1) {
    cout << svcExec << " -n " << svcName << " -c " << cfn
	 << " -l " << log << " -d " << " -s " << env
	 << endl;
  }

  // Config file, log files, and internal files all go to data directory
  execlp(svcExec, svcExec, "-n", svcName, "-c", cfn.c_str(),
	 "-l", log.c_str(), "-d", "-s", env.c_str(), (const char*)0);
      
  cerr << "FATAL ERROR in XrootdSimple: ";
  perror("execlp");
  return -1;
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
