# $Id$
#
# Makefile for building objectID-chunk indexing test programs.
#
# 20151026  Michael Kelsey
# 20151103  Use $(pkg-config) to test for Memcached support
# 20151106  Add XRootD support
# 20151110  Reduce number of intermediate files by using lib(obj) dependences

# Source and header files

LIBSRC := UsageTimer.cc IndexTester.cc ArrayIndex.cc BlockArrays.cc \
	MapIndex.cc FileIndex.cc
BINSRC := index-performance.cc simple-array.cc block-array.cc flat-file.cc	

# Check local platform for Memcached API library

HASMEMCACHED := $(shell pkg-config --modversion --silence-errors libmemcached)
ifneq (,$(HASMEMCACHED))
  BINSRC += memcd-index.cc
  LIBSRC += MemCDIndex.cc
endif

# Check local platform for XRootD

ifneq (,$(XROOTD_DIR))
  BINSRC +=  simple-xrd.cc query-xrd.cc
  LIBSRC += XrootdSimple.cc
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

# Incorporate /usr/local in building

LDFLAGS += -L.
LDLIBS  += -lindextest

ifneq (,$(HASMEMCACHED))
  CPPFLAGS += -I/usr/local/include
  LDFLAGS  += -L/usr/local/lib
  LDLIBS   += -lmemcached
endif

ifneq (,$(XROOTD_DIR))
  CPPFLAGS += -I$(XROOTD_DIR)/include/xrootd
  LDFLAGS += -L$(XROOTD_DIR)/lib
  LDLIBS += -lXrdCl
endif

# Dependencies

simple-array.cc index-performance.cc : ArrayIndex.hh
block-array.cc index-performance.cc  : BlockArrays.hh
flat-file.cc index-performance.cc    : FileIndex.hh
memcd-index.cc index-performance.cc  : MemCDIndex.hh
simple-xrd.cc index-performance.cc   : XrootdSimple.hh
index-performance.cc                 : MapIndex.hh

IndexTester.hh : UsageTimer.hh

ArrayIndex.cc BlockArrays.cc \
MapIndex.cc FileIndex.cc \
MemCDIndex.cc XrootdSimple.cc : IndexTester.hh

$(BINSRC) : IndexTester.hh UsageTimer.hh
$(LIBSRC) : %.cc:%.hh

$(BIN) : $(LIB)

$(LIB) : $(patsubst %,$(LIB)(%),$(LIBO))
