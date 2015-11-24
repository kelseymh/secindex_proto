#!/bin/csh -f
#
# Runs all of the performance test jobs, collecting CSV files from each
# Data collection is at 10x and 30x (logarithmic steps)
#
# 20151025  Michael Kelsey
# 20151113  Add XRootD, drop redirection (handled using job name)
# 20151123  Update test ranges to reflect latest performance results
# 20151124  Add RocksDB test

./index-performance array     100000000  15000000000
./index-performance blocks    100000000  15000000000
./index-performance stdmap     10000000    300000000
./index-performance file      100000000 100000000000
./index-performance memcached  10000000    150000000
./index-performance xrootd     10000000  10000000000
./index-performance rocksdb    10000000  10000000000
