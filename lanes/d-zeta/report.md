# Report, lane d-zeta

## Work completed

- Z1: finite meromorphic continuation, complete simple pole set, residue 1/log(p), and no regular zeros.
- Z2: real continuation, complete simple pole set, residues 2 (-1)^n pi^n/n!, and no regular zeros.
- Z3: exact real pole classification; finite dyadic classification conditional on Gelfond-Schneider.
- Z4: finite midpoint certificate and real integer geometry cannot return OK on a pole-containing ball.
- Z5: explicit separation/error sufficient condition; no unsupported universal prec or guard-bit guarantee.
- Z6: rigorous derivative and whole-ball variation bounds at both places.
- Z7: near-pole size, extreme finite half-plane bounds, and independent lower image-width witnesses.
- Z8: Gamma reference from an integrated Taylor polynomial with two proved tails and recurrence.
- Z9: independent reference values, status simulation, sample containment, and factor-64 width checks.

Gamma continuation and the absence of zeros use the checked theorem in
refs/src/tate-poonen/notes.txt:62-64. This closes catalogue Proposition 9's stated Gamma source gap.
The finite dyadic classification still has a pending theorem source. No counterexample to that claim was found.

## Files written

- docs/design/local-zeta.md: 603 lines; proofs, declarations, choices, test plan, user calls and five TJO points.
- proto/zeta_checks.py: 589 lines; expected(place,s_mid,s_rad,prec), reference certificates and checks.
- lanes/d-zeta/zeta-fixtures.jsonl: 1377 rows, 11162934 bytes, including 5845 sampled point references.
- lanes/d-zeta/progress.md: 40 lines, including failures and repairs.
- lanes/d-zeta/report.md: this report, written after the work and checks.

No C implementation or header was changed. No git state-changing command, bd, installation or agent spawn was run.
An inline import generated one oracle .pyc file outside the owned paths; that incidental file was removed.
The last cleanup check found 0 such bytecode files. Existing lane logs were not edited.

## Recommended interface, ten lines

1. adf_complex_local_zeta_at(acb_t y, adf_place_t *where, const acb_t s, adf_place_t v, slong prec).
2. Dedicated local_zeta.h; raw acb input and output, with the same complex spectral parameter at every place.
3. One place-dispatched function; use existing real and word-prime handles, with every prime below 2^64.
4. Composite/0/1 prime construction is DOMAIN; a text prime above one word is UNSUPPORTED before a call.
5. Working bits max(2,prec)+32; prec>2097152 gives LIMIT first; no fixed relative-accuracy promise.
6. Recognised exact pole gives DOMAIN; mixed pole ball or failed certificate gives NOT_DETERMINED.
7. Stable finite exponent sign and midpoint variation; real integer geometry, Gamma, then recurrence if needed.
8. Recurrence fallback at most 64 factors, otherwise LIMIT; bounded midpoint/derivative refinement at infinity.
9. Temporaries preserve y on failure and permit y=s; failure writes where=v, OK preserves where, NULL is valid.
10. Defer public entire reciprocals and finite products; products and normalisations belong with milestone 5.

## Final oracle run

Command:

```text
timeout 120 /usr/bin/time -f 'elapsed_seconds=%e peak_kib=%M exit=%x' \
  python3 proto/zeta_checks.py --fixtures lanes/d-zeta/zeta-fixtures.jsonl
```

Exit 0; 108.59 seconds; peak 41896 KiB; one FLINT thread.

| Check | Count/result |
|---|---:|
| Finite poles k=-3..3 at six primes, and their residues | 42 |
| Real poles 0,-2,...,-40 and their residues | 21 |
| Independent Gamma integral reference checks | 7 |
| Fixed-input precision cases | 12: 6 refusals at 16 bits, 6 OK at 256 bits |
| Status fixtures | 1377 |
| OK | 701 |
| NOT_DETERMINED | 642 |
| DOMAIN | 32 |
| LIMIT | 2 |
| Independently certified sampled values contained by simulated output | 5845, 0 failures |
| Positive lower image-width witnesses and factor-64 diameter checks | 643, 0 failures |

The fixtures include radii 10^-j for j=1,4,12,30,60, both finite stable signs, closed pole boundaries,
degenerate rectangles, wide pole-free refusals, regular complex boxes, real parts +/-2^1000, and Im(s)=2^1000.
Membership of each nonzero finite pole in its around/segment fixture is certified with scalar arb intervals.

