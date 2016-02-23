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


MysqlIndex::MysqlIndex(int verbose)
  : IndexTester("mysql",verbose), mysqlDB(0), dbname("SecIdx"),
    table("chunks"), blockSize(0ULL) {;}

MysqlIndex::~MysqlIndex() {
  cleanup();
}


// Transmit query string to MySQL server, report error if any

void MysqlIndex::sendQuery(const std::string& query) const {
  if (verboseLevel>1) std::cout << "sending: " << query << std::endl;

  mysql_query(mysqlDB, query.c_str());
  reportError();
}


// Print MySQL error message if any in present

void MysqlIndex::reportError() const {
  if (mysql_error(mysqlDB)[0] == '\0') return;	// No current error message

  std::cerr << mysql_error(mysqlDB) << std::endl;
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
  std::vector<objectId_t>::const_iterator lb =
    std::lower_bound(blockStart.begin(), blockStart.end(), objID);

  return (lb - blockStart.begin());
}


// Construct unique table names for each data block

std::string MysqlIndex::makeTableName(int tblidx) const {
  if (tblidx < 0 || !usingMultipleTables()) return table;

  std::stringstream suffixed;
  suffixed << table << tblidx;

  return suffixed.str();
}



// Create and populate secondary index from scratch

void MysqlIndex::create(objectId_t asize) {
  if (!connect(dbname) || !mysqlDB) return;	// Avoid unnecessary work

  tableSize = asize;		// Store for use in filling and querying

  blockStart.clear();
  accessDatabase();
  createTables();
}

chunkId_t MysqlIndex::value(objectId_t objID) {
  MYSQL_RES* result = findObjectID(objID);	// NULL handled automatically
  chunkId_t chunk = extractChunk(result);
  mysql_free_result(result);			// Clean up before exiting

  return chunk;
}


// Configure MySQL database and tables for use

bool MysqlIndex::connect(const std::string& newDBname) {
  if (verboseLevel) {
    std::cout << "MysqlIndex::connect " << (newDBname.empty()?dbname:newDBname)
	      << std::endl;
  }

  if (mysqlDB) cleanup();			// Get rid of previous version

  if (!newDBname.empty()) dbname = newDBname;	// Replace database name in use

  // connect to mysql server, no particular database
  mysqlDB = mysql_init(NULL);

  if (!mysql_real_connect(mysqlDB, "127.0.0.1", "root", "changeme",
			  "", 13306, NULL, 0)) {
    std::cerr << mysql_error(mysqlDB) << std::endl;
    mysql_close(mysqlDB);
    mysqlDB = 0;
    return false;
  }

  return true;
}

void MysqlIndex::accessDatabase() const {
  if (!mysqlDB) return;				// Avoid unnecessary work

  if (verboseLevel) std::cout << "MysqlIndex::accessDatabase" << std::endl;

  sendQuery("CREATE DATABASE "+dbname);
  sendQuery("USE "+dbname);
}

