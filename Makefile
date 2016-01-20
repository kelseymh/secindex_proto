# $Id$
#
# Makefile for building objectID-chunk indexing test programs.
#
# 20151026  Michael Kelsey
# 20151103  Use $(pkg-config) to test for Memcached support
# 20151106  Add XRootD support
# 20151110  Reduce number of intermediate files by using lib(obj) dependences
# 20151124  Add RocksDB support, need to require C++11
# 20160119  Add MysqlDB support

# Source and header files

LIBSRC := UsageTimer.cc IndexTester.cc ArrayIndex.cc BlockArrays.cc \
	MapIndex.cc FileIndex.cc

BINSRC := index-performance.cc simple-array.cc block-array.cc flat-file.cc

# Incorporate /usr/local in building

CXXFLAGS += -std=c++11 -g
CPPFLAGS += -I/usr/local/include
LDFLAGS += -L. -L/usr/local/lib
LDLIBS  += -lindextest

# Check local platform for Memcached API library

HASMEMCACHED := $(shell pkg-config --modversion --silence-errors libmemcached)
ifneq (,$(HASMEMCACHED))
  BINSRC += memcd-index.cc
  LIBSRC += MemCDIndex.cc
  CPPFLAGS += -DHAS_MEMCACHED=1
  LDLIBS   += -lmemcached
endif

# Check local platform for XRootD

ifneq (,$(XROOTD_DIR))
  BINSRC +=  simple-xrd.cc query-xrd.cc
  LIBSRC += XrootdSimple.cc
  CPPFLAGS += -I$(XROOTD_DIR)/include/xrootd -DHAS_XROOTD=1
  LDFLAGS += -L$(XROOTD_DIR)/lib
  LDLIBS += -lXrdCl
endif

# Check local platform for RocksDB (NOTE: Not registered with pkg-config)

HASROCKSDB := $(shell ls -d /usr/local/include/rocksdb 2> /dev/null)
ifneq  (,$(HASROCKSDB))
  BINSRC += rocksdb-index.cc
  LIBSRC += RocksIndex.cc
  CPPFLAGS += -DHAS_ROCKSDB=1
  LDLIBS += -lrocksdb
endif

# Check for LSST installation of MYSQL

ifneq (,$(MYSQLCLIENT_DIR))
  BINSRC += mysql-index.cc
  LIBSRC += MysqlIndex.cc
  CPPFLAGS += -DHAS_MYSQL=1 -I$(MYSQLCLIENT_DIR)/include
  LDFLAGS += -L$(MYSQLCLIENT_DIR)/lib
  LDLIBS += -lmysqlclient
endif

# Derivatives of source files 

LIB := libindextest.a
LIBO := $(LIBSRC:.cc=.o)

BIN := $(BINSRC:.cc=)

# User targets

all : lib bin

bin : $(BIN)
lib : $(LIB)

clean :
	/bin/rm -f $(LIBO) *~

veryclean : clean
	/bin/rm -f $(BIN) $(LIB)

# Dependencies

simple-array.cc index-performance.cc  : ArrayIndex.hh
block-array.cc index-performance.cc   : BlockArrays.hh
flat-file.cc index-performance.cc     : FileIndex.hh
memcd-index.cc index-performance.cc   : MemCDIndex.hh
simple-xrd.cc index-performance.cc    : XrootdSimple.hh
rocksdb-index.cc index-performance.cc : RocksIndex.hh
mysql-index.cc index-performance.cc   : MysqlIndex.hh
index-performance.cc                  : MapIndex.hh

IndexTester.hh : UsageTimer.hh

ArrayIndex.cc BlockArrays.cc \
MapIndex.cc FileIndex.cc \
MemCDIndex.cc XrootdSimple.cc \
RocksIndex.cc MysqlIndex.cc : IndexTester.hh

$(BINSRC) : IndexTester.hh UsageTimer.hh
$(LIBSRC) : %.cc:%.hh

$(BIN) : $(LIB)

$(LIB) : $(patsubst %,$(LIB)(%),$(LIBO))
