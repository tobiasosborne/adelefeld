# Milestone 1 review: text

BLOCKER FOUND: 1 BLOCKER, 6 MAJOR, 3 MINOR.

The blocker follows the literal admission of arbitrary fields in `fball.h:93`. It is a false validator
contract, demonstrated by an invalid read. No enclosure failure was found for valid stored values.
All findings below have a reproducer that was run. Paths beginning `checks/` are relative to this directory.

## Findings

### R1: BLOCKER. The canonicality predicate admits fields that it cannot safely read

Location and requirement: `include/adelefeld/fball.h:92` promises a 0/1 result and says "never aborts,
whatever the fields hold". This overrides the general canonical-input precondition for this predicate.

Input: initialize a finite ball; set backend to LOCAL, A=0, H=2, d=1, res to one live word containing 0,
and mctx to address 1. Call `adf_fball_is_canonical`. `src/fball.c:358` passes mctx to
`adf_modctx_nblocks`; `src/modctx.c:410` reads through it. The process receives SIGSEGV, not 0.

Reproducer: `checks/probe.c`, mode `predicate 0`; `checks/resources.py` runs it with core dumps disabled.
Result: return code -11. The ASan/UBSan build also fails, exit 1, with the invalid read in
`checks/predicate-san.log`. Modes 1, 2 and 3 demonstrate the same storage hazard through the adele,
cadele and scaled predicates; each returns -11.

This is not a memory error on a canonical value with a live context. The explicit arbitrary-fields
promise makes the fball input admitted. Require initialized FLINT fields, live context pointers and
accessible residue storage before these predicates. Apply the clarification to `rat.h:54`, `fball.h:92`,
`adele.h:90`, `scaled.h:76`, and conventions 2.3. A predicate cannot establish pointer validity by reading it.

### R2: MAJOR. Printing an admitted finite ball can terminate the process

Location: `src/text.c:1078`. `include/adelefeld/text.h:27` requires an allocated result; lines 32-34 give
no status channel. `adele.h:109` admits every finite arb to the raw constructor.

Input: exact midpoint 2^(2^64), radius 0, finite coordinate 0, digits=20. The public constructor returns
OK, and both finite and canonical predicates return 1. The printer explicitly calls `flint_abort` because
the dyadic exponent does not fit a signed word. This does not depend on allocation exhaustion.

Reproducer: `checks/probe.c`, mode `print 18446744073709551616`, via `checks/resources.py`.
Result: -6, with "an arb exponent beyond a word cannot be printed exactly".

The same script confirms adf-b8l without exhausting the laptop. Each child has 256 MiB address-space and
20 CPU-second limits. Here e denotes the exponent in the exact input 2^e. Output uses 20 digits.

| e | Wall seconds | Peak KiB | Result |
|---|---:|---:|---|
| 100000 | 0.015 | 9256 | 47 output bytes |
| 1000000 | 0.273 | 9260 | 49 output bytes |
| 10000000 | 4.821 | 18924 | 51 output bytes |
| 100000000 | 20.009 | 129668 | CPU limit, -9 |
| 1073741824 | 0.581 | 136320 | allocation abort, -6 |
| -100000 | 0.017 | 9260 | 49 output bytes |
| -1000000 | 0.280 | 9264 | 51 output bytes |
| -10000000 | 2.042 | 34108 | 53 output bytes |
| -100000000 | 2.749 | 166312 | allocation abort, -6 |
| -1073741824 | 0.616 | 136320 | allocation abort, -6 |

These are bounded measurements, not a claim about the host's unrestricted OOM threshold. The first
unusable tested magnitude under these limits is 10^8. The output remains short while exact rational
expansion and decimal conversion grow with |e|.

Proposed rule: add a checked printer with output byte and binary-exponent limits, returning LIMIT with
both outputs untouched before expansion. An initial exponent limit of 100000 matches the existing driver
rule in `docs/SPEC.md:860`. Check midpoint and radius, and check the final text against the requested
read-back limits. Keep the existing decimal enclosure algorithm inside the admitted range. This requires
an explicit API decision; this review does not change the specification.

### R3: MAJOR. Default parsing is not closed under value printing

Location and promise: `include/adelefeld/text.h:148`; `docs/conventions.md:1275` says reading printed
values encloses their source. The promise needs a limit qualification.

Input: `(9.99e100000 ; 0)`, prec=128, default limits, digits=1. Input status is OK. Output is
`(1e100001 +/- 1.1e99998 ; 0)`. Reading it with the same precision and limits returns LIMIT=10.

