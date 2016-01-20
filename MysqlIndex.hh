#ifndef MYSQL_INDEX_HH
#define MYSQL_INDEX_HH 1
// $Id$
// MysqlIndex.hh -- Exercise performance of MysqlDB for lookup table.
//
// 20160119  Michael Kelsey

#include "IndexTester.hh"
#include <mysql/mysql.h>	/* Needed for MYSQL typedef below */
#include <string>


class MysqlIndex : public IndexTester {
public:
  MysqlIndex(int verbose=0);
  virtual ~MysqlIndex();

protected:
  virtual void create(unsigned long long asize);
  virtual int value(unsigned long long index);

  bool connect(const char* dbname=0);
  void createTable();
  void fillTable(unsigned long long asize);
  void cleanup();

  void reportError();		// Print MySQL error message if any

private:
  MYSQL *mysqlDB;
  std::string dbname;
  std::string table;
};

#endif	/* MYSQL_INDEX_HH */
