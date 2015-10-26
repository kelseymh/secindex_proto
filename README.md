This directory contains standalone classes and exeuctables for testing
different models for the objectID-chunk "secondary index."  There are
several kinds of indexes we are currently exploring:

1)  A memory resident C-style array, with the objectID as the array index
    and chunk number as value.

2)  A flat file (on SSD for fast access), with the objectID representing
    an offset into the file, and the chunk number stored in binary.

3)  A client-server lookup using Memcached to store key-value pairs, with
    both the objectID and chunk number stored as byte strings.

The main driver program is |index-performance|, which provides a command
line interface to select which index model to test, and a range of sizes.

There is also a shell script, |index-perf.csh|, which runs jobs for all the
different models, and collects performance results in .csv files.

Building requires that the user has installed both the memcached server, as
well as the libmemcached API.  The latter is available for MacOSX from
Homebrew, |brew install libmemcached|.
