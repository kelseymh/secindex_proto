#ifndef INDEX_TESTER_HH
#define INDEX_TESTER_HH 1
// $Id$
// IndexTester.hh -- Interface base class for creating, exercising and
// recording performance of different two-column lookup tables.
//
// 20151023  Michael Kelsey
// 20151113  Add accessors for verbosity and name
// 20160216  Add interface and optional subclass function for bulk updates
// 20160217  FINALLY:  Provide typedefs for "objectId_t" and "chunkId_t"

#include "UsageTimer.hh"
#include <iosfwd>

// Use these everywhere for abstraction/convenience

typedef unsigned long long objectId_t;
typedef unsigned int chunkId_t;

class IndexTester {
public:
  IndexTester(const char* name, int verbose=0);
  virtual ~IndexTester() {;}

  void SetVerboseLevel(int verbose) { verboseLevel = verbose; }
  int GetVerboseLevel() const { return verboseLevel; }
  const char* GetName() const { return tableName; }

  // Support sparse objectID values to support bulk update tests
  void SetIndexSpacing(unsigned step=1) { indexStep = step; }
  unsigned GetIndexSpacing() const { return indexStep; }

  // Generate test and print comma-separated data; asize=0 for column headings
  virtual void TestAndReport(objectId_t asize, long ntrials, std::ostream& csv);

  void CreateTable(objectId_t asize);
  void UpdateTable(const char* datafile=0);
  void ExerciseTable(long ntrials);
  const UsageTimer& GetUsage() const { return usage; }

protected:
  // Subclass must implement their own specific table creator and accessor
  virtual void create(objectId_t asize) = 0;
  virtual void update(const char* datafile) {;}		// May be unimplemented
  virtual chunkId_t value(objectId_t index) = 0;

  objectId_t randomIndex() const;

  int verboseLevel;		// For informational messages
  objectId_t tableSize;		// Used to generate random indices
  unsigned indexStep;		// Interval for generating object IDs

private:
  const char* tableName;	// For writing CSV output
  UsageTimer usage;		// For collecting time and memory data
  long lastTrials;		// Last set of trials performed (for CSV)
};

#endif	/* INDEX_TESTER_HH */
