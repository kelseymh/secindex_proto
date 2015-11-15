#ifndef INDEX_TESTER_HH
#define INDEX_TESTER_HH 1
// $Id$
// IndexTester.hh -- Interface base class for creating, exercising and
// recording performance of different two-column lookup tables.
//
// 20151023  Michael Kelsey
// 20151113  Add accessors for verbosity and name

#include "UsageTimer.hh"
#include <iosfwd>

class IndexTester {
public:
  IndexTester(const char* name, int verbose=0) :
    tableName(name), verboseLevel(verbose) {;}
  virtual ~IndexTester() {;}

  void SetVerboseLevel(int verbose) { verboseLevel = verbose; }
  int GetVerboseLevel() const { return verboseLevel; }
  const char* GetName() const { return tableName; }

  // Generate test and print comma-separated data; asize=0 for column headings
  void TestAndReport(unsigned long long asize, long ntrials, std::ostream& csv);

  void CreateTable(unsigned long long asize);
  void ExerciseTable(long ntrials);
  const UsageTimer& GetUsage() const { return usage; }

protected:
  // Subclass must implement their own specific table creator and accessor
  virtual void create(unsigned long long asize) = 0;
  virtual int value(unsigned long long index) = 0;

  unsigned long long randomIndex() const;

  int verboseLevel;		// For informational messages
  unsigned long long tableSize;	// Used to generate random indices

private:
  const char* tableName;	// For writing CSV output
  UsageTimer usage;		// For collecting time and memory data
  long lastTrials;		// Last set of trials performed (for CSV)
};

#endif	/* INDEX_TESTER_HH */
