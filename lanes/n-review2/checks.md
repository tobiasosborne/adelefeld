# Commands and results

All commands were run from the repository root. L below abbreviates lanes/n-review2.
Every compiled program and Python script was run under timeout. No repository test suite was run.
The table records both differential runs; the second added the old class printer to the comparison.

## Build and compilation

The one library build was:

```sh
timeout 60 make -j2 BUILD=lanes/n-review2/build > lanes/n-review2/build.log 2>&1
```

Result: exit 0, 29 objects and one archive. The top-level target does not build the driver.

Source extraction, exit 0: 17 renamed symbols and four source copies.

```sh
L=lanes/n-review2
timeout 60 env PYTHONDONTWRITEBYTECODE=1 python3 "$L/prepare_text.py"
```

prepare_text.py ran read-only git show for src/text.c and src/text_idele.c at 674c9db.
It used nm on the two normal printer objects to name the exports. Each child command had a 10-second timeout.
The following four compilations completed and the objects linked into the probes:

```sh
timeout 60 cc -Iinclude -Isrc -std=c11 -O2 -g -c "$L/old_text.c" -o "$L/old_text.o"
timeout 60 cc -Iinclude -Isrc -std=c11 -O2 -g -c "$L/old_text_idele.c" -o "$L/old_text_idele.o"
timeout 60 cc -Iinclude -Isrc -std=c11 -O2 -g -c "$L/review_text.c" -o "$L/review_text.o"
timeout 60 cc -Iinclude -Isrc -std=c11 -O2 -g -c "$L/review_text_idele.c" -o "$L/review_text_idele.o"
```

Printer probe compilation, exit 0. This was run twice; the second version also compared the old class printer.

```sh
timeout 60 cc -Iinclude -std=c11 -O2 -g -Wall -Wextra "$L/printer_probe.c" \
  "$L/old_text.o" "$L/old_text_idele.o" "$L/review_text.o" "$L/review_text_idele.o" \
  "$L/build/libadelefeld.a" -lflint -lgmp -lm -o "$L/printer_probe"
```

Other probe compilations, all exit 0:

```sh
timeout 60 cc -Iinclude -std=c11 -O2 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
  tools/adf/adf.c "$L/build/libadelefeld.a" -lflint -lgmp -lm -o "$L/adf-san"
timeout 60 cc -Iinclude -std=c11 -O1 -g -fsanitize=undefined -fno-sanitize-recover=undefined \
  "$L/edge_probe.c" "$L/build/libadelefeld.a" -lflint -lgmp -lm -o "$L/edge_probe"
timeout 60 cc -Iinclude -Isrc -std=c11 -O1 -g -fsanitize=undefined -fno-sanitize-recover=undefined \
  "$L/precision_probe.c" "$L/build/libadelefeld.a" -lflint -lgmp -lm -o "$L/precision_probe"
timeout 60 cc -Iinclude -Isrc -std=c11 -O2 -g "$L/family_scan.c" \
  "$L/build/libadelefeld.a" -lflint -lgmp -lm -o "$L/family_scan"
timeout 60 cc -Iinclude -std=c11 -O1 -g -fsanitize=address,undefined "$L/leak_probe.c" \
  "$L/build/libadelefeld.a" -lflint -lgmp -lm -o "$L/leak_probe"
timeout 60 cc -Iinclude -std=c11 -O2 -g "$L/double_bound.c" \
  "$L/build/libadelefeld.a" -lflint -lgmp -lm -o "$L/double_bound"
```

The first edge run did not instrument the library. A second, focused reproducer included the three read-only
implementation files and linked the rest from the normal archive. These four compilations returned exit 0:

```sh
timeout 60 cc -Iinclude -Isrc -std=c11 -O1 -g -fsanitize=undefined -fno-sanitize-recover=undefined \
  -DEDGE_LBALL -c "$L/edge_impl.c" -o "$L/edge-lball.o"
timeout 60 cc -Iinclude -Isrc -std=c11 -O1 -g -fsanitize=undefined -fno-sanitize-recover=undefined \
  -DEDGE_DECOMP -c "$L/edge_impl.c" -o "$L/edge-decomp.o"
timeout 60 cc -Iinclude -Isrc -std=c11 -O1 -g -fsanitize=undefined -fno-sanitize-recover=undefined \
  -DEDGE_LFUNC -c "$L/edge_impl.c" -o "$L/edge-lfunc.o"
timeout 60 cc -Iinclude -std=c11 -O1 -g -fsanitize=undefined -fno-sanitize-recover=undefined \
  "$L/edge_probe.c" "$L/edge-lball.o" "$L/edge-decomp.o" "$L/edge-lfunc.o" \
  "$L/build/libadelefeld.a" -lflint -lgmp -lm -o "$L/edge_instrumented"
```

