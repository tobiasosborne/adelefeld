# Proofs of the independent oracles

These arguments describe the constructions in oracle.py. They do not use the author's reference,
normalisation routine, powering, gcd or root finder to decide the expected answer. Python integer
arithmetic performs every modular computation of the oracle. The C bridge proves each selected prime
with the public place constructor; the Python probable-prime function is only a candidate filter.

## A power test that excludes a quadratic root

1. For a nonzero a modulo a prime p, multiplication by a permutes the p - 1 nonzero residues.
2. Multiply all of those residues before and after that permutation. Their product is nonzero, so it
   can be cancelled. This gives a^(p - 1) = 1 modulo p.
3. The script chooses c with c^((p - 1)/2) = -1 modulo p by Python pow. If c = a^2 modulo p, step 2
   would give c^((p - 1)/2) = a^(p - 1) = 1. Since p is odd, 1 != -1. This is a contradiction.
4. Therefore X^2 - c has no root modulo p. A root in Z_p would reduce to a root modulo p, so there is none.

## Products and coefficient perturbations

1. Over F_p a product is zero exactly when one factor is zero. The monic product of the planted linear
   factors, times the quadratic of the previous argument, has exactly the planted residues as roots.
   Repeating a factor does not add a root.
2. Multiplication by a nonzero residue does not change this set. Adding a multiple of p to any integer
   coefficient does not change the reduction. The 4096-bit signed offsets and the appended leading
   coefficients divisible by p therefore preserve the exact modular root set.
3. For the independent Hensel cases, the planted residues are distinct and appear only once. At each
   residue the derivative of the product is a product of nonzero factors. Thus it is nonzero modulo p.
   Coefficient perturbations by p, including the higher leading term, preserve that derivative value.

## One-digit lifting and completeness in Z_p

1. Suppose a_e is a root modulo p^e and f'(a_e) is nonzero modulo p. For a digit t in [0, p), expand
   f(a_e + p^e t) modulo p^(e+1). The terms of order at least two are divisible by p^(2e), hence by p^(e+1).
2. The next digit must satisfy f(a_e)/p^e + t f'(a_e) = 0 modulo p. The script computes the unique t
   using Python pow(f'(a_e), -1, p). It checks the resulting polynomial value modulo p^(e+1).
3. The successive a_e are compatible and converge to a root in Z_p. They give its residue at every
   requested precision. Every root of f in Z_p must reduce to a modular root from the constructed set.
4. If alpha and beta are two such roots with the same residue, their difference has valuation at least
   one. Taylor expansion factors f(alpha) - f(beta) as (alpha - beta) times
   f'(beta) + (alpha - beta) Q, with Q in Z_p. The second factor is a unit. Since Z_p has no zero divisors,
   alpha = beta. Thus there is exactly one root above each constructed simple residue, and no others.
5. The output must consequently be the entire constructed list of centres modulo p^prec, with K = prec
   and s = 0. A short list, an extra ball, a wrong centre or a wrong precision fails the comparisons.

## Roots of X^k - 1

1. Put d = gcd(k, p - 1). For any nonzero root x of X^k - 1, x^k = x^(p - 1) = 1. An integer Bezout
   identity for k and p - 1 gives x^d = 1; negative exponents are defined because x is nonzero.
2. Hence there are at most d roots: every one is a root of X^d - 1, a polynomial of degree d over a field.
   This bound follows by dividing out a linear factor for each distinct root, one at a time.
3. The script constructs w = g^((p - 1)/d) modulo p. It checks w^(d/q) != 1 for every prime divisor q
   of d. Also w^d = 1 by the first argument. These checks force the order of w to be exactly d:
   a proper divisor order would divide d/q for some such q.
4. The powers 1, w, ..., w^(d - 1) are distinct, and each is a root of X^k - 1 since d divides k.
   There are d of them. Step 2 therefore makes this the complete list.

## Close integer roots, content and repetition

1. In Q_p a nonzero scalar times a product of linear factors vanishes exactly at the planted integers.
   The additional quadratic has no root by the first argument. The factor p X - 1 has no root in Z_p,
   since p times an integral element cannot be 1. Thus these factors add no integral root.
2. Each distinct linear factor occurs once in the normalised polynomial. The derivative at a root r
   is the product of r - r' over the other roots, times the additional factors at r. Those additional
   factors are units modulo p. Its valuation is therefore the sum of the valuations of r - r'.
3. The two-pair construction has roots -1, -1 + p^3, 2, 2 - p^2, 5. Only the first pair and second pair
   agree modulo p. Their derivative valuations are 3, 3, 2, 2, 0. Digits first separate at levels 3 and 2.
   A complete result has centres r modulo p^max(prec, s + 1) and precisely these s values.
4. The repeated/content construction has roots -1, 0, p^2 - 1, p - 2. Only the first and third agree
   modulo p; their derivative valuations are 2, 0, 2, 0. Its duplicate first factor must set reduced = 1.
   All factors are primitive: modulo any prime their reductions are nonzero, and their product is
   nonzero over that field. Multiplying by -p^3 2^2048 is only content and sign; it does not change roots.
5. At a common next digit of two roots, the class polynomial has a multiple linear factor modulo p.
   At the first separating digit it has simple factors. Thus depth below the largest common-digit
   valuation leaves the corresponding classes unresolved, while that depth suffices to resolve all roots.
   Every planted root must appear in exactly one ball or class, and every reported ball must hold one
   planted root. The partial checks test those conditions directly.