Failure conditions: wrong status; wrong pole membership; reference error above its certified margin;
variation exceeding R B plus point errors; a missed sampled value; an accepted non-finite output;
missing positive width witness; or output diameter above 64 times the certified lower image diameter.
Reference error allowance is 10^-200 max(1,|point|), independently checked, with an additional
10^-229 max(1,|point|) for exported decimal formatting. Residue comparison margin is
10^-70 max(1,|residue|), also checked against independent interval references.
The numerical periodicity residual test uses 10^-230. It is not substituted for a pole-membership certificate.

This was a deterministic fixture run. It was not a long differential fuzz run or a test of adelefeld C code.

## Every other check run

The commands below were run in this lane. Repeated commands are listed with their separate outcomes.
The `python3 -` commands were inline diagnostic scripts; the tested expressions are named in the result.
Read-only rg, sed, cat, ls and source discovery commands are not test runs.
P below means `timeout 15 python3 -`; B means `timeout 15 python3 -B -`;
L means `timeout 120 python3 -`. F is:

```text
timeout 120 python3 proto/zeta_checks.py --fixtures lanes/d-zeta/zeta-fixtures.jsonl
```

| Command | Result |
|---|---|
| timeout 15 python3 -c (import/version/context probe) | exit 0; mpmath 1.3.0, Python FLINT 0.8.0, 1 thread |
| P (arb API probe) | exit 0; 16 methods inspected, exact man_exp conversion inspected |
| timeout 120 python3 proto/zeta_checks.py --quick, first run | exit 1; 63 pole checks, 1 Gamma error assertion |
| timeout 15 python3 - (Gamma(1) point enclosure/norm probe) | exit 0; finite reference 1, norm NaN 1 |
| timeout 15 python3 - (sign-indefinite and positive arb square) | exit 0; 2 inputs; 1 general-power NaN |
| timeout 120 python3 proto/zeta_checks.py --quick, repaired | exit 0; 63 pole checks and 7 Gamma checks |
| F, run 1 | exit 1; reference margin failure 1 |
| same fixture command, run 2 | exit 1; 870 rows written; unexpected real Gamma refusal 1 |
| timeout 15 python3 - (four Gamma/rgamma boxes) | exit 0; Gamma finite 2/non-finite 2; rgamma finite 4 |
| P (shift/reflection diagnostic) | exit 0; shifted candidate finite 1; reflected candidate non-finite 1 |
| P (three negative Gamma/shift points) | exit 0; direct finite 2/non-finite 1; shifted finite 3 |
| timeout 15 python3 - (nine negative Gamma boxes) | exit 0; direct finite 3/non-finite 6 |
| same fixture command, run 3 | exit 1; 1324 rows written; integer string export limit failure 1 |
| timed fixture command above, first complete run | exit 0; 1349 rows, 5719 samples, 630 width witnesses; 35.44 s |
| L (real simulation width/containment audit) | exit 0; 1890 containment checks, 0 misses; 4/210 width failures |
| timeout 15 python3 - (intersection/union API probe) | exit 0; 2 methods present; default prec 53 |
| L (four width cases and derivative alternative) | exit 0; 4 cases; alternative ratios 13.9089..15.0278 |
| timed fixture command, refined complete run | exit 0; rows 1349, samples 5719, widths 630; 32.01 s |
| timeout 15 python3 - (real parts +/-2^1000) | exit 0; 2 OK statuses |
| timed fixture command above, added-case run | exit 1; huge-phase certificate mismatch 1; 35.63 s |
| timeout 15 python3 - (first line-length check) | exit 0; Markdown overlength lines 1, Python/progress 0 |
| timeout 15 python3 -B - (incidental bytecode cleanup) | exit 0; removed files 1 |
| timeout 15 python3 -B - (syntax/line check) | exit 0; compile checks 1; overlength lines 0 in 3 files |
| B (fixture/syntax/line audit) | exit 0; rows 1377, samples 5845, widths 643, compile checks 1 |
| timeout 15 python3 -B - (python-flint version probe) | exit 0; Python 0.8.0 runtime FLINT 3.3.1 |
| timeout 15 python3 -B - (incorrect ctypes version-symbol probe) | exit 139; signal 11; no oracle value used |
| timeout 15 python3 -B - (corrected char-array version probe) | exit 0; system FLINT 3.0.1 |
| timeout 15 python3 -B - (last line/cleanup audit) | exit 0; lines 603/589/40; overlength 0/0/0; bytecode files 0 |
| B (first report line audit) | exit 0; overlength report lines 9 |
| B (report shortening audit) | exit 1; overlength report lines 1 |
| B (final report line audit) | exit 0; overlength report lines 0 |

