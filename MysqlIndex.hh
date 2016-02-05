#ifndef MYSQL_INDEX_HH
#define MYSQL_INDEX_HH 1
// $Id$
// MysqlIndex.hh -- Exercise performance of MysqlDB for lookup table.
//
// 20160119  Michael Kelsey
// 20160204  Extend to support blocking data into smaller tables

#include "IndexTester.hh"
#include <mysql/mysql.h>	/* Needed for MYSQL typedef below */
#include <string>


class MysqlIndex : public IndexTester {
public:
  MysqlIndex(int verbose=0);
  virtual ~MysqlIndex();

public:
  // Call this function before running to create multiple smaller tables
  void setTableSize(unsigned long long max=0ULL) { tableSize = max; }

protected:
  virtual void create(unsigned long long asize);
  virtual int value(unsigned long long objID);

  bool connect(const char* dbname=0);
  void accessDatabase();

  void createTables();				// One for all, or multiple
  void createTable(int tblidx=-1);		// >= 0 allows data blocks
  void fillTable(int tblidx, unsigned long long tsize,
		 unsigned long long firstID);

  void cleanup();
  void dropTable(int tblidx=-1);		// Drop specified table

  MYSQL_RES* findObjectID(unsigned long long objID);  	  // Does MySQL query
  int extractChunk(MYSQL_RES* result);		          // Parses result

  // Wrapper functions to handle multiple tables for data blocks

  bool usingMultipleTables() const {		// Flag if data is in blocks
    return (tableSize != 0ULL && tableSize < totalSize);
  }

  int numberOfTables() const;			// Total number of data blocks

  int chooseTable(unsigned long long objID) const {	// Get table for ID
    return (usingMultipleTables() ? (objID/tableSize) : -1);
  }

  std::string makeTableName(int tblidx=-1) const;	// Unique block names

  void reportError() const;			// Print MySQL message if any

private:
  MYSQL *mysqlDB;
  std::string dbname;
  std::string table;

  unsigned long long totalSize;		// Save size from create() for blocking
  unsigned long long tableSize;		// Table size for data blocking
};

#endif	/* MYSQL_INDEX_HH */
