This directory contains standalone classes and exeuctables for testing
different models for the objectID-chunk "secondary index."  There are
several kinds of indexes we are currently exploring:

1)  A memory resident C-style array, with the objectID as the array index
    and chunk number as value.  Two versions of this are implemented, one
    with a single large array, the other as a set of smaller block arrays,
    each one allocated separately.

    NOTE:  This implementation is incorrect.  The objectID may require a
    full 64-bit range of values, with the estimated 40 billion objects
    very sparsely filling that range.  The objectID cannot be used as a
    simple array index under those conditions.

2)  A flat file (on SSD for fast access), with the objectID representing
    an offset into the file, and the chunk number stored in binary.  This is
    implemented as a "large" (64-bit file size) file.

    NOTE:  This implementation is incorrect.  The objectID may require a
    full 64-bit range of values, with the estimated 40 billion objects
    very sparsely filling that range.  The objectID cannot be used as a
    simple array index under those conditions.

3)  A memory resident key-value lookup using std::map<>, with the objectID
    as the key, and the chunk number as value.

4)  A client-server lookup using Memcached to store key-value pairs, with
    both the objectID and chunk number stored as byte strings.

*** Building requires that the user has installed both the memcached server,
    as well as the libmemcached API.  The latter is available for MacOSX
    from Homebrew, |brew install libmemcached|.

5)  A client-server lookup using XRootD to store and access a set of flat
    files, as in option (2).

*** Building requires that the user has installed XRootD (which should come
    in through the LSST QServ stack).

6)  A memory and file resident key-value lookup using RocksDB, with the
    objectID as the key, and the chunk number as value.

*** Building requires that the user has installed RocksDB, available for
    MacOSX from Homebrew, |brew install rocksdb|.

The main driver program is |index-performance|, which provides a command
line interface to select which index model to test, and a range of sizes.

There is also a shell script, |index-perf.csh|, which runs jobs for all the
different models, and collects performance results in .csv files.

The Makefile includes checks for the third-party packages needed above, and
will exclude (without error) building those tests which have missing
dependences.
