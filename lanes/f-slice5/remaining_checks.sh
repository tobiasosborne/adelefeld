#!/bin/sh
for mode in san clang inv; do
  case "$mode" in
    san) option=SAN=1;;
    clang) option=CC=clang;;
    inv) option=INV=1;;
  esac
  timeout 900 sh -c 'make clean && make -j2 check "$1"' sh "$option" \
    > "lanes/f-slice5/check-$mode.log" 2>&1
  echo "$mode exit=$?"
  tail -1 "lanes/f-slice5/check-$mode.log"
done
timeout 900 sh lanes/m1-headers/check_headers.sh > lanes/f-slice5/check-headers.log 2>&1
echo "headers exit=$?"
tail -1 lanes/f-slice5/check-headers.log