Reproducer: `checks/probe.c`, mode `roundtrip`; result is recorded by `checks/resources.py`.
The decimal rounding specified by conventions 9.5 is correct here. The defect is the unconditional
round-trip contract: a value admitted at the exponent boundary can print beyond that boundary.
Require adequate read-back limits, or have the proposed checked printer reject this output explicitly.

### R4: MAJOR. A hidden 18-digit limit overrides the caller's permitted exponent

Location: `src/text.c:661`. Requirement: `include/adelefeld/text.h:63` and
`docs/conventions.md:1066` bound the exponent by max_exp10, a signed 64-bit field.

Input: `(0e1000000000000000000 ; 0)`, max_exp10=1000000000000000000. The literal is within that
limit and denotes exactly zero. C returns LIMIT=10 instead of OK. `proto/text_grammar.py:587` has the
same defect. Differential agreement therefore cannot establish this requirement.

Reproducer: `checks/boundaries.py`. It records C=10, reference=!LIMIT, required=0.
`tests/test_text_adele.c:1400` explicitly expects the undocumented refusal with WORD_MAX as the limit.
That test preserves the defect. Compare digit strings against the actual limit; short-circuit a zero
coefficient before constructing a power of ten. Document a separate resource rule if one is intended.

### R5: MAJOR. Twenty-one declared text operations have no implementation

Location: `include/adelefeld/dump.h:72`. Requirement: `docs/SPEC.md:752` and
`docs/conventions.md:1489` require exported public operations. `docs/PLAN.md:254` includes value text
and dump in milestone 1.

Input: initialize adf_rat to zero and call `adf_rat_dump_str`. Linking fails with an undefined reference.
The same absence affects load_str, load_str_binds, dump_str and dump_inspect for each of rat, fball,
scaled, adele and cadele, plus adf_scaled_get_str. There are 193 declarations and 172 exported functions.

Reproducer: `checks/headers.py`, which generates `checks/missing_link.c`, performs the link, and lists
all 21 missing symbols in `checks/headers.log`. Link exit=1.

The existing context loader is also incomplete. `modctx.h:72` promises inspection of any dump occurrence.
For `adf1 Q fball l 1 6 2 2 3 0 0`, occurrence=0, `src/modctx.c:713` requires the body keyword modctx
and returns PARSE=9. The reference round-trips this dump unchanged. `checks/probe.c`, mode `nested`,
reproduces it. The output remains NULL. This missing portion of work package 1.4 must be implemented.
`tests/test_exports.sh:23` explicitly exempts missing declarations from failure; its pass is not an M1 gate.

### R6: MAJOR. The cap interface promises local inputs that it refuses

Location and requirement: `include/adelefeld/scaled.h:160` describes the cap on plain adf_fball values
and permits OK or DOMAIN for an invalid cap. `fball.h:14` admits both storage backends.

Input: construct context (6), convert `1 mod 6` to that context, and use cap C=1. The input is canonical.
`src/cap.c:62` returns UNSUPPORTED=8. The four combined cap operations do the same for a local operand.

Reproducer: `checks/probe.c`, mode `local`. Five cap statuses are 8. The same local ball prints as
`(* ; 1 mod 6)`, and scaled conversion returns OK with lost=0. Thus the old lack of local accessors no
longer explains this refusal. Implement cap through the canonical triple; preserve the documented interface.

### R7: MAJOR. The promised context-lifetime diagnostic is absent

Location and requirement: `include/adelefeld/modctx.h:19`, `docs/conventions.md:327`.
With ADF_CHECK_INVARIANTS, freeing a context with a live borrower must abort.

Input: build the entire library with that define, construct context (2), initialize a scaled borrower,
then free the context. `src/modctx.c:388` frees it without checking a borrow count.

Reproducer: `checks/probe.c`, mode `lifetime`, linked to `checks/invariants/libadelefeld.a`.
Result: exit 0, `freed_borrowed_context_without_abort=1`. The borrower is cleared without dereferencing
the freed context. This tests the debug-build guarantee, not a claim that this call is valid in release mode.

### R8: MINOR. The header specifies nonexistent FLINT operations

Location: `include/adelefeld/adele.h:145` and `:150` name arb_add_fmpq and arb_mul_fmpq.
Neither exists in the linked FLINT 3.0.1. `src/adele.c:256` and `:283` first enclose the rational at
prec with arb_set_fmpq, then add or multiply. That description also governs the corresponding cadele calls.

Reproducer: `checks/headers.py` lists the actual dynamic symbols; both named operations are absent.
The available conversion is documented at `refs/src/flint-3.0.1/arb.rst:167`; addition and multiplication
are at lines 767 and 798. State the conversion and subsequent rounding in the contract. No numerical
enclosure failure is claimed. The current header literally names two nonexistent functions, not six.

