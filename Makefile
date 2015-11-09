# $Id$
#
# Makefile for building objectID-chunk indexing test programs.
#
# 20151026  Michael Kelsey
# 20151103  Use $(pkg-config) to test for Memcached support

# Source and header files

SRC := index-performance.cc simple-array.cc block-array.cc flat-file.cc	

LIB := libindextest.a

LIBSRC := UsageTimer.cc IndexTester.cc ArrayIndex.cc BlockArrays.cc \
	MapIndex.cc FileIndex.cc

# Check local platform for Memcached API library

HASMEMCACHED := $(shell pkg-config --modversion --silence-errors libmemcached)
ifneq (,$(HASMEMCACHED))
  SRC += memcd-index.cc
  LIBSRC += MemCDIndex.cc
endif

# Check local platform for XRootD

ifneq (,$(XROOTD_DIR))
  SRC +=  simple-xrd.cc
  LIBSRC += XrootdSimple.cc
endif

# User targets

all : lib bin

bin : $(SRC:.cc=)
lib : $(LIB)

clean :
	/bin/rm -f $(SRC:.cc=.o) $(LIBSRC:.cc=.o) *~

veryclean : clean
	/bin/rm -f $(SRC:.cc=) $(LIB)

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

$(SRC) : IndexTester.hh UsageTimer.hh
$(LIBSRC) : %.cc:%.hh

$(SRC:.cc=) : $(LIB)

$(LIB) : $(LIBSRC:.cc=.o)
	$(AR) -r $@ $^