void MysqlIndex::createTables() {
  if (!mysqlDB) return;				// Avoid unnecessary work

  if (verboseLevel) {
    std::cout << "MysqlIndex::createTables (up to " << blockSize << " entries)"
	      << std::endl;
  }

  if (usingMultipleTables()) {			// One table for each block
    int nTables = numberOfTables();
    if (verboseLevel>1) {
      std::cout << " creating " << nTables << " tables with "
		<< blockSize << " entries" << std::endl;
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

  if (verboseLevel>1) {
    std::cout << numberOfTables() << " tables created";
    if (usingMultipleTables()) {
      std::cout << " with starting indices";
      for (size_t i=0; i<numberOfTables(); i++) {
	if (i%8 == 0) std::cout << std::endl;
	std::cout << " " << blockStart[i];
      }
    }
    std::cout << std::endl;
  }
}

void MysqlIndex::createTable(int tblidx) const {
  if (!mysqlDB) return;				// Avoid unnecessary work

  if (verboseLevel)
    std::cout << "MysqlIndex::createTable " << tblidx << std::endl;

  std::string maketbl = "CREATE TABLE " + makeTableName(tblidx)
    + " (objectId BIGINT NOT NULL PRIMARY KEY, chunkId INT NOT NULL)"
    + " ENGINE='InnoDB'";
  sendQuery(maketbl);
}

void MysqlIndex::fillTable(int tblidx, objectId_t tsize,
			   objectId_t firstID) {
  if (!mysqlDB) return;				// Avoid unnecessary work

  if (verboseLevel) {
    std::cout << "MysqlIndex::fillTable " << tblidx << " " << tsize
	      << " " << firstID << std::endl;
  }

  // Load table using a flat data file rather than "exponential select"
  std::string loadfile = makeTableName(tblidx)+".dat";

  createLoadFile(loadfile.c_str(), tsize, firstID, indexStep);
  updateTable(loadfile.c_str(), tblidx);
  unlink(loadfile.c_str());		// Delete input file when done

  // Get table range and add to range list
  if (tblidx < 0) return;		// Single table doesn't use range

  objectId_t minID=0ULL, maxID=0ULL;	// MaxID is ignored below
  getObjectRange(tblidx, minID, maxID);

  // Extend list of table ranges for later lookup
  size_t absidx = (size_t)tblidx;		// Casting for convenience below
  if (absidx < blockStart.size()) {		// Replace previous range
    blockStart[absidx] = minID;
  } else if (absidx == blockStart.size()) {	// Next in sequence
    blockStart.push_back(minID);
  } else {				// Fill up to this table block index
    for (size_t i=blockStart.size(); i<absidx; i++) {
      blockStart.push_back(minID);
    }
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
    std::cout << "MysqlIndex::getTableRange " << tblidx << std::endl;

  sendQuery("SELECT MIN(objectId), MAX(objectId) FROM "+makeTableName(tblidx));
  MYSQL_RES *result = getQueryResult();
  minID = extractObject(result,0);
  maxID = extractObject(result,1);
  mysql_free_result(result);  
}


// Insert contents of external file in one action

void MysqlIndex::update(const char* datafile) {
  if (!usingMultipleTables()) {		// Single table requires no splitting
    updateTable(datafile, -1);
    return;
  }

  // FIXME:  Need to scan input and split at table boundaries
  updateTable(datafile, 0);
}

// Insert contents of external file into specified table block

void MysqlIndex::updateTable(const char* datafile, int tblidx) {
  if (!datafile) return;		// No external file specified

  if (verboseLevel) {
    std::cout << "MysqlIndex::updateTable " << datafile
	      << " to table " << tblidx << std::endl;
  }

  char* realdata = realpath(datafile,0);	// Returns new pointer!
  if (!realdata) {
    std::cerr << "MysqlIndex::update " << datafile << " not found" << std::endl;
    return;
  }

  std::stringstream loadIt;
  loadIt << "LOAD DATA INFILE '" << realdata << "' REPLACE INTO TABLE "
	 << makeTableName(tblidx) << " FIELDS TERMINATED BY '\\t'";
  sendQuery(loadIt.str());

  delete realdata;				// Clean up output of realpath()
}


// Do MySQL query to extract chunk for given object from correct table

MYSQL_RES* MysqlIndex::findObjectID(objectId_t objID) const {
  if (!mysqlDB) return (MYSQL_RES*)0;		// Avoid unnecessary work

  std::stringstream lookup;
  lookup << "SELECT chunkId FROM " << makeTableName(chooseTable(objID))
	 << " WHERE objectId=" << objID;

  sendQuery(lookup.str());
  return getQueryResult();
}

MYSQL_RES* MysqlIndex::getQueryResult() const {
  MYSQL_RES *result = mysql_store_result(mysqlDB);

  if (result && mysql_num_rows(result)>0) {
    if (verboseLevel>1)
      std::cout << "got " << mysql_num_rows(result) << " rows, with "
		<< mysql_num_fields(result) << " columns" << std::endl;
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
    std::cout << "result " << irow << ": " << row[irow] << std::endl;

  return strtoul(row[irow], 0, 0);	// Everything comes back as strings
}

objectId_t MysqlIndex::extractObject(MYSQL_RES* result, size_t irow) const {
  if (!result) return 0;			// Avoid unnecessary work

  MYSQL_ROW row = mysql_fetch_row(result);	// There should be only one!
  if (!row) return 0;

  if (verboseLevel>1)
    std::cout << "result " << irow << ": " << row[irow] << std::endl;

  return strtoull(row[irow], 0, 0);	// Everything comes back as strings
}


// Discard tables and database at end of job

void MysqlIndex::cleanup() {
  if (!mysqlDB) return;				// Avoid unnecessary work

  if (verboseLevel) std::cout << "MysqlIndex::cleanup" << std::endl;

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
    std::cout << "MysqlIndex::dropTable " << tblidx << std::endl;

  sendQuery("DROP TABLE "+makeTableName(tblidx));
}


// Construct a flat file to use for loading into table

void MysqlIndex::createLoadFile(const char* datafile, objectId_t fsize,
				objectId_t start, unsigned step) const {
  if (!datafile) return;			// No filename, no file

  if (verboseLevel) {
    std::cout << "MysqlIndex::createLoadFile " << datafile << " " << fsize
	      << " entries from " << start << " by " << step << std::endl;
  }

  std::ofstream bulkdata(datafile, std::ios::trunc);
  for (objectId_t i=0; i<fsize; i++) {
    bulkdata << start+i*step << "\t" << 5 << std::endl;
  }

  bulkdata.close();
}
