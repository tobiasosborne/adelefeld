#!/bin/sh
set -e
timeout 180 python3 -B lanes/f-slice5/faults.py > lanes/f-slice5/faults-final.log 2>&1
timeout 60 cc -std=c11 -O2 -g -Wall -Wextra -Werror -Iinclude lanes/f-review3/probe.c \
  lanes/f-slice5/build/libadelefeld.a -lflint -lgmp -lm -o lanes/f-slice5/build/probe
timeout 900 sh lanes/f-slice5/oracle_runs.sh > lanes/f-slice5/oracle-final.log 2>&1