### R9: MINOR. Successful fuzz inputs cannot exercise the advertised precision range

Location: `tests/fuzz/fuzz_text.c:239`. Prec and digits are selected from the same bytes used as text.
An accepted adele or cadele starts with '(' or whitespace and ends with ')' or whitespace.
Consequently successful parses only reach prec={11,12,15,34,42} and digits={3,10,11,12,14}.
They never exercise prec=2 or digits=1, despite the parameter expressions spanning wider ranges.

Reproducer: `checks/boundaries.py` enumerates the 25 legal leading/trailing whitespace combinations and
records those sets. Conventions 9.5 and `text.h:35` cover the missing cases too. Use separate control
bytes, or invoke several precisions and digits for each accepted text. Unit tests cover some missing
values, so this is a fuzz-coverage gap, not evidence that all testing omits them.

### R10: MINOR. Sentinel comparisons inspect uninitialized representation bytes

Location: `tests/test_text_adele.c:420` and `:435`; `tests/fuzz/fuzz_text.c:128`, `:164`, `:203`.
The intended check is the untouched-output rule of `text.h:21` and conventions 4.3. Whole-struct memcmp
also reads padding and unused representation bytes that initialization did not define.

Input: one rejected byte, `@`. `checks/sentinel_padding.c` includes and invokes the exact reviewed
helper. Valgrind reports 2 errors from 1 context, exit 97, with an uninitialized stack origin.
`checks/fuzz_one.c` invokes the actual fuzzer on the same byte: 7 errors from 3 contexts, exit 97.
Both reproducers free every allocated block. The complete adele test reports 2748 errors from 71 contexts.

Initialize the full sentinel storage before its ordinary init, or compare defined fields. Preserve the
untouched-output assertions. These reports originate in the test comparisons, not in the parser's reads
of untrusted text. ASan/UBSan alone does not detect these reads.

## Coverage and checks that found no defect

- Own Fraction oracle: 100000 random decimal texts, 6120 halfway/dyadic boundary cases and 36 cases at
  decimal exponents +/-100000. Total 106156; 0 failures; 3693 exactness checks. Prec covers every integer
  2..256 and selected values through 1024. The 10000 complex cases inspect 5000 real and 5000 imaginary
  coordinates. Radii include zero and values far above or below the midpoint. `checks/oracle.py enclosure`.
- Own printing oracle: 12000 nonzero dyadic balls, 0 failures in exact decimal enclosure and C read-back
  enclosure. Digits include 1, 2, 20, 999999 and 1000000. Every permitted digits value was also tested on
  exact zero: 1000000 cases. This is not an exhaustive nonzero test for every digits value.
- Own grammar generator: 100000 texts spanning all 13 start symbols, whitespace, edits and forbidden
  bytes. No classifier disagreements with proto/text_grammar.py; no status disagreements in 400000
  typed parser calls. The classifier returned 57726 kinds, 39834 PARSE and 2440 LIMIT. Every failed
  typed call checked unchanged sentinel bytes; failed classification checked its untouched kind output.
- Explicit limits and order: 88 checks, 0 failures apart from the separately recorded R4 expectation.
  Included default max_len at 1048576 and 1048577, signed midpoint/radius exponent limits, and pairs of
  length/alphabet/grammar/exponent/zero-denominator faults. max_prec and max_items do not constrain the
  four implemented value parsers; classification correctly ignores them even for later type syntax.
- Local printing: 612 balls, input A=-25..25, raw H=12 and d=1..12. Expected centre was computed as
  ((A mod 12)+12) mod 12 divided by d, and radius as 12/d. All printed templates and read-back sets agreed.
- Four existing text tests with ASan/UBSan: 51 tests, 144843 checks, 0 failures. Counts by executable:
  rat 18122, fball 21545, adele 84400, classify 20776. LeakSanitizer cannot run under this harness's
  ptrace environment; it stopped the first run after rat. The repeated run disabled only leak detection.
- Valgrind: rat, fball and classify each report 0 errors. The adele errors are R10. Definite and indirect
  lost bytes are 0 in all four; rat/fball/adele report possibly lost bytes 135872/139304/135648.
  These possible losses were not resolved. The two minimal R10 reproducers report 0 bytes in use at exit.
- Existing fuzzer with ASan/UBSan: 500269 executions in 21 seconds, 0 crashes, peak RSS 279 MiB.
  The numerical coverage restrictions in R9 remain. All builds and generated corpora are under checks/.

