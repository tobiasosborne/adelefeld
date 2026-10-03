# d-idlog report

Completed 2026-10-04. Design and exact-integer oracle only. No C implementation or header was changed.
No git command or bd was run. At most two test processes ran together. Every program had timeout <= 120 s.

## Statements proved

- IL1: restricted input factors are r c + p^(m+k) Z_p; unrestricted odd factors are shells; at 2 they are balls.
- IL2: restricted odd-prime Log image is Log(r' c) + p^k Z_p; the prime-free content contribution is retained.
- IL3: at 2 the image is Log(r' c) + 2^k Z_2 for k >= 2 and 4 Z_2 for k = 0 or 1.
- IL4: unrestricted factors map onto p^d Z_p; exact units give singleton Log(r') with either sign.
- IL5: the finite image lies in D and 4 Zhat; finite integral refinements give a global ball by integer CRT.
- IL6: enumeration modulo p^H determines and covers every output residue at H; H'=H for the tested grids.
- IL7: degrees below 2H suffice; work at H+floor(log_p(2H-1)) permits exact modular term division.
- IL8: the oracle returns canonical centre/exponent fields, handles exact zero, and supplies CRT conditions.

IL8 is a contract justified by IL1-IL7 and checked against the complete finite-quotient enumeration.
The proofs are numbered steps in docs/design/idele-log.md. They do not infer surjectivity from inclusion.
For the surjectivity step they use the inverse bijections proved in functions.md Lemma 9:265-294.

## Files written

- docs/design/idele-log.md: proofs, six proposed declarations, statuses, limits, test plan, five TJO points.
- proto/idlog_checks.py: standard-library integer oracle, expected_ball and expected_refinement, CLI and fixtures.
- lanes/d-idlog/progress.md: incremental proof and run notes.
- lanes/d-idlog/fixtures.jsonl: 21880 rows, 3158687 bytes; integer local expectations.
- lanes/d-idlog/check_artifacts.py: syntax, line-length and fixture-field checks.
- lanes/d-idlog/artifact-check.log: final artifact check output.
- lanes/d-idlog/oracle-H5.log and oracle-H5-final.log: complete H=5 runs.
- lanes/d-idlog/oracle-H6.log and oracle-H6-final.log: partial expanded runs stopped by timeout.
- lanes/d-idlog/oracle-H6-grid.log and oracle-H6-tail.log: completed H=6 groups in separate commands.
- lanes/d-idlog/report.md: this report, written once at the end.

lane.log and stdout.log were present when work started; this lane did not edit them directly.

## Oracle runs and numbers

The full grid has 4280 finite-precision input cases and 160 exact-unit cases. For each p it uses
m=-2,-1,0,1,2, four prime-free contents, k=0,1,2,3,4, three cofactors of the modulus, and valid residues.
The H=5 image group has 1080 cases at 2, 1140 at 3, 940 at 5, and 1120 at 7.
Their enumerated unit counts are 9440, 61200, 581000 and 3457160 respectively.
At H=6 those counts are 18880, 183600, 2905000 and 24200120 respectively.
All image and component comparisons use H'=H and compare sets for equality.

| Group | H=5 | H=6 |
|---|---|---|
| Component set equalities | 4280 | 4280 |
| Image set equalities | 4280 | 4280 |
| Unit residues in the image enumeration | 4108800 | 27307600 |
| Capped precision checks | 34240 | 34240 |
| Factor-2 equivalent input pairs | 1644 | 1644 |
| Exact-unit cases | 160 | 160 |
| Exact-zero cases / sign pairs | 40 / 80 | 40 / 80 |
| Kernel unit residues / output classes | 17084 / 3115 | 113860 / 20191 |
| Independent odd-prime torsion lifts | 17068 | 113828 |
| Independent 2-adic square routes | 16 | 32 |
| Isometry differences | 68320 | 569268 |
| CRT cases / membership comparisons | 120 / 91160 | 120 / 91160 |
| Finite mutants rejected, at H=5 | 8 of 8 | 8 of 8 |

The requested cap exponents are -2,0,1,2,3,4,H,H+2. An image comparison fails for any missing or extra
residue, or an output count other than p^(H-E). At N >= E the ball must equal the image set;
at N < E it must contain that image and have exactly exponent N. A field error, torsion-route
disagreement, isometry failure or CRT membership disagreement also fails a check.
The eight planted finite-rule mutations each change a residue set. They are not C mutation tests.

All runs, including intermediate versions:

1. Initial oracle, before expanded kernel checks:

   `timeout 120 python3 -B proto/idlog_checks.py --H 5 --fixtures lanes/d-idlog/fixtures.jsonl`

   Exit 0. 4280 image equalities, 4108800 units, 34240 caps, 1644 factor-2 pairs,
   160 exact cases, 17084 kernel units, 16 square routes, 120 CRT cases, 91160 memberships,
   8 mutants rejected, 21880 fixture rows. Its kernel did not yet check torsion lifts or isometry pairs.

2. Initial H=6 oracle, before expanded kernel checks:

   `timeout 120 python3 -B proto/idlog_checks.py --H 6 > lanes/d-idlog/oracle-H6.log`

   Exit 0. 4280 image equalities, 27307600 units, 34240 caps, 1644 factor-2 pairs,
   160 exact cases, 113860 kernel units, 32 square routes, 120 CRT cases, 91160 memberships,
   8 mutants rejected. This log was overwritten by run 4; its output was observed and recorded in progress.md.

3. Expanded kernel, H=5:

   `timeout 120 python3 -B proto/idlog_checks.py --H 5 > lanes/d-idlog/oracle-H5.log`

   Exit 0. All H=5 groups in the table, including 17068 torsion lifts and 68320 isometry differences.

4. Expanded kernel, H=6, concurrent with run 3:

   `timeout 120 python3 -B proto/idlog_checks.py --H 6 > lanes/d-idlog/oracle-H6.log`

   Exit 124. Components and images completed: 4280 equalities each, 27307600 units, 34240 caps.
   The remaining groups did not complete in this invocation. No all-groups success is claimed.

5. H=6 after eliminating repeated construction of coarse cap sets:

   `timeout 120 python3 -B proto/idlog_checks.py --H 6 > lanes/d-idlog/oracle-H6-final.log`

   Exit 124. Components and images completed with the same counts as run 4; remaining groups unfinished.
   Full-image set equality was retained. Cap membership and the exact exponent were retained.

6. H=5 after the cap optimisation and distinct positive/negative-valuation finite mutants:

```text
timeout 120 python3 -B proto/idlog_checks.py --H 5 --fixtures lanes/d-idlog/fixtures.jsonl \
  > lanes/d-idlog/oracle-H5-final.log
```

   Exit 0. All H=5 groups in the table. 21880 regenerated fixtures: 21400 finite-precision rows,
   480 exact-input rows, including 120 exact-zero rows.

7. Remaining expanded H=6 groups, without reducing their grids:

```text
timeout 120 python3 -B proto/idlog_checks.py --H 6 --checks factor_two exact kernel crt faults \
  > lanes/d-idlog/oracle-H6-tail.log
```

   Exit 0. All H=6 groups from factor-two through CRT in the table; 8 finite mutants rejected at H=5.

8. Component and image groups after visiting congruence progressions directly and caching cap moduli:

```text
timeout 120 python3 -B proto/idlog_checks.py --H 6 --checks components images \
  > lanes/d-idlog/oracle-H6-grid.log
```

   Exit 0. 4280 component equalities, 4280 image equalities, 27307600 unit residues, 34240 cap checks.
   Every expanded H=6 group completed across runs 7 and 8. No full expanded H=6 exit 0 is claimed.

9. Expected-ball CLI witness:

   `timeout 10 python3 -B proto/idlog_checks.py --expect 3 4 1 1 9 5`

   Exit 0. p=3, exact=false, center=3, exponent=2, image_exponent=2.

## Other checks run

One initial standard-library check used this command and body:

```text
timeout 10 python3 -B - <<'PY'
from pathlib import Path
for name in ('docs/design/idele-log.md', 'proto/idlog_checks.py', 'lanes/d-idlog/progress.md'):
    lines = Path(name).read_text().splitlines()
    bad = [(i, len(line), line) for i, line in enumerate(lines, 1) if len(line) > 116]
    print(name, 'lines=', len(lines), 'over_116=', len(bad))
    for row in bad:
        print(*row)
compile(Path('proto/idlog_checks.py').read_text(), 'proto/idlog_checks.py', 'exec')
print('syntax_checks=1 errors=0')
PY
```

Exit 0. At that revision: design 658 lines, oracle 461 lines, notes 55 lines;
0 lines above 116, 1 syntax check, 0 errors.

The artifact script ran twice with the same command:

`timeout 10 python3 -B lanes/d-idlog/check_artifacts.py > lanes/d-idlog/artifact-check.log`

Both exits 0. First run: design 658, oracle 461, notes 63, artifact script 53 lines.
Final run: design 679, oracle 463, notes 78, artifact script 53 lines; 0 lines above 116.
Both runs: 2 syntax checks, 0 errors; 21880 fixture rows parsed, 21400 finite-precision inputs,
480 exact inputs, 120 exact-zero fields, 4 primes, 5 content valuations, 0 canonical-field errors.
All content, expected centre and exponent checks use integer arithmetic.

## Recommended interface in ten lines

1. Append the six proposed declarations to gfunc.h only in a later authorised implementation lane.
2. adf_idele_Log returns an adf_adele with real log(I) and finite ball 0 + 4 Zhat.
3. adf_idele_log_abs uses real log(abs(I)) and the same finite Iwasawa enclosure.
4. adf_idele_Log_at returns an adf_sball over one named place, with local images IL2-IL4.
5. adf_idele_log_abs_at is real-only; at a prime return UNSUPPORTED, as rfunc.h does.
6. Log_refine and log_abs_refine retain all places and store named finite refinements by integer CRT.
7. At primes use K=min(N,E), E=max(k,d); exact inputs use N or proved exact zero in _at.
8. A negative I gives DOMAIN at infinity for Log; excessive real prec gives LIMIT there first.
9. Use compact local descriptors; defer an exact public projection until it can return odd-prime shells.
10. Bound CRT at 2^26 bits and the named list at 65536; preserve outputs on failure and combine statuses.

## Not done

No C implementation, public header, C test, driver command, golden driver file or Julia test was written or run.
The Julia example and driver commands in the design require the future API. The eight faults were planted in
finite mathematical expectations, not compiled library code. No differential fuzz run was performed.
The proposed status and resource contracts have not been exercised against an implementation.
TJO has not recorded the five recommendations in SPEC 15.4. The specification was not edited.
No claim that a nonzero rational Log value is impossible at rational input was proved or needed.

## Findings against the specification

0 counterexamples to SPEC 9.3.2:598-600, functions.md Proposition 12:386-406 or ideles.md Lemma 5:94-108.
The statements needed for this design were proved. There is no requested change to SPEC.

The draft's unrestricted-prime hull objection cannot be proved as written at 2:
lanes/f-slice10/brief-draft.md:170-175 and :298-306 need a prime-2 exception.
Counterexample: r=1, c=1, M=1. X_2=Z_2^x=1+2 Z_2 is an additive ball, and the hull has this
same 2-factor. It excludes zero and its Log image is 4 Z_2, so lball_Log is determined.
At an unrestricted odd prime the objection is correct: the shell contains p^m and 2p^m,
forcing any additive hull to contain zero. This is a finding against the draft, not SPEC or Lemma 5.

The normal-form assertion k=1 cannot occur at 2 is true. It does not describe every valid stored input:
(c,M)=(1,2) is canonical, is kept by constructors, and represents the same set as (1,1).
The design and oracle handle it rather than rejecting it (conventions.md:601-608, CV-17).

## Sources pending

- The conventional name and normalisation of Iwasawa Log, already pending in functions.md:343.
- The name Teichmueller representative, already pending in functions.md:101.
- Real intermediate value and real/complex exponential normalisations, pending in functions.md:40-50.

The finite proofs use the explicitly defined series and proved identities, so the two pending names
are not premises. arb_log's API description was read in refs/src/flint-3.0.1/arb.rst:1050-1058;
it does not supply the real analytic theorem missing from Lemma 2. All other external facts used
are cited by refs/ file and line in the design. No source was fetched or installed.
