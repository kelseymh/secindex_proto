// $Id$
// MysqlIndex.hh -- Exercise performance of MySQL with InnoDB as lookup table.
//
// 20160119  Michael Kelsey
// 20160204  Extend to support blocking data into smaller tables
// 20160216  Add support for doing "bulk updates" from flat files

#include "MysqlIndex.hh"
#include <mysql/mysql.h>
#include <iostream>
#include <cmath>
#include <string>
#include <sstream>


MysqlIndex::MysqlIndex(int verbose)
  : IndexTester("mysql",verbose), mysqlDB(0), dbname("SecIdx"),
    table("chunks"), blockSize(0ULL) {;}

MysqlIndex::~MysqlIndex() {
  cleanup();
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
  
  accessDatabase();
  createTables();
}

chunkId_t MysqlIndex::value(objectId_t objID) {
  if (!mysqlDB) return 0xdeadbeef;		// Avoid unnecessary work

  MYSQL_RES* result = findObjectID(objID);
  if (!result) return 0xdeadbeef;

  chunkId_t chunk = extractChunk(result);

  mysql_free_result(result);		// Clean up before exiting

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

  if (!mysql_real_connect(mysqlDB,"127.0.0.1","root","changeme",
			  "",13306,NULL,0)) {
    std::cerr << mysql_error(mysqlDB) << std::endl;
    mysql_close(mysqlDB);
    mysqlDB = 0;
    return false;
  }

  return true;
}

void MysqlIndex::accessDatabase() {
  if (!mysqlDB) return;				// Avoid unnecessary work

  if (verboseLevel) std::cout << "MysqlIndex::accessDatabase" << std::endl;

  std::string makedb = "CREATE DATABASE "+dbname;
  if (verboseLevel>1) std::cout << "sending: " << makedb << std::endl;

  mysql_query(mysqlDB, makedb.c_str());
  reportError();

  std::string usedb = "USE "+dbname;
  if (verboseLevel>1) std::cout << "sending: " << usedb << std::endl;

  mysql_query(mysqlDB, usedb.c_str());
  reportError();
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

    objectId_t tsize = blockSize;	// Last table may be short
    for (int itbl=0; itbl<nTables; itbl++) {
      if (itbl == nTables-1) tsize = tableSize - (nTables-1)*blockSize;
      createTable(itbl);
      fillTable(itbl, tsize, itbl*blockSize);
    }
  } else {
    createTable(-1);				// One massive table
    fillTable(-1, tableSize, 0);
  }
}

void MysqlIndex::createTable(int tblidx) {
  if (!mysqlDB) return;				// Avoid unnecessary work

  if (verboseLevel)
    std::cout << "MysqlIndex::createTable " << tblidx << std::endl;

  std::string maketbl = "CREATE TABLE " + makeTableName(tblidx)
    + " (objectId BIGINT NOT NULL PRIMARY KEY, chunkId INT NOT NULL)"
    + " ENGINE='InnoDB'";

  if (verboseLevel>1) std::cout << "sending: " << maketbl << std::endl;

  mysql_query(mysqlDB, maketbl.c_str());
  reportError();
}

void MysqlIndex::fillTable(int tblidx, objectId_t tsize,
			   objectId_t firstID) {
  if (!mysqlDB) return;				// Avoid unnecessary work

  if (verboseLevel) {
    std::cout << "MysqlIndex::fillTable " << tblidx << " " << tsize
	      << " " << firstID << std::endl;
  }

  // NOTE:  Each (select 0 union all...) extends the table by a factor of 10
  int nTens = (int)std::log10(tsize);
  if (verboseLevel>1)
    std::cout << " Got " << nTens << " powers of ten" << std::endl;

  std::stringstream theInsert;
  theInsert << "INSERT INTO " << makeTableName(tblidx) << " (objectId, chunkId)"
	    << " SELECT @row := @row + " << indexStep << " AS row, 0 FROM \n";
  for (int iTen=0; iTen < nTens; iTen++) {
    theInsert << "(SELECT 0 UNION ALL SELECT 1 UNION ALL SELECT 2 UNION ALL"
	      << " SELECT 3 UNION ALL SELECT 4 UNION ALL SELECT 5 UNION ALL"
	      << " SELECT 6 UNION ALL SELECT 7 UNION ALL SELECT 8 UNION ALL"
	      << " SELECT 9) t" << iTen << ", \n";
  }
  
  // Do a few extras at the end to get the desired DB size
  int nExtra = (int)std::ceil((double)tsize / pow(10., nTens));
  if (verboseLevel>1)
    std::cout << " Got extra factor of " << nExtra << std::endl;

  if (nExtra > 1) {
    theInsert << "(";
    for (int iExtra=0; iExtra < nExtra; iExtra++) {
      theInsert << " SELECT " << iExtra;
      if (iExtra < nExtra-1) theInsert << " UNION ALL";
    }
    theInsert << ") t" << nTens << ", \n";
  }
  
  // Start the ball rolling
  theInsert << "(SELECT @row:=" << firstID << ") t" << nTens+1;

  if (verboseLevel>1)
    std::cout << "sending: " << theInsert.str() << std::endl;

  mysql_query(mysqlDB, theInsert.str().c_str());
  reportError();
}


