# f-review12 progress

- Built libadelefeld.a (plain) and a SAN=1 INV=1 copy; harness h.c (stdin protocol), h2.c (idele vector), h3.c
  (statuses, aliasing, untouched, local ball, identities); Python oracles orc.py (binom), orc2.py (small profpow,
  exhaustive per prime power), orc3.py (N up to 2500 bits, known factorisation), orc4.py (cyclo), orc5.py (big tight).
- No finding. Two oracle bugs of mine found and fixed (k=0 conservative radius; canon of v_2(F)=1 in the N branch).
- Driver: 101 lines through an ASAN build, values checked by hand against the library harness.
- Cleanup done: build trees and executables deleted.
