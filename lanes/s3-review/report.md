# S.3 review report

## Finding

### MINOR: the aliasing assertion accepts a false `NO_SOLUTION`

Input: `c = 3`, `m = 100`, `A = 3`, `B = 3`, `limit = 0`.

The library returns `ADF_OK` with `3/1`. Direct enumeration gives only `3/1`: for `d = 1, 2, 3`,
the required numerator is respectively `3, 6, 9` modulo 100, and only 3 lies in `[-3, 3]`.

In `tests/test_resid.c:792-797`, the aliasing call may return either `ADF_OK` or
`ADF_NO_SOLUTION`. The following comparison repeats the call with separate bounds. A solver
that returns `ADF_NO_SOLUTION` on both calls passes both assertions. This assertion therefore
cannot detect a shared false `NO_SOLUTION` in the aliased and unaliased calls. The grid tests
elsewhere do check this result; the finding is confined to this aliasing test.

Reproduce with the program in this lane:

```sh
cc -std=c11 -O2 -Wall -Wextra -Werror -Iinclude \
  lanes/s3-review/edge_probe.c build/libadelefeld.a -lflint -lgmp -lm \
  -o lanes/s3-review/edge_probe
lanes/s3-review/edge_probe
```

The last line printed is `test_gap: c=3 m=100 A=3 B=3, actual OK 3/1; false NO_SOLUTION
passes both assertions`.

## Work and files

I read `CLAUDE.md`, the named parts of `docs/SPEC.md`, `docs/PLAN.md`, `docs/api-s.md`,
`docs/proofs/solvers.md`, the header, implementation, and both author test files. I wrote
`lanes/s3-review/probe.c` and `lanes/s3-review/edge_probe.c`. This report was written last.
The requested `make -j2` generated the library and object files under `build/`. Probe binaries
were removed after the checks.

## Checks run

- `make -j2`: exit 0; built `build/libadelefeld.a` from 15 C objects.
- Compile and run `probe.c` with `cc -std=c11 -O2 -Wall -Wextra -Werror -Iinclude`, the static
  library, `-lflint -lgmp -lm`: exit 0. It checked 901,872 calls by direct enumeration:
  63,938 `OK`, 218,886 `NO_SOLUTION`, 333,669 `NOT_UNIQUE`, 285,379 `NOT_DETERMINED`;
  0 mismatches. It checked each returned certificate by separate C arithmetic and checked
  `q` unchanged on each non-`OK` status. Limits were `-7, 0, 1, 2, 5, 17`.
- Compile and run `edge_probe.c` by the command above: exit 0. It checked 100,005 certificate
  cases against a separate C predicate, 5 huge-bound calls, 3 alias cases, and 3 invalid
  canonical-form cases; 0 mismatches. The final revision ran twice with the same counts.
- Compile `probe.c` with `-fsanitize=address,undefined` together with `src/resid.c`, then run
  `ASAN_OPTIONS=detect_leaks=1 lanes/s3-review/probe_san`: exit 1. LeakSanitizer reported a
  fatal ptrace incompatibility before reporting leaks. This gives no leak result.
- Run that sanitized `probe_san` with `ASAN_OPTIONS=detect_leaks=0`: exit 0; 901,872 calls,
  0 address or undefined-behavior diagnostics.
- Compile `edge_probe.c` with `-fsanitize=address,undefined` together with `src/resid.c`,
  then run with `ASAN_OPTIONS=detect_leaks=0`: exit 0; 100,005 certificate cases and
  5 huge-bound calls, 0 diagnostics. The final revision ran twice with the same counts.
- `wc -L lanes/s3-review/probe.c lanes/s3-review/edge_probe.c`: longest lines 104 and 105.

For the sanitized builds the full command was:

```sh
cc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
  -fno-omit-frame-pointer -Iinclude lanes/s3-review/probe.c src/resid.c \
  build/libadelefeld.a -lflint -lgmp -lm -o lanes/s3-review/probe_san
```

The same command with `edge_probe.c` and `edge_probe_san` was used for the edge probe.

## Attacks without a library finding

- The 901,872 enumerated calls covered `m = 1` to 18, negative and unreduced input `c`,
  `A = -1` through `m + 2`, `B = -1` through `m + 2`, both signs of `T`, `A = 0`, both
  candidates in one round, and reused certificates. Any wrong status, fraction, certificate,
  or changed `q` on a non-`OK` status would have stopped the program.
- Five targeted calls covered `X` above the word range, `limit = LONG_MAX`, a negative limit,
  a cut search, and a negative `T`. The program required the specified status and fraction.
- The checker probe compared 100,000 random quadruples and 5 deliberate valid or changed
  quadruples with (C1) to (C4). A false acceptance or rejection would have stopped it.
- Setter/getter probes covered invalid `m`, input members used as arguments, swapped output
  members, and manual noncanonical structs. A wrong value or canonical predicate would have
  stopped the program.

## Not done

Leak checking was unavailable because LeakSanitizer failed under ptrace. I did not test
resource exhaustion or a complete run with `limit = LONG_MAX` when the bound would admit
billions of rounds; the header explicitly permits that cost. I did not review functions of
later S.3 slices that are absent from this header and source file.

## Sources pending

None. No formula or convention of another author is quoted here. The expected results above
come from direct enumeration and the repository contract.

## Findings against the specification

None.
