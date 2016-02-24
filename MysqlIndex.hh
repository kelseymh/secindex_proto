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
#include <vector>


class MysqlIndex : public IndexTester {
public:
  MysqlIndex(int verbose=0);
  virtual ~MysqlIndex();

public:
  // Call this function before running to create multiple smaller tables
  void setTableSize(objectId_t max=0ULL) { blockSize = max; }

protected:
  virtual void create(objectId_t asize);
  virtual void update(const char* datafile);
  virtual chunkId_t value(objectId_t objID);

  bool connect(const std::string& newDBname="");
  void accessDatabase() const;

  void createTables() const;			// One for all, or multiple
  void createTable(int tblidx=-1) const;	// >= 0 allows data blocks
  void fillTable(int tblidx, objectId_t tsize, objectId_t firstID) const;

  void updateTable(const char* datafile, int tblidx=-1) const;

  void getObjectRange(int tblidx, objectId_t &minID, objectId_t &maxID) const;

  void createLoadFile(const char* datafile, objectId_t fsize,
		      objectId_t start, unsigned step) const;

  void cleanup();
  void dropTable(int tblidx=-1) const;		// Drop specified table

  MYSQL_RES* findObjectID(objectId_t objID) const;  // Get chunk for given ID

  // Wrapper functions to handle multiple tables for data blocks

  bool usingMultipleTables() const {		// Flag if data is in blocks
    return (blockSize != 0ULL && blockSize < tableSize);
  }

  int numberOfTables() const;			// Total number of data blocks

  void fillTableRanges();			// Populate ID range lookup

  int chooseTable(objectId_t objID) const;	// Identify block table for ID

  std::string makeTableName(int tblidx=-1) const;	// Unique block names

  // Wrapper functions to generate and process queries

  void sendQuery(const std::string& query) const;  // Transmit query to server
  void reportError() const;			// Print MySQL message if any
  MYSQL_RES* getQueryResult() const;		// Result container w/err check

  chunkId_t extractChunk(MYSQL_RES* result, size_t irow=0) const;
  objectId_t extractObject(MYSQL_RES* result, size_t irow=0) const;

private:
  MYSQL *mysqlDB;
  std::string dbname;
  std::string table;

  objectId_t blockSize;			// For dividing overly large tables
  std::vector<objectId_t> blockStart;	// Lowest objectID in each table block
};

#endif	/* MYSQL_INDEX_HH */
