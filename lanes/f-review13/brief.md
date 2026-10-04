# Lane f-review13: referee of the proofs IL1 to IL8 and G7 to G12 (`Log` on ideles), and the tests that cannot fail

Your task is to REFUTE, not to confirm. `Log` on ideles (WP 1F.8, third part) was designed, proved, coded and
tested by one model family (codex `gpt-6.1-sol`: design lane d-idlog, code lane f-slice11); you are of another
family on purpose. The bug hunt f-review10 (`docs/reviews/f1/review-gfunc-log.md`) compared VALUES and statuses
with its own oracle (1060 local images as sets, 700 CRT refinements, 52 driver lines): no finding. Do NOT repeat
that. Nobody has read the proofs as a referee, and nobody has planted faults against the tests. That is your
task.

Under review: `docs/design/idele-log.md` (IL1 to IL8, the interface, the oracle's proof, 679 lines);
`docs/api-1f8.md` lines 274-475 (G7 to G12); `proto/idlog_checks.py` (the oracle); `include/adelefeld/gfunc.h`
(the six `Log` declarations), `src/gfunc_log.c` (373 lines); `tests/test_gfunc_log.c` (562 lines),
`tests/driver/gfunc-log-*`, `tests/julia/gfunc_log.jl` if present. Contract: `docs/SPEC.md` 9.3.2 (lines
574-600) and 15.4 (N-D17); `docs/proofs/functions.md` Lemma 9, Propositions 10, 11, 12, 22;
`docs/proofs/ideles.md` Definition 4, Lemmas 5 to 7; `docs/conventions.md` 3.1 to 3.3, 4.3, 5.6 to 5.9.
`refs/src/` is on disk.

**You own:** `lanes/f-review13/` only. Everything else is read-only. No git command that changes state, no
`bd`. At most 2 cores. Build the archive ONCE with `timeout 600 make -j2 BUILD=lanes/f-review13/build
lanes/f-review13/build/libadelefeld.a`; single test programs into the same directory (`make -j2
BUILD=lanes/f-review13/build lanes/f-review13/build/test_gfunc_log`). Do not run the repository's suites as a
whole. Every program under `timeout`; none over 180 s.

## Part 1: referee IL1 to IL8 and G7 to G12 as proofs

Read each statement and its proof line by line, as a referee who is paid for each false step. For each: the
claim in your own words; every step checked; for each step that is not immediate your own derivation, or a
counterexample computed in exact integers by a program in your directory (state the precision of every
comparison and why it suffices). Points to press, among others you find:
- IL1 (the local input set): the component at `p | N`, at `p` not dividing `N`, the exact units, and the
  prime 2 under the normal form of unit cosets (`N != 2 mod 4`): is each case a consequence of Lemma 5 as
  quoted? Is `v_2(N) = 1` really excluded, for every input the header admits (also after arithmetic)?
- IL2, IL3: "the image is `Log(r c) + p^k Z_p`": equality of sets or only inclusion? Which part of `r`
  contributes (`Log(p) = 0`)? At 2: `k = 2`, `k >= 3`, `c = -1 mod 4`, the kernel `{1, -1}` of `Log`.
- IL4: surjectivity of `log` from `1 + p Z_p` onto `p Z_p` and from `1 + 4 Z_2` onto `4 Z_2`: proved or
  cited (file and line)? `Log(r')` lies in `p Z_p`: for negative `r'`, for `r'` with `p - 1` not dividing
  the order of its residue?
- IL5: the passage from the local images to one finite ball: the CRT step, the modulus, "`0 + 4 Zhat`
  conservative also for exact inputs" (N-D17): is the claimed refined ball the SMALLEST finite ball that
  contains the image over the named primes, and is it claimed to be?
- IL6, IL7: the oracle: does the enumeration modulo `p^H'` cover every unit class that matters (prove or
  refute the bound on `H'`); the tail bound of the truncated series, in particular at `p = 2, 3` and for the
  route `log(u^(p-1)) / (p-1)`; are the "independent routes" independent?
- IL8: is the oracle function's expected ball the statement of IL2 to IL5 or the code's formula restated?
- G7 to G12: each sentence the code must keep (working precision `K = min(N, E)`, limits, preservation of
  outputs on failure, `where`, the order of statuses in the preflight, the bounded intersections, the
  rounding at an exact zero): find one the code does not keep, by reading `src/gfunc_log.c` against it and
  by a program. G12 names what is NOT proved: is anything the header promises left unproved?
A step that is true but not proved as written is a MINOR with the missing argument supplied; a false step
with a counterexample input to the LIBRARY is a BLOCKER or MAJOR by its consequence; a false step the code
does not rely on is a MINOR.

## Part 2: tests that cannot fail

Plant faults in a scratch copy of `src/gfunc_log.c` under your build directory: your OWN faults, at least 14,
different from the eight of the design's test plan (`docs/design/idele-log.md` section 4; read which those
are). Among them: the exponent `v_p(N)` off by one in each direction; `v_p(r)` with the wrong sign; the
cofactor `r'` replaced by `r`; the case `p = 2`, `v_2(N) = 2` treated as `>= 3`; the image at an
unrestricted odd prime given as `p^2 Z_p`; the real coordinate `log |t|` where `Log` must refuse a negative
ball; `K = N` instead of `min(N, E)`; one status of the preflight swapped with the next; the output written
before the last failure check; `where` set on `OK`; the CRT centre not reduced; a named prime that divides
neither `N` nor `r` skipped. Build `test_gfunc_log` against each, run it, and record which fault it detects,
also for the driver fixtures if they can be run alone. A fault that every test passes is a finding (MAJOR if
the fault gives a wrong enclosure or status, MINOR otherwise) with the smallest input that distinguishes it.
Read the fixtures' generator too: which stored rows are computed by the same formula as the code (they guard
against regression only)?

## Report

`lanes/f-review13/result.md`, written once, at the end (the harness refuses the name `report.md` for a Claude
subagent); running notes in `lanes/f-review13/progress.md` as you go (a note after every statement refereed:
if the session is cut off they are the report). For each finding: severity, the proof step or the input, what
the text or the code says, what is true and why, the command that reproduces it. Then, statement by
statement, what you checked and how (counts, precisions, what would have made a case fail). Then the fault
table of part 2. No praise, no summary. Give the same text as your final message. Delete your build tree and
executables at the end.
