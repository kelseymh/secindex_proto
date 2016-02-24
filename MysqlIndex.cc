// $Id$
// MysqlIndex.hh -- Exercise performance of MySQL with InnoDB as lookup table.
//
// 20160119  Michael Kelsey
// 20160204  Extend to support blocking data into smaller tables
// 20160216  Add support for doing "bulk updates" from flat files
// 20160218  Use update() loading for initial table setup

#include "MysqlIndex.hh"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <mysql/mysql.h>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>
using namespace std;


MysqlIndex::MysqlIndex(int verbose)
  : IndexTester("mysql",verbose), mysqlDB(0), dbname("SecIdx"),
    table("chunks"), blockSize(0ULL) {;}

MysqlIndex::~MysqlIndex() {
  cleanup();
}


// Transmit query string to MySQL server, report error if any

void MysqlIndex::sendQuery(const string& query) const {
  if (verboseLevel>1) cout << "sending: " << query << endl;

  mysql_query(mysqlDB, query.c_str());
  reportError();
}


// Print MySQL error message if any in present

void MysqlIndex::reportError() const {
  if (mysql_error(mysqlDB)[0] == '\0') return;	// No current error message

  cerr << mysql_error(mysqlDB) << endl;
}


// Return total number of tables, including partial

int MysqlIndex::numberOfTables() const {
  if (!usingMultipleTables()) return 1;
  
  int nTables = tableSize / blockSize;
  if (nTables*blockSize < tableSize) nTables++;		// Partial at end
  return nTables;
}


// Map input object ID to lookup table block

int MysqlIndex::chooseTable(objectId_t objID) const {
  if (!usingMultipleTables()) return -1;	// Single table for all data

  // If there are no table ranges, assume sequential indices for test
  if (blockStart.empty()) return (objID/indexStep/blockSize);

  // Get table which should contain specified objectID
  vector<objectId_t>::const_iterator lb =
    lower_bound(blockStart.begin(), blockStart.end(), objID);

  int tblidx = lb - blockStart.begin();
  if (objID < *lb) tblidx--;			// Lower bound returns ceiling

  return tblidx;
}


// Construct unique table names for each data block

string MysqlIndex::makeTableName(int tblidx) const {
  if (tblidx < 0 || !usingMultipleTables()) return table;

  stringstream suffixed;
  suffixed << table << tblidx;

  return suffixed.str();
}



// Create and populate secondary index from scratch

void MysqlIndex::create(objectId_t asize) {
  if (!connect(dbname) || !mysqlDB) return;	// Avoid unnecessary work

  tableSize = asize;		// Store for use in filling and querying

  accessDatabase();
  createTables();
  fillTableRanges();
}

chunkId_t MysqlIndex::value(objectId_t objID) {
  MYSQL_RES* result = findObjectID(objID);	// NULL handled automatically
  chunkId_t chunk = extractChunk(result);
  mysql_free_result(result);			// Clean up before exiting

  return chunk;
}


// Configure MySQL database and tables for use

bool MysqlIndex::connect(const string& newDBname) {
  if (verboseLevel) {
    cout << "MysqlIndex::connect " << (newDBname.empty()?dbname:newDBname)
	      << endl;
  }

  if (mysqlDB) cleanup();			// Get rid of previous version

  if (!newDBname.empty()) dbname = newDBname;	// Replace database name in use

  // connect to mysql server, no particular database
  mysqlDB = mysql_init(NULL);

  if (!mysql_real_connect(mysqlDB, "127.0.0.1", "root", "changeme",
			  "", 13306, NULL, 0)) {
    cerr << mysql_error(mysqlDB) << endl;
    mysql_close(mysqlDB);
    mysqlDB = 0;
    return false;
  }

  return true;
}

void MysqlIndex::accessDatabase() const {
  if (!mysqlDB) return;				// Avoid unnecessary work

  if (verboseLevel) cout << "MysqlIndex::accessDatabase" << endl;

  sendQuery("CREATE DATABASE "+dbname);
  sendQuery("USE "+dbname);
}

void MysqlIndex::createTables() const {
  if (!mysqlDB) return;				// Avoid unnecessary work

  if (verboseLevel) {
    cout << "MysqlIndex::createTables (up to " << blockSize << " entries)"
	      << endl;
  }

  if (usingMultipleTables()) {			// One table for each block
    int nTables = numberOfTables();
    if (verboseLevel>1) {
      cout << " creating " << nTables << " tables with "
		<< blockSize << " entries" << endl;
    }

    objectId_t tsize = blockSize;		// Last table may be short
    for (int itbl=0; itbl<nTables; itbl++) {
      if (itbl == nTables-1) tsize = tableSize - (nTables-1)*blockSize;
      createTable(itbl);
      fillTable(itbl, tsize, itbl*blockSize*indexStep);
    }
  } else {
    createTable(-1);				// One massive table
    fillTable(-1, tableSize, 0);
  }
}

