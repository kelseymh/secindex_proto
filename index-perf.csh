#!/bin/csh -f
#
# Runs all of the performance test jobs, collecting CSV files from each
# Data collection is at 10x and 30x (logarithmic steps)
#
# 20151025  Michael Kelsey

./index-performance array     100000000  15000000000 > array.csv
./index-performance blocks    100000000  15000000000 > blocks.csv
./index-performance file      100000000 100000000000 > file.csv
./index-performance memcached 100000000  50000000000 > memcd.csv
./index-performance stdmap    100000000    300000000 > stdmap.csv
./index-performance xrootd     10000000    1000000000 > xrootd.csv
