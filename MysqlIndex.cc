// $Id$
// MysqlIndex.hh -- Exercise performance of MySQL with InnoDB as lookup table.
//
// 20160119  Michael Kelsey
// 20160204  Extend to support blocking data into smaller tables

#include "MysqlIndex.hh"
#include <mysql/mysql.h>
#include <iostream>
#include <cmath>
#include <string>
#include <sstream>


MysqlIndex::MysqlIndex(int verbose)
  : IndexTester("mysql",verbose), mysqlDB(0), dbname("SecIdx"),
    table("chunks"), totalSize(0ULL), tableSize(0ULL) {;}

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
  
  int nTables = totalSize / tableSize;
  if (nTables*tableSize < totalSize) nTables++;		// Partial at end
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

void MysqlIndex::create(unsigned long long asize) {
  if (!connect() || !mysqlDB) return;		// Avoid unnecessary work

  totalSize = asize;		// Store for use in filling and querying
  
  accessDatabase();
  createTables();
}

int MysqlIndex::value(unsigned long long objID) {
  if (!mysqlDB) return 0xdeadbeef;		// Avoid unnecessary work

  MYSQL_RES* result = findObjectID(objID);
  if (!result) return 0xdeadbeef;

  int chunk = extractChunk(result);

  mysql_free_result(result);		// Clean up before exiting

  return chunk;
}


bool MysqlIndex::connect(const char* dbname) {
  if (verboseLevel)
    std::cout << "MysqlIndex::connect " << (dbname?dbname:"") << std::endl;

  if (mysqlDB) cleanup();			// Get rid of previous version

  // connect to mysql server, no particular database
  mysqlDB = mysql_init(NULL);
  
  if (!mysql_real_connect(mysqlDB,"127.0.0.1","root","changeme",dbname,13306,NULL,0)) {
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
    std::cout << "MysqlIndex::createTables (up to " << tableSize << " entries)"
	      << std::endl;
  }

  if (usingMultipleTables()) {			// One table for each block
    int nTables = numberOfTables();
    if (verboseLevel>1) {
      std::cout << " creating " << nTables << " tables with "
		<< tableSize << " entries" << std::endl;
    }

    unsigned long long tsize = tableSize;	// Last table may be short
    for (int itbl=0; itbl<nTables; itbl++) {
      if (itbl == nTables-1) tsize = totalSize - (nTables-1)*tableSize;
      createTable(itbl);
      fillTable(itbl, tsize, itbl*tableSize);
    }
  } else {
    createTable(-1);				// One massive table
    fillTable(-1, totalSize, 0);
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

  mysql_query(mysqlDB, "GRANT ALL on *.* TO ''@'%'");
  reportError();
}

void MysqlIndex::fillTable(int tblidx, unsigned long long tsize,
			   unsigned long long firstID) {
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
	    << " SELECT @row := @row + 1 AS row, 0 FROM \n";
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


// Do MySQL query to extract object/chunk pair from correct table

MYSQL_RES* MysqlIndex::findObjectID(unsigned long long objID) {
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

int MysqlIndex::extractChunk(MYSQL_RES* result) {
  if (!result) return 0xdeadbeef;		// Avoid unnecessary work

  MYSQL_ROW row = mysql_fetch_row(result);	// There should be only one!
  if (!row) return 0xdeadbeef;

  if (verboseLevel>1) std::cout << "first result: " << row[0] << std::endl;

  return strtol(row[0], 0, 0);		// Everything comes back as strings
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
