#!/bin/sh
# The four remaining checks of the brief, one after the other. Logs in lanes/f-repair1/.
R=lanes/f-repair1
timeout 900 make clean >/dev/null 2>&1
timeout 900 make -j2 check SAN=1 >$R/check-san.log 2>&1; echo "san exit=$?" >>$R/summary.log
timeout 900 make clean >/dev/null 2>&1
timeout 900 make -j2 check CC=clang >$R/check-clang.log 2>&1; echo "clang exit=$?" >>$R/summary.log
timeout 900 make clean >/dev/null 2>&1
timeout 900 make -j2 check INV=1 >$R/check-inv.log 2>&1; echo "inv exit=$?" >>$R/summary.log
timeout 900 sh lanes/m1-headers/check_headers.sh >$R/check-headers.log 2>&1; echo "headers exit=$?" >>$R/summary.log
echo finished >>$R/summary.log
