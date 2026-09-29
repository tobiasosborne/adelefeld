#!/bin/sh
# The three full runs of item 6 of the brief, one after the other; run from the repository root.
make clean >/dev/null 2>&1
make -j2 check > lanes/s3-slice2/check-plain.log 2>&1
echo "plain: exit $?" >> lanes/s3-slice2/check-plain.log
make clean >/dev/null 2>&1
make -j2 check SAN=1 > lanes/s3-slice2/check-san.log 2>&1
echo "san: exit $?" >> lanes/s3-slice2/check-san.log
make clean >/dev/null 2>&1
make -j2 check CC=clang > lanes/s3-slice2/check-clang.log 2>&1
echo "clang: exit $?" >> lanes/s3-slice2/check-clang.log
make clean >/dev/null 2>&1
echo finished > lanes/s3-slice2/checks.done
