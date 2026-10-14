# Measurements before changes

All benchmark commands use `timeout 170 taskset -c 2`. Times are wall seconds.
The table is incremental. Completed medians use five serial runs on CPU 2.
Raw runs are in before.jsonl. The first random run overlapped a profiling run.
The early sqrt2, xn2 at 16384, and first xn2 at 65536 overlapped the last cluster runs.
Those rows are excluded from final comparisons and will be repeated serially.

| Input | prec | Before median s | Runs |
| --- | ---: | ---: | ---: |
| dense random, degree 50, 10000 bits, seed (1,2) | 2 | 18.647433 | 5 |
| prod (X - 2^190 - i), i=1..50 | 2 | 0.070947 | 5 |
| prod (X - i), i=1..50 | 2 | 0.014045 | 5 |
| prod (X - i 2^150), i=1..50 | 2 | 2.962556 | 5 |
| prod (2^150 X - i), i=1..50 | 2 | 0.110470 | 5 |

One preliminary scratch profile of X^50 - 2 at 16384 bits (overlapped; no timing comparison):
34 QIR attempts, 98 exact value evaluations, 76 sign calls, 52 exact sign fallbacks.
The refinement already doubles the QIR exponent after a success.
This count suggests measuring filter cancellation before changing the refinement sequence.

Serial scratch profiles (one run each; times are not five-run medians):

| Input | prec | Count s | Isolation/refinement s | Certificate s | Complete verifier s |
| --- | ---: | ---: | ---: | ---: | ---: |
| cluster, e=190, d=50 | 2 | 11.885964 | 0.151242 | 0.014082 | 13.916625 |
| random, bits=10000, d=50 | 2 | 20.135522 | 1.619963 | 0.019296 | 23.194535 |
| X^50 - 2 | 262144 | 0.000007 | 25.729195 | 13.582454 | 6.749715 |

X^50 - 2 at 262144 used 42 QIR attempts, 130 exact values, and 68 exact sign fallbacks.
The coefficient translation cost 0.000511 s; it is done once per initial cell.
The endpoint certificate already exceeds 10 s here. It is unchanged by this lane.

Red regression: `timeout 170 taskset -c 3 lanes/r-cost1/build-red/test_roots_real_isolate`.
15 tests, 19244 checks, 1 failed check. The new test saw 98 exact evaluations against a bound of 16.
The exact ball oracle passed. The shell command also printed the log tail, so the shell's final status
was 0; the test program's own summary records the assertion failure.
