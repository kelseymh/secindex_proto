#ifndef MYSQL_UPDATE_HH
#define MYSQL_UPDATE_HH 1
// $Id$
// MysqlUpdate.hh -- Exercise performance of MysqlDB with bulk updating
//
// 20160217  Michael Kelsey -- sets bulk data file automatically
// 20160224  Allow configuration of bulk-update size, cleanup() function

#include "MysqlIndex.hh"

class MysqlUpdate : public MysqlIndex {
public:
  MysqlUpdate(int verbose=0);
  virtual ~MysqlUpdate() { cleanup(); }

  // Generate test and print comma-separated data; asize=0 for column headings
  // NOTE:  THIS IS A HACK WITH COPY AND MODIFIED CODE FROM BASE
  virtual void TestAndReport(objectId_t asize, long ntrials, std::ostream& csv);

  void setUpdateSize(objectId_t usize) { bulksize = usize; }

protected:
  // Do base class database creation, then construct bulk-update file
  virtual void create(objectId_t asize);
  virtual void update(const char* datafile) {
    MysqlIndex::update(datafile?datafile:bulkfile);
  }

  virtual void cleanup();

private:
  const char* bulkfile;		// Name of bulk-udpate input file
  objectId_t bulksize;		// Number of entries for bulk-update
};

#endif	/* MYSQL_UPDATE_HH */