The midpoint algorithm also supports a direct enclosure argument. First, it truncates |m| to t 2^e.
Second, it computes the exact rational error |m|-t 2^e and adds it to r. Third, tx_mag_upper rounds
that sum upward. Thus the stored radius is at least r plus the distance between the old and new
midpoints, which proves inclusion of both endpoints. The independent checks above test that execution
matches this argument, including the exact dyadic cases.

## The ten proposed equivalent mutants

`checks/mutants.py` recreated all ten exact edits from the mutation log. Each compiled and ran 1011
parser comparisons, 1011 classifier comparisons and 270 printer comparisons against the original.
Total: 22920 comparisons, 0 differences. Equivalence is supported by the following case arguments.

| Original line:column, edit | Judgment and reason |
|---|---|
| 100:27, <= to < | Equivalent. Z stops the keyword scan; it remains unconsumed and forces PARSE. |
| 100:53, <= to < | Equivalent. The same argument applies to z. No keyword contains Z or z. |
| 114:18, <= to < | Equivalent. Only a terminal +/- changes; every start symbol still rejects the missing radius. |
| 713:14, 0 to 1 | Equivalent. Only e=0 changes branch; either branch produces num=D and den=1. |
| 753:12, >= to > | Equivalent. At ex=0 both branches copy the numerator and denominator with zero shifts. |
| 753:15, 0 to 1 | Equivalent. Same ex=0 case as the preceding row. |
| 804:22, 0 to 1 | Equivalent. At e=0 both branches copy a and md with zero shifts. |
| 889:11, >= to > | Equivalent. At x=0 the multiplier is 10^0=1 on either side of the comparison. |
| 889:14, 0 to 1 | Equivalent. Same x=0 case as the preceding row. |
| 1096:16, 0 to 1 | Equivalent. Only e=0 changes the conditional's arm; both returned values are 0. |

For each arithmetic row, the changed condition differs from the original only at zero. Substitution of
zero gives identical initialized values in the two branches. For the lexical rows, no new complete
sentence can consume the excluded letter or terminal +/-; the public result remains PARSE.

## Header decisions and sources

Read all public headers and the 15 choices in docs/api-m1.md. Choices 1-7 and 9-15 do not themselves
refute the named conventions. Choice 8 correctly makes the pointer array read-only; a C caller should
still build an array of const context pointers, rather than assume an adf_modctx_struct ** converts to
that nested const type. The implemented declaration gaps are R5, and the cap status gap is R6.

The impossible or unfulfilled promises found are the arbitrary-fields validator guarantee (R1), an
unconditional printable result and read-back enclosure (R2-R3), complete public text exports and dump
inspection (R5), local cap support (R6), debug lifetime checks (R7), and named FLINT calls (R8).
The finite-result claim in `adele.h:26` is already marked unverified; this review did not prove it.

Ground truth read on disk: `refs/src/flint-3.0.1/arf.rst:236` specifies exact dyadic extraction;
`mag.rst:149` specifies an upper bound for mag_set_ui_2exp_si; `fmpz.rst:427` documents integer parsing;
`arb.rst:167`, `:767`, `:798` document rational conversion, addition and multiplication.
The source-pending comments in text.c are partly stale: these documents now exist.

[source pending: exactness of mag_set_ui_2exp_si for every representable 30-bit mantissa].
The local manual promises only an upper bound; probes do not replace a source-level proof of exactness.
[source pending: complete FLINT allocator exhaustion behavior]. `flint.rst:121` and `memory.rst:9`
describe the allocation interface and wrappers, not a complete failure guarantee.

## Findings against the specification and limits of this review

No mathematical formula in docs/SPEC.md was refuted. The unconditional value-form round-trip statement
in `docs/PLAN.md:203` and conventions 9.6 needs a resource-limit qualification, as R3 shows. The public
string interface in `docs/SPEC.md:761` also needs a decision for finite values that the printer refuses
(R2). The arbitrary-fields admission is in the header, not a mathematical requirement of SPEC.
No specification, production source, existing test or header was changed.

Not examined exhaustively: all positive slong precisions, nonzero balls at every digits value, custom
limits demanding enormous allocations, constrained printers for later milestones, all context and ring
arithmetic, or dump implementations that do not exist. The bounded resource tests do not establish an
unrestricted OOM threshold. Header ABI tests were not rerun in C++ or Julia. No 3-minute computation or
more than two concurrent computation processes were used.

Exact commands, outcomes, harness corrections and file inventory are in
`lanes/m1-review-text/report.md`. Raw evidence and all reproducer sources are in `checks/`.