void MysqlIndex::createTable(int tblidx) const {
  if (!mysqlDB) return;				// Avoid unnecessary work

  if (verboseLevel) cout << "MysqlIndex::createTable " << tblidx << endl;

  string maketbl = "CREATE TABLE " + makeTableName(tblidx)
    + " (objectId BIGINT NOT NULL PRIMARY KEY, chunkId INT NOT NULL)"
    + " ENGINE='InnoDB'";
  sendQuery(maketbl);
}

void MysqlIndex::fillTable(int tblidx, objectId_t tsize,
			   objectId_t firstID) const {
  if (!mysqlDB) return;				// Avoid unnecessary work

  if (verboseLevel) {
    cout << "MysqlIndex::fillTable " << tblidx << " " << tsize
	      << " " << firstID << endl;
  }

  // Load table using a flat data file rather than "exponential select"
  string loadfile = makeTableName(tblidx)+".dat";

  createLoadFile(loadfile.c_str(), tsize, firstID, indexStep);
  updateTable(loadfile.c_str(), tblidx);
  unlink(loadfile.c_str());		// Delete input file when done
}


// Set up local cache of object ID ranges in each table block

void MysqlIndex::fillTableRanges() {
  blockStart.clear();			// Discard existing data first!

  if (!mysqlDB || !usingMultipleTables()) return;  // Avoid unnecessary work

  if (verboseLevel) cout << "MysqlIndex::fillTableRanges" << endl;

  objectId_t minID=0ULL, maxID=0ULL;	// MaxID is ignored below
  int nTables = numberOfTables();
  for (int itbl=0; itbl<nTables; itbl++) {
    getObjectRange(itbl, minID, maxID);
    blockStart.push_back(minID);	// Only need to store start of range
  }

  // Report final tabulation for diagnostics
  if (verboseLevel>1) {
    cout << nTables << " tables with starting indices";
    for (size_t i=0; i<numberOfTables(); i++) {
      if (i%8 == 0) cout << endl;
      cout << " " << blockStart[i];
    }
    cout << endl;
  }
}


// Query specified table to get minimum and maximum keys

void MysqlIndex::getObjectRange(int tblidx, objectId_t &minID,
				objectId_t &maxID) const {
  if (!mysqlDB) { 				// Avoid unnecessary work
    minID = maxID = 0;
    return;
  }

  if (verboseLevel) 
    cout << "MysqlIndex::getTableRange " << tblidx << endl;

  sendQuery("SELECT MIN(objectId), MAX(objectId) FROM "+makeTableName(tblidx));
  MYSQL_RES *result = getQueryResult();
  if (result) {
    MYSQL_ROW row = mysql_fetch_row(result);	// There should be only one!
    if (row) {
      minID = strtoull(row[0], 0, 0);
      maxID = strtoull(row[1], 0, 0);
      if (verboseLevel>1)
	cout << " got range " << minID << " " << maxID << endl;
    } else {
      minID = maxID = 0;
    }
  }

  mysql_free_result(result);  
}


// Insert contents of external file in one action

void MysqlIndex::update(const char* datafile) {
  if (!datafile) return;		// No external file specified

  if (!usingMultipleTables()) {		// Single table requires no splitting
    updateTable(datafile, -1);
    return;
  }

  // Scan input file and create temp file for single table
  char* realdata = realpath(datafile,0);	// Returns new pointer!
  if (!realdata) {
    cerr << "MysqlIndex::update " << datafile << " not found" << endl;
    return;
  }

  string bulkline;		// Buffers for successive lines and values
  objectId_t objID;
  int tblidx = -1;		// Current table index; new file on change

  ofstream splitfile;		// Stream to produce splits in sequence
  string splitname;

  ifstream bulkfile(realdata);
  while (bulkfile.good() && !bulkfile.eof()) {
    getline(bulkfile, bulkline);		// Read a line from the file
    if (bulkline.empty()) continue;		// Skip blank lines

    objID = strtoull(bulkline.c_str(), 0, 0);

    if (tblidx != chooseTable(objID)) {		// Finished split, ship it
      if (tblidx >= 0) {
	if (verboseLevel>1)
	  cout << " finished splitting table " << tblidx << endl;
	
	splitfile.close();
	updateTable(splitname.c_str(), tblidx);
	unlink(splitname.c_str());
      }

      tblidx = chooseTable(objID);		// Start new file here
      if (verboseLevel>1)
	cout << " starting new split for table " << tblidx << endl;

      splitname = makeTableName(tblidx)+".split";
      splitfile.open(splitname, ios::trunc);
    }

    if (verboseLevel>2) cout << " writing '" << bulkline << "'" << endl;

    splitfile << bulkline << endl;		// Write line to split file
  }

  if (tblidx >= 0) {			// Wrap up final split here
    if (verboseLevel>1)	cout << " finished splitting table " << tblidx << endl;

    splitfile.close();
    updateTable(splitname.c_str(), tblidx);
    unlink(splitname.c_str());
  }

  bulkfile.close();
  delete realdata;				// Clean up output of realpath()
}