// Insert contents of external file in one action
// FIXME:  This currently assumes single giant table, no splitting!

void MysqlIndex::update(const char* datafile) {
  if (!datafile) return;		// No external file specified

  if (verboseLevel) std::cout << "MysqlIndex::update " << datafile << std::endl;

  char* realdata = realpath(datafile,0);
  if (!realdata) {
    std::cerr << "MysqlIndex::update " << datafile << " not found" << std::endl;
    return;
  }

  std::stringstream loadIt;
  loadIt << "LOAD DATA INFILE '" << realdata << "' REPLACE INTO TABLE "
	 << makeTableName() << " FIELDS TERMINATED BY '\\t'";

  if (verboseLevel>1) std::cout << "sending: " << loadIt.str() << std::endl;

  mysql_query(mysqlDB, loadIt.str().c_str());
  reportError();

  delete realdata;	// Clean up output of realpath()
}


// Do MySQL query to extract object/chunk pair from correct table

MYSQL_RES* MysqlIndex::findObjectID(objectId_t objID) {
  std::stringstream lookup;
  lookup << "SELECT chunkId FROM " << dbname << "."
	 << makeTableName(chooseTable(objID)) << " WHERE objectId=" << objID;
  
  if (verboseLevel>1) std::cout << "sending: " << lookup.str() << std::endl;
  
  mysql_query(mysqlDB, lookup.str().c_str());
  reportError();
  
  MYSQL_RES *result = mysql_store_result(mysqlDB);

  if (result && mysql_num_rows(result)>0) {
    if (verboseLevel>1)
      std::cout << "got " << mysql_num_rows(result) << " rows, with "
		<< mysql_num_fields(result) << " columns" << std::endl;
  } else {
    std::cerr << "error selecting objectID " << objID << "\n";
    reportError();
  }

  return result;
}


// Parse query result to get chunk ID (there should be only one!)

chunkId_t MysqlIndex::extractChunk(MYSQL_RES* result) {
  if (!result) return 0xdeadbeef;		// Avoid unnecessary work

  MYSQL_ROW row = mysql_fetch_row(result);	// There should be only one!
  if (!row) return 0xdeadbeef;

  if (verboseLevel>1) std::cout << "first result: " << row[0] << std::endl;

  return strtoul(row[0], 0, 0);		// Everything comes back as strings
}


// Discard tables and database at end of job

void MysqlIndex::cleanup() {
  if (!mysqlDB) return;				// Avoid unnecessary work

  if (verboseLevel) std::cout << "MysqlIndex::cleanup" << std::endl;

  if (usingMultipleTables()) {
    for (int itbl=0; itbl<numberOfTables(); itbl++) {
      dropTable(itbl);
    }
  } else {
    dropTable(-1);
  }

  std::string dropdb = "DROP DATABASE " + dbname;
  if (verboseLevel>1) std::cout << "sending: " << dropdb << std::endl;

  mysql_query(mysqlDB, dropdb.c_str());
  reportError();

  mysql_close(mysqlDB);
  mysqlDB = 0;
}

void MysqlIndex::dropTable(int tblidx) {
  if (!mysqlDB) return;				// Avoid unnecessary work
  
  if (verboseLevel>1)
    std::cout << "MysqlIndex::dropTable " << tblidx << std::endl;
  
  std::string droptbl = "DROP TABLE " + makeTableName(tblidx);
  if (verboseLevel>1) std::cout << "sending: " << droptbl << std::endl;

  mysql_query(mysqlDB, droptbl.c_str());
  reportError();
}