The first complete run checked 420 finite output diameters and generated 210 real width witnesses.
The later independent real audit checked those 210 real diameters and exposed four failures.
The refined and final complete runs assert diameter and sample containment at both places.

## Failures and repairs

- General arb power on a sign-indefinite interval produced NaN for exponent 2. Added an endpoint square.
- At 240 mpmath digits a near-pole reference lost too many digits: error 4.4669964e-135 against 1.3818273e-147.
  Raised reference precision to at least 320 digits and added phase-dependent guard digits.
- Direct Gamma on [-1.05,-0.95]+i[0.75,0.85] was non-finite at 288 bits despite exclusion of every Gamma pole.
  Added a bounded recurrence fallback through the positive half-plane. The input and expected OK were retained.
- Fraction export reached Python's 4300-digit string limit. Exported exact dyadic bound endpoints instead.
- Four real diameter ratios were 65.9042, 118.8617, 177.9176 and 279.6214. Midpoint/derivative intersection
  repaired them. The factor 64 and the original input balls were retained.
- At Im(s)=2^1000 the scalar expm1 formula lost phase correlation. Intersected its denominator with 1-exp.
  The pole-free input and expected OK were retained; all final cases pass.
- A ctypes metadata probe treated a declared char array as a pointer and exited with signal 11.
  Read refs/src/flint-src-3.0.1/flint.h.in:105, corrected the type, and obtained version 3.0.1 with exit 0.

## Not done and limits of the evidence

- No C implementation, driver command, Julia execution, implementation alias/sentinel test, or C mutation was run.
  The design gives the eight required planted faults and proposed calls for that lane.
- The full nonzero finite dyadic classification remains conditional on the pending Gelfond-Schneider source.
- Python-flint uses FLINT 3.3.1; the system and repository target are 3.0.1. The real candidate simulation,
  direct-Gamma refusals, fallback LIMIT triggers and last radius bits are version-specific evidence.
  Recheck them on 3.0.1 in the implementation lane. The reference values never use complex Gamma as an oracle.
- Scalar arb supplies the interval arithmetic for the analytic certificates. The on-disk contracts are 3.0.1;
  the corresponding runtime 3.3.1 source contract is still pending. Do not describe this as a 3.0.1 numerical run.
- Certified reference scope: real points -64<=Re(s)<=64 and |Im(s)|<=64; finite points |Im(s)|<=2^1024.
  An out-of-scope reference raises an oracle error. It does not fabricate an adelefeld status.
- No universal prec threshold or global factor-64 output-width guarantee is asserted.
  The sufficient certificate includes measured enclosure error; factor 64 is the implementation fixture target.

## Sources pending

1. Gelfond-Schneider theorem, including every value of alpha^beta;
   needed only for Z3's finite dyadic classification.
2. FLINT 3.0.1 acb/arb exponential and Gamma evaluator source and range guards; absent from the local snapshot.
3. Checked FLINT 3.3.1 scalar arb enclosure contracts, matching the installed oracle runtime.

## Findings against the specification

No counterexample to SPEC 9.3.7, catalogue Proposition 9 or analysis Proposition 10 was found.
Catalogue Proposition 9's Gamma continuation obligation is closed by notes.txt:62-64.
Recurrence alone does not establish Gamma's absence of zeros; the checked theorem supplies it.

The local-factor row in conventions.md:223 omits LIMIT. The recommended precision and fallback caps require
that status. This is an interface reconciliation point for TJO, not a change made by this lane.
The premise of a finite arf exponent range is false: refs/src/flint-3.0.1/arf.rst:39 explicitly excludes
overflow and underflow of that representation. Non-finite evaluator output remains NOT_DETERMINED under CV-08.
