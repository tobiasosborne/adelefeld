# Milestone 1 dump review

NO BLOCKER FOUND; 0 BLOCKER, 4 MAJOR, 0 MINOR.

R1 (MAJOR): the dump constructor bypasses the 65,536-block context cap.
Location: `src/dump.c:591`, `src/modctx.c:552`. Reproducer: `checks/cost.py 65537` and `checks/bridge.c`.
Command: `python3 -B docs/reviews/m1/dump/checks/verify.py cost`.
Input: `adf1 Q modctx K 10001 q_1 ... q_65537`, where the blocks are the first 65,537 primes and K
is their product, all in hexadecimal. The script generates the exact 682,067-byte input.
Required: UNSUPPORTED, without constructing the context, by `docs/SPEC.md:864` (M1-D5) and
`include/adelefeld/modctx.h:23`. Actual: OK, returning a context with 65,537 blocks after 65.005279 CPU
seconds. The scaled inspector also accepts the same occurrence; validation took 1.708779 CPU seconds.
The parser checks only `max_items`, whose default is 1,048,576. There is no cap check before validation
or allocation. The constructor then calls the raw allocator directly. The cap is not applied at any stage.

R2 (MAJOR): an undocumented qclass exponent limit rejects a valid context occurrence.
Location: `src/dump.c:713`, `src/dump.c:722`; the test at `tests/test_dump_ctx.c:896` expects this limit.
Reproducer: `checks/status_findings.py`, run through
`python3 -B docs/reviews/m1/dump/checks/verify.py unit`. Output is in `checks/verified-status.log`.

Input: `adf1 Q qclass pieces 1 1 1 -100001 0 0 l 1 2 1 2 0`, occurrence 0, default limits.
Required: OK under `include/adelefeld/modctx.h:89`, `docs/conventions.md:681`, `:1061`, and `:1422`.
Actual: LIMIT; the context output stays untouched. Changing the exponent to `-100000` returns OK.

The validity proof is direct:

1. The real midpoint is `2^(-1048577)`, strictly between 0 and 1; its radius is 0. The mantissa is odd.
2. The finite part is `(0 + 2 Zhat)/1`, with canonical triple `(0, 2, 1)`.
3. There is one piece and one valid context, so ordering imposes no further condition. No limit in 8.4
   bounds this binary exponent.

The reference-only bound in `proto/text_grammar.py:1102` has become a C API
restriction. It also runs among semantic checks: denominator 0 returns DOMAIN before this limit, while
a negative midpoint returns LIMIT before its DOMAIN check. This contradicts the staged policy in 8.5.

R3 (MAJOR): the dump fuzz target can abort on a valid seed because it reads an unset length.
Location: `tests/fuzz/fuzz_dump.c:108`, `:125`; the same expression occurs at `:145`, `:177`, `:202`, `:221`.
Reproducer: `checks/fuzz_driver.c`, compiled with GCC at O0 and run under Valgrind by
`python3 -B docs/reviews/m1/dump/checks/verify.py memory`.

Input: `adf1 Q rat 1 1`. Required: the successful round trip in `include/adelefeld/dump.h:38` completes;
`tests/fuzz/fuzz_dump.c:26` claims to check that property. Actual: SIGABRT, with one Valgrind error for
an uninitialized value from `load_one`, used by `same_bytes` at line 95.
The expression `same_bytes(adf_rat_dump_str(&tl, x), tl, s, n)` does not ensure the dump writes `tl`
before the length argument is read. The reproducer includes the unchanged fuzz target.
This is a harness failure, not a library crash. The lane's reported fuzz count does not resolve it.

