// $Id$
// MysqlIndex.hh -- Exercise performance of MySQL with InnoDB as lookup table.
//
// 20160119  Michael Kelsey

#include "MysqlIndex.hh"
#include <mysql/mysql.h>
#include <iostream>
#include <cmath>
#include <string>
#include <sstream>


MysqlIndex::MysqlIndex(int verbose)
  : IndexTester("mysql",verbose), mysqlDB(0),
    dbname("SecIdx"), table("chunks") {;}

MysqlIndex::~MysqlIndex() {
  cleanup();
}


// Print MySQL error message if any in present

void MysqlIndex::reportError() {
  if (mysql_error(mysqlDB)[0] == '\0') return;	// No current error message

  std::cerr << mysql_error(mysqlDB) << std::endl;
}


// Create and populate secondary index from scratch

void MysqlIndex::create(unsigned long long asize) {
  if (!connect() || !mysqlDB) return;		// Avoid unnecessary work

  createTable();
  fillTable(asize);
}

int MysqlIndex::value(unsigned long long index) {
  if (!mysqlDB) return 0xdeadbeef;		// Avoid unnecessary work

  std::stringstream lookup;
  lookup << "SELECT chunkId FROM " << dbname << "." << table << " WHERE objectId=" << index;

  if (verboseLevel>1) std::cout << "sending: " << lookup.str() << std::endl;

  mysql_query(mysqlDB, lookup.str().c_str());
  reportError();

  MYSQL_RES *result = mysql_store_result(mysqlDB);
  if (!result) {
    std::cerr << "error selecting objectID " << index << "\n";
    reportError();

    return 0xdeadbeef;
  }

  if (verboseLevel>1)
    std::cout << "got " << mysql_num_rows(result) << " rows, with "
	      << mysql_num_fields(result) << " columns" << std::endl;

  MYSQL_ROW row = mysql_fetch_row(result);	// There should be only one!
  if (!row) {
    std::cerr << "error!  null result selecting objectID " << index
	      << std::endl; 
    return 0xdeadbeef;
  }

  if (verboseLevel>1) std::cout << "first result: " << row[0] << std::endl;

  int chunk = strtol(row[0], 0, 0);
  mysql_free_result(result);

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

void MysqlIndex::createTable() {
  if (!mysqlDB) return;				// Avoid unnecessary work

  if (verboseLevel) std::cout << "MysqlIndex::createTable" << std::endl;

  std::string makedb = "CREATE DATABASE "+dbname;
  if (verboseLevel>1) std::cout << "sending: " << makedb << std::endl;

  mysql_query(mysqlDB, makedb.c_str());
  reportError();

  std::string usedb = "USE "+dbname;
  if (verboseLevel>1) std::cout << "sending: " << usedb << std::endl;

  mysql_query(mysqlDB, usedb.c_str());
  reportError();

  std::string maketbl = "CREATE TABLE " + table + " (objectId BIGINT NOT NULL PRIMARY KEY, chunkId INT NOT NULL) ENGINE='InnoDB'";
  if (verboseLevel>1) std::cout << "sending: " << maketbl << std::endl;

  mysql_query(mysqlDB, maketbl.c_str());
  reportError();

  mysql_query(mysqlDB, "GRANT ALL on *.* TO ''@'%'");
  reportError();

  mysql_query(mysqlDB, "GRANT ALL on *.* TO 'kelsey'@'%'");
  reportError();
}

void MysqlIndex::fillTable(unsigned long long asize) {
  if (!mysqlDB) return;				// Avoid unnecessary work

  if (verboseLevel) std::cout << "MysqlIndex::fillTable " << asize << std::endl;

  // NOTE:  Each (select 0 union all...) extends the table by a factor of 10
  int nTens = (int)std::log10(asize);
  if (verboseLevel>1)
    std::cout << " Got " << nTens << " powers of ten" << std::endl;

  std::stringstream theInsert;
  theInsert << "INSERT INTO " << table << "(objectId, chunkId)"
	    << " SELECT @row := @row + 1 AS row, 0 FROM \n";
  for (int iTen=0; iTen < nTens; iTen++) {
    theInsert << "(SELECT 0 UNION ALL SELECT 1 UNION ALL SELECT 2 UNION ALL"
	      << " SELECT 3 UNION ALL SELECT 4 UNION ALL SELECT 5 UNION ALL"
	      << " SELECT 6 UNION ALL SELECT 7 UNION ALL SELECT 8 UNION ALL"
	      << " SELECT 9) t" << iTen << ", \n";
  }
  
  // Do a few extras at the end to get the desired DB size
  int nExtra = (int)std::ceil((double)asize / pow(10., nTens));
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
  theInsert << "(SELECT @row:=0) t" << nTens+1;

  if (verboseLevel>1)
    std::cout << "sending: " << theInsert.str() << std::endl;

  mysql_query(mysqlDB, theInsert.str().c_str());
  reportError();
}


void MysqlIndex::cleanup() {
  if (!mysqlDB) return;				// Avoid unnecessary work

  if (verboseLevel) std::cout << "MysqlIndex::cleanup" << std::endl;

  std::string droptbl = "DROP TABLE " + table;
  mysql_query(mysqlDB, droptbl.c_str());
  reportError();

  std::string dropdb = "DROP DATABASE " + dbname;
  mysql_query(mysqlDB, dropdb.c_str());
  reportError();

  mysql_close(mysqlDB);
  mysqlDB = 0;
}
