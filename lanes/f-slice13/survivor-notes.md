# Mutation survivors (seed 1, 60 distinct mutants)

The tool compiled 60 mutants: 47 killed, 13 survived, 0 not compiled, 0 tool timeouts.
Replay with strengthened invariant diagnostics compiled the same 13 survivors: 5 killed, 8 survive.
The replay is not a second sweep. It adds no distinct mutants. Full log: survivor-replay.log.

| Index | Change | Final result and reason |
|---|---|---|
| 01 | mul(z,z,t) -> mul(z,t,z) | Survives: commutative; supported alias. |
| 03 | omit binom's INV_FBALL | Killed: invalid input must abort before LIMIT. |
| 16 | mul(t,A,B) -> mul(t,B,A) | Survives: commutative. |
| 22 | signed_powm e<0 -> e<=0 | Survives: at e=0 both a unit and its inverse have power 1. |
| 24 | tight samples start at j=0 | Survives: the extra sample has difference 0 and changes no gcd. |
| 26 | omit fmpz_zero(R) | Survives: R was just initialised to 0 and has not been written. |
| 36 | omit zero residue for modulus 1 | Survives: discarded by gcd(1,*), mod 1 or explicit 1. |
| 38 | omit coarse exponent INV_FBALL | Killed: fallback diagnostic has the wrong function name. |
| 43 | gcd(R,R,b) -> gcd(R,b,R) | Survives: commutative; supported alias. |
| 48 | omit fine base INV_UC | Killed: fallback diagnostic has the wrong function name. |
| 49 | mul(A,A,h) -> mul(A,h,A) | Survives: commutative; supported alias. |
| 51 | omit strict base INV_UC | Killed: fallback diagnostic has the wrong function name. |
| 58 | omit coarse base INV_UC | Killed: fallback diagnostic has the wrong function name. |

Index is the seed-selected mutant number, as used by the kept scratch directory.
No equivalent.txt entry was written: that path is read-only for this lane.
FLINT permits input/output aliases except where stated: refs/src/flint-3.0.1/fmpz.rst:51-55.