R4 (MAJOR): dump whitespace is rejected at the alphabet stage, before an unsupported header.
Location: `src/dump.c:1038`. Reproducer: `checks/status_findings.py`, `checks/verified-status.log`.
Input bytes: `adf2 Q rat\t1 1` and `adf1 R rat\n1 1`, with default limits.
Required: UNSUPPORTED. `docs/conventions.md:1043` includes TAB, LF and CR in the alphabet;
`:1048` and `:1324` exclude them from dump syntax. Section 8.5, `:1081`, checks the header before body
grammar; `:1429` requires UNSUPPORTED for another version or field. `include/adelefeld/dump.h:24`
adopts this order. Actual: PARSE, with outputs untouched. The control input containing a NUL instead
returns PARSE correctly, since NUL is an alphabet error. The Python reference repeats the early rejection
at `proto/text_grammar.py:1020`; agreement with that reference cannot establish this contract.

The reviewed surface is `src/dump.c`, `adf_modctx_new_from_dump` in `src/modctx.c`, the three dump test
files, and `tests/fuzz/fuzz_dump.c`. I read both headers, the named SPEC, PLAN and conventions sections,
the dump reference, golden vectors, and the m1-dump and m1-modctx-b reports. No implementation was changed.

The completed checks found these properties correct within the stated coverage:

- `test_dump`, `test_dump_ctx`, `test_dump_golden`: 30 tests, 41,487 checks, 0 failures. Golden coverage
  is 165 rows, including 76 typed rows and 23 valid typed rows. The same tests pass with ASan and UBSan.
- `roundtrip.c`: 6,000 public-API round trips, 27,029 checks, 0 failures. Inputs use global constructors,
  `adf_fball_set_local`, local addition, multiplication and negation, scaled conversion and arithmetic,
  exact scaled values, and real and complex balls. Loading uses the original context pointer. The test
  destroys the source text before testing the current public `is_canonical`, `identical`, and dump bytes.
  It also checks missing, NULL, reordered and wrong-modulus bindings, SIZE_MAX counts, equal contexts at
  different addresses, and inspector capacities 0, 1 and 2 with an untouched trailing descriptor.
- `bridge.c`: 561 full or truncated inputs end immediately before a protected page; no NUL is supplied.
  There are 0 contract errors. A FLINT counting allocator records 223 calls and 0 retained allocations
  after cleanup. Valgrind reports 0 errors and 0 live bytes for this check and for the 6,000 round trips.
  The page cases include rejected residues, scales, rational denominators and real mantissas, and accepted
  real exponents beyond a machine word. ASan and UBSan also complete both check programs with exit 0.
- `differential.py`: seed 2026092807; 150,000 texts, exactly 10,000 generated for each of the 15 bodies.
  It generates grammar productions and mutates tokens, bytes, counts and limits. All 50,000 typed
  inspector results match the reference. The five typed loaders have 0 predicate, unchanged-output or
  byte-round-trip errors. There are 24,150 independent semantic checks using Python integers, gcds and
  range tests, with 0 mismatches. The independent predicates cover both valid and mutated texts; the
  project reference supplies syntax recognition only for selecting those checks.

The differential command exits 1 and is not reported as passing. Its 28 mismatches are all constructor
results for `char` with modulus `40000001`: Python returns UNSUPPORTED and C returns DOMAIN.
`proto/text_grammar.py:782` refuses character moduli above 100,000 as a reference limitation (`:19`).
These bodies contain no context, so C's DOMAIN is consistent with the constructor contract. This is not
another C finding. The script retains the failing assertion and logs every mismatch.

The existing local and scaled tests construct their inputs field by field (`tests/test_dump_ctx.c:10`).
Their helper predicates and identity tests do not establish integration with the current type APIs.
The 6,000 round trips above supply that missing coverage without changing those tests. The qclass limit
test noted in R2 encodes an implementation restriction absent from the contract. The two test files
reported as written after implementation cannot establish the claimed red-green process retrospectively.

The FLINT citations checked against files under `refs/` support exact dyadic conversion
(`refs/src/flint-3.0.1/arf.rst:227`, `:236`, `:411`), integer string conversion
(`refs/src/flint-3.0.1/fmpz.rst:337`, `:427`, `:600`), word remainder (`:845`), and CRT (`:1279`).
The serialization description is at `refs/src/flint-3.0.1/arb.rst:286`. The general CRT precompute
documentation is now present at `refs/src/flint-3.0.1/fmpz.rst:1354`; that part of the older lane's
pending-source statement is stale. The predicate comments were checked against conventions 5.1 to 5.5
and 5.14, including raw local data with no gcd condition (CV-55).

