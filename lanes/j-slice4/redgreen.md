# J-a.4 red-green record

All gates use timeout 200 sh lanes/j-slice4/gate.sh, with an internal timeout 180.
One Julia thread, one precompile task, offline mode, lane temporary files and depot with trailing colon.
The system GMP is preloaded before Julia starts.

| Log | Exit | J-a.1 | J-a.2 | J-a.3 | J-a.4 pass/fail/error/total |
|---|---:|---:|---:|---:|---|
| before.log | 0 | 904 | 1015 | 1649 | absent; baseline 3568 |
| red.log | 1 | 904 | 1015 | 1649 | 26/15/39/80 |
| green1.log | 1 | 904 | 1015 | 1649 | 1341/3/0/1344 |
| green2.log | 0 | 904 | 1015 | 1649 | 1364/0/0/1364; total 4932 |
| final.log | 124 | no completed summaries | | | 180 s timeout; unverified |
| final2.log | 0 | 904 | 1015 | 1649 | 1313/0/0/1313; total 4881 |

The tests were appended before the implementation. Red had missing functions/enum and digits keywords.
The implementation was written only after that run finished. The first implementation run found three
oracle errors: the FBall/Adele/CAdele typed loaders see a rat body as PARSE, before checking its fields.
The correction tests a non-canonical body of each requested type for DOMAIN. Its gcd condition fails.
No status or identity assertion was weakened.

Counted-call and enum-layout fixtures were added after green1. They test all four dump finally paths,
including NULL, non-NULL with zero length, and a missing NUL; all four loader contexts; digits/precision
guards before any C call; both text-kind layout gates before Rat init.

After green2, the identity oracle gained a same-text/different-balls self-check at default digits.
The earlier no-digits-keyword decisions of api-ja.md finding 5 and step J-a.2 were then restored for
Rat/FBall. The first implementation had unnecessarily widened those exact printers. They now retain
the original signatures and exact output. The digits matrix and counted guards apply to the four
numerical types; the test still proves 64 ArgumentErrors and 0 C printer calls on invalid digits.
The exact types explicitly assert MethodError on a digits keyword and unchanged IOContext output.
This corrects scope, not a mathematical test or a C status contract.

The first final gate ran alongside the small driver comparison and reached 180 s without a completed
summary. No passing count is assigned to it. A solo gate is required after that timeout. The implementation
and the timeout remain unchanged. The cause was not isolated.
The solo rerun passes 4881/4881. Its counted-call subprocess passes 43/43 more checks, outside that count.
It prints DUMPS=16 FREES=16, CONTEXTS=0, DIGIT_ERRORS=64 PRINT_CALLS=0, and
PREC_ERRORS=5 LIMIT_CALLS=0 CLASSIFY_CALLS=0, with LIVE=0. The 10000-round-trip subprocess also ends
with LIVE=0 ERRORS=0.