// Insert contents of external file into specified table block

void MysqlIndex::updateTable(const char* datafile, int tblidx) const {
  if (!datafile) return;		// No external file specified

  if (verboseLevel) {
    cout << "MysqlIndex::updateTable " << datafile
	      << " to table " << tblidx << endl;
  }

  char* realdata = realpath(datafile,0);	// Returns new pointer!
  if (!realdata) {
    cerr << "MysqlIndex::update " << datafile << " not found" << endl;
    return;
  }

  stringstream loadIt;
  loadIt << "LOAD DATA INFILE '" << realdata << "' REPLACE INTO TABLE "
	 << makeTableName(tblidx) << " FIELDS TERMINATED BY '\\t'";
  sendQuery(loadIt.str());

  delete realdata;				// Clean up output of realpath()
}


// Do MySQL query to extract chunk for given object from correct table

MYSQL_RES* MysqlIndex::findObjectID(objectId_t objID) const {
  if (!mysqlDB) return (MYSQL_RES*)0;		// Avoid unnecessary work

  stringstream lookup;
  lookup << "SELECT chunkId FROM " << makeTableName(chooseTable(objID))
	 << " WHERE objectId=" << objID;

  sendQuery(lookup.str());
  return getQueryResult();
}

MYSQL_RES* MysqlIndex::getQueryResult() const {
  MYSQL_RES *result = mysql_store_result(mysqlDB);

  if (result && mysql_num_rows(result)>0) {
    if (verboseLevel>1)
      cout << "got " << mysql_num_rows(result) << " rows, with "
		<< mysql_num_fields(result) << " columns" << endl;
  } else {
    reportError();
    mysql_free_result(result);		// Discard invalid result
    result = 0;    
  }

  return result;			// Transfers ownership to client
}


// Parse query result to get object or chunk ID (there should be only one!)

chunkId_t MysqlIndex::extractChunk(MYSQL_RES* result, size_t irow) const {
  if (!result) return 0xdeadbeef;		// Avoid unnecessary work

  MYSQL_ROW row = mysql_fetch_row(result);	// There should be only one!
  if (!row) return 0xdeadbeef;

  if (verboseLevel>1)
    cout << "result " << irow << ": " << row[irow] << endl;

  return strtoul(row[irow], 0, 0);	// Everything comes back as strings
}

objectId_t MysqlIndex::extractObject(MYSQL_RES* result, size_t irow) const {
  if (!result) return 0;			// Avoid unnecessary work

  MYSQL_ROW row = mysql_fetch_row(result);	// There should be only one!
  if (!row) return 0;

  if (verboseLevel>1)
    cout << "result " << irow << ": " << row[irow] << endl;

  return strtoull(row[irow], 0, 0);	// Everything comes back as strings
}


// Discard tables and database at end of job

void MysqlIndex::cleanup() {
  if (!mysqlDB) return;				// Avoid unnecessary work

  if (verboseLevel) cout << "MysqlIndex::cleanup" << endl;

  // FIXME:  Why did this go into an infinite loop when empty()?
  blockStart.clear();		// Discard index ranges for block tables


  if (usingMultipleTables()) {
    for (int itbl=0; itbl<numberOfTables(); itbl++) {
      dropTable(itbl);
    }
  } else {
    dropTable(-1);
  }

  sendQuery("DROP DATABASE "+dbname);

  mysql_close(mysqlDB);
  mysqlDB = 0;
}

void MysqlIndex::dropTable(int tblidx) const {
  if (!mysqlDB) return;				// Avoid unnecessary work
  
  if (verboseLevel>1)
    cout << "MysqlIndex::dropTable " << tblidx << endl;

  sendQuery("DROP TABLE "+makeTableName(tblidx));
}


// Construct a flat file to use for loading into table

void MysqlIndex::createLoadFile(const char* datafile, objectId_t fsize,
				objectId_t start, unsigned step) const {
  if (!datafile) return;			// No filename, no file

  if (verboseLevel) {
    cout << "MysqlIndex::createLoadFile " << datafile << " " << fsize
	      << " entries from " << start << " by " << step << endl;
  }

  ofstream bulkdata(datafile, ios::trunc);
  for (objectId_t i=0; i<fsize; i++) {
    bulkdata << start+i*step << "\t" << 5 << endl;
  }

  bulkdata.close();
}
