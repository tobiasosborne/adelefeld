# Setup errors

The first benchmark link was attempted before the baseline archive existed.
Command (exit 1):

    timeout 170 taskset -c 2 cc -std=c11 -O2 -g -Wall -Wextra -Iinclude
      bench/bench_roots_real.c lanes/r-cost2/build-before/libadelefeld.a
      -lflint -lgmp -lm -o lanes/r-cost2/build-before/bench_roots_real

The linker reported that libadelefeld.a was absent. The same link on CPU 3 returned 0 after the build ended.

The first mutation-cache make named test_roots, which does not exist. It returned 2 with
"No rule to make target 'lanes/r-cost2/build-mutcache/test_roots'". The corrected build used
real, isolate, padic, bigp, seed and forged root suites and returned 0.

A subsequent cp used a repository-relative destination while already in the mutation mirror.
It returned 1 with "No such file or directory". The script was already written in the correct mirror;
the build continued and returned 0. Copying that script back to the lane returned 0.

The mutation self-test was launched before its scratch-path convention was read.

    timeout 170 taskset -c 3 python3 tools/mutate/selftest.py

It generated build/mutate/selftest outside lane ownership. It was interrupted with Ctrl-C, exit 130.
No self-test completion is claimed. Those generated paths were left for the orchestrator.
The retained paths included live.log, equivalent-live.txt, make/Makefile and make/tests/.
The baseline source snapshot and all mutation scratch trees are under lanes/r-cost2/ instead.
The interrupted self-test also generated tools/mutate/__pycache__/mutate.cpython-312.pyc
(timestamp 03:45 local time). It was left in place. The later replay read that existing cache;
the retained replay script now disables bytecode writes before importing the mutation tool.