Not examined: a complete proof over all input strings, allocation-failure injection, thread safety,
other context constructors, or arithmetic enclosure correctness outside dump restoration. The typed
API has at most one occurrence; repeated multi-occurrence bindings cannot be exercised through it.
The project tests exercise multi-occurrence qclass extraction, but no qclass value loader exists here.
The independent grammar generator is not a replacement for a repaired coverage-guided fuzz campaign.

The cost search also tested valid contexts at the 65,536-block cap. It squares an initial segment of the
first 65,536 primes, leaving the others unsquared, and chooses the largest segment fitting the byte limit.
With 41,931 squared blocks, the modctx dump has 1,048,571 bytes and loads in 59.495496 CPU seconds.
With 28,367 squared blocks, a one-piece qclass dump has 1,048,574 bytes and loads in 68.804485 CPU seconds.
The qclass midpoint is 1/2, radius 0, denominator 1 and all residues 0. It adds CRT work during validation.
This is the slowest admitted input found here, not a proof of the most expensive possible input.
These are single CPU-time measurements on the shared laptop, not benchmark medians. Every measured call
finished within 69 CPU seconds; the longest complete generation-and-check command took 134.982010 seconds.

Sources pending: [source pending: a FLINT 3.0.1 source under refs stating the exact MAG_MAN/MAG_EXP
representation used by src/dump.c:1276]. The mag documentation at `refs/src/flint-3.0.1/mag.rst:6`
states 30 mantissa bits but does not specify that field equation. Also
[source pending: the C language text under refs for evaluation order of function arguments].
R3 is confirmed by the unchanged-target executable and Valgrind, independently of that missing citation.

Findings against the specification: no mathematical counterexample to SPEC was found. R1 violates M1-D5;
R2 and R4 violate the documented loader contract. The reference repeats the restrictions behind R2 and R4.

Commands were run from the repository root. Let `P=docs/reviews/m1/dump/checks`:

- `python3 -B "$P/verify.py" build`: run twice; both make -j2 invocations and all six compilations exit 0.
- `python3 -B "$P/verify.py" unit`: three project tests and two review programs exit 0. The status
  diagnostic exits 0 while printing 4 expected/actual differences in 7 cases, confirming R2 and R4.
- `python3 -B "$P/verify.py" diff`: run twice; the underlying differential command exits 1 both times
  for the 28 reference-limit mismatches. The second run adds the independent and typed checks above.
- `python3 -B "$P/verify.py" memory`: both library Valgrind runs exit 0; fuzz-driver compilation exits 0;
  the valid fuzz seed ends with signal 6 and one uninitialized-value error, confirming R3.
- `python3 -B "$P/verify.py" san`: make -j2, two compilations and five executables exit 0. Runs use
  `ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1`; leak checking is supplied by Valgrind.
- `python3 -B "$P/verify.py" cost`: the 65,537-block case exits 0 but returns the wrong OK status (R1).
- `python3 -B "$P/verify.py" near-modctx` and `near-qclass`: both underlying commands exit 0;
  their byte counts and CPU times are recorded above.
- `python3 -B "$P/finish.py"`: exit 0; removes 2 build directories and 7 standalone binaries;
  0 binaries remain. Sources, scripts and logs are retained.

The phase runner records child statuses and does not turn finding reproducers into passing assertions.
All 31 expanded commands, exit statuses and elapsed times are in `checks/commands.md` and
`checks/verified-commands.jsonl`. Full outputs are the `checks/verified-*.log` files; runtime output for
bridge and roundtrip is in the sanitizer and Valgrind logs. Earlier unprefixed logs were already present
and are not used as evidence for this review. The reviewed file hashes are in `checks/reviewed-files.sha256`.
The cleanup script was syntax-checked with Python ast.parse: 0 syntax errors. Final document and artifact
checks are recorded in the lane report.
