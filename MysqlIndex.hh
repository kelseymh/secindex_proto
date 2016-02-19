#ifndef MYSQL_INDEX_HH
#define MYSQL_INDEX_HH 1
// $Id$
// MysqlIndex.hh -- Exercise performance of MysqlDB for lookup table.
//
// 20160119  Michael Kelsey
// 20160204  Extend to support blocking data into smaller tables
// 20160216  Add support for doing "bulk updates" from flat files

#include "IndexTester.hh"
#include <mysql/mysql.h>	/* Needed for MYSQL typedef below */
#include <string>


class MysqlIndex : public IndexTester {
public:
  MysqlIndex(int verbose=0);
  virtual ~MysqlIndex();

public:
  // Call this function before running to create multiple smaller tables
  void setTableSize(objectId_t max=0ULL) { blockSize = max; }

protected:
  virtual void create(objectId_t asize);
  virtual void update(const char* datafile, int tblidx=-1);
  virtual chunkId_t value(objectId_t objID);

  bool connect(const std::string& newDBname="");
  void accessDatabase();

  void createTables();				// One for all, or multiple
  void createTable(int tblidx=-1);		// >= 0 allows data blocks

  void fillTable(int tblidx, objectId_t tsize,
		 objectId_t firstID);

  void createLoadFile(const char* datafile, objectId_t fsize,
		      objectId_t start, unsigned step) const;

  void cleanup();
  void dropTable(int tblidx=-1);		// Drop specified table

  MYSQL_RES* findObjectID(objectId_t objID);	// Does MySQL query
  chunkId_t extractChunk(MYSQL_RES* result);	// Parses result

  // Wrapper functions to handle multiple tables for data blocks

  bool usingMultipleTables() const {		// Flag if data is in blocks
    return (blockSize != 0ULL && blockSize < tableSize);
  }

  int numberOfTables() const;			// Total number of data blocks

  int chooseTable(objectId_t objID) const {	// Get table for ID
    return (usingMultipleTables() ? (objID/blockSize) : -1);
  }

  std::string makeTableName(int tblidx=-1) const;	// Unique block names

  void reportError() const;			// Print MySQL message if any

private:
  MYSQL *mysqlDB;
  std::string dbname;
  std::string table;

  objectId_t blockSize;		// For dividing overly large tables
};

#endif	/* MYSQL_INDEX_HH */