## Executed checks

Python commands below had `env PYTHONDONTWRITEBYTECODE=1` before `python3`.
All checks returned exit 0. The two driver processes returned the expected exit 1 for rejected commands;
their checking scripts returned 0. No timeout expired. No sanitizer diagnostic appeared.

| Command, with L as above | Result and log |
|---|---|
| `timeout 60 python3 "$L/monotonicity.py"` (twice) | 4 assertions, 0 failures; monotonicity.log |
| `timeout 60 "$L/printer_probe" level` | 4 levels; success, failure, success, success; level.log |
| `timeout 60 "$L/printer_probe" diff` (first) | 320 balls, 1280 calls, 0 failures; diff.log |
| `timeout 60 "$L/printer_probe" diff` (expanded) | 320 balls, 1600 calls, 0 failures; diff-expanded.log |
| `timeout 60 "$L/printer_probe" counterexample` | 21-byte idele text, 0 failures; counterexample.log |
| `timeout 60 "$L/printer_probe" family 4000` | prints, 2410 levels, 9642410 work; family-4000.log |
| `timeout 60 "$L/printer_probe" family 5500` | prints, 3314 levels, 18230314 work; family-5500.log |
| `timeout 60 "$L/printer_probe" family 7462` | prints, 4494 levels, 33538722 work; family-7462.log |
| `timeout 60 "$L/printer_probe" family 7463` | NULL, len 0, 4495 formed levels; family-7463.log |
| `timeout 60 "$L/printer_probe" family 8000` | NULL, len 0, 4193 formed levels; family-8000.log |
| `timeout 60 "$L/printer_probe" family 100000` | NULL, len 0, 335 formed levels; family-100000.log |
| `timeout 60 "$L/printer_probe" point 99999 1000000` | 30122 bytes, 2 levels; point-positive.log |
| `timeout 60 "$L/printer_probe" point -99999 1000000` | 69917 bytes, 2 levels; point-negative.log |
| `timeout 20 "$L/printer_probe" mantissa 1000000 1` | 29 bytes, 2000005 work; mantissa-1000000.log |
| `timeout 30 "$L/printer_probe" mantissa 8000000 1` | 30 bytes, 16000005 work; mantissa-8000000.log |
| `timeout 60 "$L/family_scan"` | 2000 inputs, first refusal 7463, 538 refused; family-scan.log |
| `timeout 60 python3 "$L/driver_matrix.py"` | 368 lines, 0 failures, 0 stderr bytes; driver-matrix.log |
| `timeout 60 python3 "$L/driver_limit.py"` | 4 lines, 2 LIMIT, 0 failures; driver-limit.log |
| `timeout 60 "$L/edge_probe"` | 1152 checks, 0 failures; edge.log |
| `timeout 60 "$L/edge_instrumented"` | 1152 checks, 0 failures, 0 UBSan diagnostics; edge-instrumented.log |
| `timeout 60 "$L/precision_probe"` | 36 LIMIT calls, 0 allocations, 0 failures; precision.log |
| `timeout 45 env ASAN_OPTIONS=detect_leaks=0 "$L/leak_probe"` | 3 NULL paths, final_live=0; leak.log |
| `timeout 60 "$L/double_bound"` | 1191 inputs, 4764 bound checks, 0 failures; double-bound.log |
| `timeout 60 python3 "$L/arithmetic_checks.py"` | 5 exact arithmetic checks, 0 failures; arithmetic.log |

The driver scripts each invoke `timeout 45 lanes/n-review2/adf-san` with their generated command file
and ASAN_OPTIONS=detect_leaks=0. The driver is instrumented; its normal library archive is not.
The precision probe instruments the included sball.c and rfunc.c with INV enabled. Its other dependencies
come from the normal archive. The edge_instrumented probe instruments lball.c, lball_decomp.c and lfunc.c.
The leak counter hooks both FLINT and GMP, then clears the objects and calls flint_cleanup before counting.

The first monotonicity log mistakenly labelled four executed assertions as three. Only the printed counter
was corrected, and the script was rerun. No assertion was removed or changed.
An additional `timeout 60 python3` file audit counted 29 build objects and checked proofs.md, checks.md,
and progress.md for lines above 116 characters: 0 such lines in all three files.
