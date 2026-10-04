# Mutation survivor dispositions

Each item identifies the run, original line and mutation. Line numbers are those in the mutation logs.

- A:82, drop fmpz_gcd(g,H,d): test gap. New mixed-domain triple (1,3,2) kills it.
- A:131, drop ADF_INV_FBALL(a) in Jacobi: test gap. Invalid input plus even lower entry kills it.
- A:108, symbol kind 1 -> 0: equivalent; both kinds use Jacobi after the same positive-odd validation.
- A:125, ball_symbol kind 0 -> 1: equivalent; the supplied lower entry is already an odd prime.
- A:44, modulus guard && -> ||: test gap. Positive odd finite Kronecker rows kill it.
- B:283, (u*u-1) -> (u*u+1): equivalent; odd squares are 1 mod 8, so integer quotients are identical.
- B:287, sign multiplication -> division: equivalent; the divisor is +1 or -1.
- B:282, (w-1)/2 -> w/2: equivalent; w is positive and odd.
- B:246, v<=WORD_MAX-3 -> v<WORD_MAX-3: test gap. Relative precision 3 at N=WORD_MAX kills it.
- B:227, u<8 -> u<=8: equivalent; the loop visits only 1,3,5,7, then 9.
- B:284, last exponent addition -> subtraction: equivalent; the sign uses only parity, including negative odds.
- B:332, rational sign<0 -> sign<1: equivalent; exact zero was rejected, so signs are only -1 and +1.
- B:261, cofactor-sign multiplication -> division: equivalent; the Legendre denominator is +1 or -1.

Generic sampled run totals: A 30 samples, 25 compiled; B 30 samples, 30 compiled. Four gaps were
resolved by targeted replay, without rerunning the generic sampler or adding excuses to the tracker file.
