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
  : IndexTester("mysql",verbose), mysqlDB(0) {;}

MysqlIndex::~MysqlIndex() {
  cleanup();
}


void MysqlIndex::create(unsigned long long asize) {
  if (!connect() || !mysqlDB) return;		// Avoid unnecessary work

  createTable();
  fillTable(asize);
}

int MysqlIndex::value(unsigned long long index) {
  if (!mysqlDB) return 0xdeadbeef;		// Avoid unnecessary work
  std::stringstream lookup;
  lookup << "SELECT chunkId FROM T WHERE objectId=" << index;

  if (verboseLevel>1) std::cout << "sending: " << lookup.str() << std::endl;

  mysql_query(mysqlDB, lookup.str().c_str());

  MYSQL_RES *result = mysql_store_result(mysqlDB);
  if (!result) {
    std::cerr << "error selecting objectID " << index << "\n"
	      << mysql_error(mysqlDB) << std::endl;
    return 0xdeadbeef;
  }

  MYSQL_ROW row = mysql_fetch_row(result);	// There should be only one!
  if (verboseLevel>1) {
    std::cout << "first result: " << row[0] << " returned " << row[1]
	      << std::endl;
  }

  int chunk = strtol(row[1], 0, 0);
  mysql_free_result(result);

  return chunk;
}


bool MysqlIndex::connect(const char* dbname) {
  if (verboseLevel)
    std::cout << "MysqlIndex::connect " << (dbname?dbname:"") << std::endl;

  if (mysqlDB) cleanup();			// Get rid of previous version

  // connect to mysql server, no particular database
  mysqlDB = mysql_init(NULL);
  
  if (!mysql_real_connect(mysqlDB,"127.0.0.1","kelsey","",dbname,3306,NULL,0)) {
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

  mysql_query(mysqlDB, "CREATE DATABASE SecIdx");
  mysql_close(mysqlDB);
  mysqlDB = 0;

  if (!connect("SecIdx")) return;

  mysql_query(mysqlDB, "CREATE TABLE m(objectId BIGINT NOT NULL PRIMARY KEY, chunkId INT NOT NULL) ENGINE='InnoDB'");
}

void MysqlIndex::fillTable(unsigned long long asize) {
  if (!mysqlDB) return;				// Avoid unnecessary work

  if (verboseLevel) std::cout << "MysqlIndex::fillTable " << asize << std::endl;

  // NOTE:  Each (select 0 union all...) extends the table by a factor of 10
  int nTens = (int)std::log10(asize);
  if (verboseLevel>1)
    std::cout << " Got " << nTens << " powers of ten" << std::endl;

  std::stringstream theInsert;
  theInsert << "INSERT INTO m(objectId, chunkID)"
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
    std::cout << " Got " << nExtra << " extra factor" << std::endl;

  if (nExtra > 1) {
    theInsert << "(";
    for (int iExtra=0; iExtra < nExtra; iExtra++) {
      theInsert << "SELECT " << iExtra << " UNION ALL";
    }
    theInsert << ") t" << nTens << ", \n";
  }
  
  // Start the ball rolling
  theInsert << "(SELECT @row:=0) t" << nTens+1;

  if (verboseLevel>1)
    std::cout << "sending: " << theInsert.str() << std::endl;

  mysql_query(mysqlDB, theInsert.str().c_str());
}


void MysqlIndex::cleanup() {
  if (!mysqlDB) return;				// Avoid unnecessary work

  if (verboseLevel) std::cout << "MysqlIndex::cleanup" << std::endl;

  mysql_query(mysqlDB, "DROP TABLE m");
  mysql_query(mysqlDB, "DROP DATABASE SecIdx");
  mysql_close(mysqlDB);
  mysqlDB = 0;
}
