# $Id$
#
# Makefile for building objectID-chunk indexing test programs.
#
# 20151026  Michael Kelsey

# Source and header files

SRC := simple-array.cc block-array.cc flat-file.cc memcd-index.cc \
	index-performance.cc

LIB := libindextest.a

LIBSRC := UsageTimer.cc IndexTester.cc \
	ArrayIndex.cc BlockArrays.cc MapIndex.cc FileIndex.cc MemCDIndex.cc

# User targets

all : lib bin

bin : $(SRC:.cc=)
lib : $(LIB)

clean :
	/bin/rm -f $(SRC:.cc=.o) $(LIBSRC:.cc=.o) *~

veryclean : clean
	/bin/rm -f $(SRC:.cc=) $(LIB)

# Incorporate /usr/local in building

CPPFLAGS += -I/usr/local/include
LDFLAGS += -L. -L/usr/local/lib
LDLIBS += -lindextest -lmemcached

# Dependencies

simple-array.cc index-performance.cc : ArrayIndex.hh
block-array.cc index-performance.cc  : BlockArrays.hh
flat-file.cc index-performance.cc    : FileIndex.hh
memcd-index.cc index-performance.cc  : MemCDIndex.hh

IndexTester.hh : UsageTimer.hh

ArrayIndex.cc BlockArrays.cc \
FileIndex.cc MemCDIndex.cc : IndexTester.hh

$(SRC) : IndexTester.hh UsageTimer.hh
$(LIBSRC) : %.cc:%.hh

$(SRC:.cc=) : $(LIB)

$(LIB) : $(LIBSRC:.cc=.o)
	$(AR) -r $@ $^
