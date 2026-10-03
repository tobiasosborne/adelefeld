# Design of Log on ideles (1F.8, before code)

Lane d-idlog, 2026-10-03. This is a proposal for gfunc.h beside the two f-slice10 slices.
No declaration below is implemented. SPEC is unchanged. TJO decides the five points in section 6.

## 1. Meaning and sources

Write x = (I, r, c U(M)), with r a positive rational. M denotes the unit modulus throughout this document.
N denotes requested absolute output precision. At a prime p put m = v_p(r), r' = r / p^m, k = v_p(M)
when M > 0, and d = 2 at 2, d = 1 at odd p. The real interval I excludes 0. Its coordinates are independent.
M = 0 means the exact diagonal unit c = +1 or -1, not an unrestricted unit group.

Repository contracts read: SPEC 5:315-326, 9.3.1:557-573, 9.3.2:577-600, 15.4:925-932;
PLAN:278; conventions 2.1:118-122, 3.1:176-192, 3.3:234-247, 4.1:251-260,
5.6:584-608, 5.7:623-648, 5.8:670-691, 5.9:693-707. Representation contracts:
idele.h:16-23, ucoset.h:12-20, idmap.h:46-53, lball.h:11-17, lfunc.h:15-65,
sball.h:139-150, rfunc.h:93-123, fball.h:122-137 (all headers in include/adelefeld/).
Proofs read: ideles.md Definition 4, Lemma 5, Proposition 6 and Lemma 7:87-145;
functions.md Proposition 4:92-113,
Lemma 9:265-294, Propositions 10-12:299-406 and 22:725-766; api-1f4.md F2, F3, F6:87-175.
The style follows api-1f4.md and api-1f6.md. References here are to the files read for this design.

External ground truth read on disk:

- refs/src/tate-poonen/notes.txt:1603-1605 identifies Zhat with the product of Z_p.
- refs/src/tate-kudla/kudla-1.txt:79-86 describes Z_p by digits and units by their nonzero first digit.
- refs/src/baker-padic/padicnotes.txt:1182-1184 gives density of integers in Z_p;
  :1287-1291 gives unique p-adic expansions; :374-387 gives the integer Chinese remainder theorem.
- refs/src/flint-3.0.1/arb.rst:597-600 says arb_is_nonzero excludes zero;
  :606-609 says arb_is_finite checks finite midpoint and radius; :1050-1058 describes arb_log.
- refs/src/flint-3.0.1/padic.rst:494-507 describes FLINT's series log and its narrower domain at 2.
  This is a second opinion, not the library's definition or evaluator (N-D9).

The local log identities and inverse maps used below are proved in functions.md Lemma 9, not quoted
from memory. The name and normalisation of Iwasawa Log remain pending there (:343), as does the
name Teichmueller (:101). Here Log is defined by the displayed decomposition; neither name is a proof premise.
The real analytic facts remain those explicitly pending in functions.md Lemma 2:40-50.

### IL1. The local input set

For M > 0 its projection X_p is:

| Case | Exact projection as a set | Representation |
|---|---|---|
| k >= 1 | r c + p^(m+k) Z_p = r c (1 + p^k Z_p) | adf_lball, centre r c, exponent m+k |
| k = 0, odd p | r Z_p^x = p^m Z_p^x | multiplicative shell; no adf_lball represents it |
| k = 0, p = 2 | r Z_2^x = r + 2^(m+1) Z_2 | adf_lball, centre r, exponent m+1 |
| M = 0 | {r c}, c = +1 or -1 | exact rational adf_lball |

Proof.

1. Lemma 5:96-108 of ideles.md says that at p | M the unit factor is exactly c + p^k Z_p.
   It is all units at p not dividing M. Its factors are independent. Any chosen p-coordinate extends
   to a unit lift by choosing c at the other restricted primes and 1 elsewhere.
2. For k >= 1, c is prime to p. Write r = p^m r' with r' a unit. Multiplication by r' maps Z_p
   onto itself. Hence r(c + p^k Z_p) = r c + p^(m+k) Z_p. Dividing its error by r c gives
   1 + p^k Z_p. Its centre has valuation m and exponent m+k > m, so it excludes zero.
3. For k = 0, multiplication by r' permutes units. This gives p^m Z_p^x, regardless of c.
   The integer c can even be divisible by this p; it is not a local unit representative there.
4. At an odd p the shell contains p^m and 2 p^m. Their difference has valuation m. Any additive
   ball containing both has exponent at most m and contains zero, which the shell excludes.
   An exact rational singleton also cannot represent this shell.
5. At 2 every unit is odd. Thus Z_2^x = 1 + 2 Z_2, and r Z_2^x = r + 2^(m+1) Z_2.
   This ball excludes zero. The exception at 2 is essential.
6. For M = 0 the definition in ucoset.h:13-14 is equality to c, so its projection is {r c}.
7. Lemma 7:134-142 of ideles.md removes the factor 2 when v_2(M) = 1. Normal form has k = 0
   or k >= 2 at 2. Constructors still admit k = 1 (conventions:601-608). Then c is odd and
   r c + 2^(m+1) Z_2 is the same shell as in step 5. Handle it without rejecting the input.

Check: oracle check_components; check_factor_two. The additive hull of idmap.h:46-53 is usable at
every restricted prime and at 2. At an unrestricted odd prime it contains zero. Calling lball_Log
on that hull gives NOT_DETERMINED, although Log on the true shell has a known image.

### IL2. The exact image at restricted odd primes

For odd p and k >= 1:

    Log(X_p) = Log(r c) + p^k Z_p = Log(r' c) + p^k Z_p.

Proof.

1. Proposition 4 and Proposition 11:341-364 of functions.md define x_p = p^m w u and
   Log(x_p) = log(u). Multiplication of the unique factors and Lemma 9 give
   Log(a b) = Log(a) + Log(b). Therefore Log(p^m) = 0 for every integer m, also negative.
2. IL1 writes X_p = r c (1 + p^k Z_p). Log(r c) = Log(r') + Log(c), since p^m contributes zero.
   The prime-free part r' can have a nontrivial principal-unit factor. It must not be discarded.
3. Lemma 9:267-294 proves inverse bijections exp and log between p^k Z_p and 1 + p^k Z_p,
   for every k >= 1 at odd p. Log agrees with log on this unit group. Thus its image is onto,
   not merely contained in p^k Z_p.
4. Translate that image by Log(r c). This proves equality. Equivalently apply Proposition 11
   with input exponent m+k: its relative exponent is (m+k)-m = k.
5. For example, at 3 take r = 4, c = 1, M = 9. Then Log(r) = log(4) has valuation 1 by
   Lemma 9's leading-term calculation. Modulo 9 it is 3. The image is 3 + 9 Z_3, not 9 Z_3.
   This follows directly from the series: 3 - 9/2 + 27/3 and every later term reduce to 3 modulo 9.

Check: oracle check_images compares complete residue sets, including this translation witness.

### IL3. The exact image at 2

For p = 2 the image is Log(r' c) + 2^k Z_2 if k >= 2. For k = 0 or k = 1 it is 4 Z_2.

Proof.

1. For k >= 2, repeat IL2 with the bijection between 1 + 2^k Z_2 and 2^k Z_2 from Lemma 9.
   The sign factor w = +1 or -1 is fixed on the input ball but has logarithm zero.
2. For k = 0 or 1 the local input is the shell 2^m Z_2^x by IL1.
   Proposition 4:108-110 writes every odd unit uniquely as w u, w in {+1,-1}, u in 1 + 4 Z_2.
3. Log kills w and 2^m. Lemma 9 maps 1 + 4 Z_2 onto 4 Z_2. Therefore the shell image is
   exactly 4 Z_2. If using the ball form of IL1 its relative exponent is 1, which is the special
   row of SPEC:582 and Proposition 11:348, not the ordinary exponent rule.
4. Normalising k = 1 to k = 0 changes no set. A valid unnormalised input has the same output.
   For k = 2 the translated coset also equals 4 Z_2 since Log(r' c) belongs to 4 Z_2.

Check: oracle check_images includes k = 0 through 4; check_factor_two compares stored pairs.

### IL4. The exact image at unrestricted primes and exact units

For M > 0 and p not dividing M:

    Log(r Z_p^x) = Log(r') + p^d Z_p = p^d Z_p.

For M = 0 the image is the singleton {Log(r c)} = {Log(r')}.

Proof.

1. The unique decomposition and definition put Log(a) in p^d Z_p for every nonzero a.
2. Every y in p^d Z_p is log(exp(y)) by Lemma 9. The point exp(y) is a unit. Hence Log of
   all units is onto p^d Z_p, giving equality, also at 2 where d = 2.
3. Additivity gives the first translated image. Log(r') belongs to p^d Z_p, so translating
   this additive group by Log(r') leaves it unchanged. This is why its centre can be zero.
   It does not assert Log(r') = 0. IL2's r' = 4 at 3 gives a nonzero example.
4. Exact units give one local point by IL1. Log(-1) = 0 at every p, so both signs have the
   same finite logarithm. It is exactly zero when r is a power of p (including r = 1).
   Conversely zero forces r' to be a rational root of unity, hence +1 as r > 0: if a rational
   a/b in lowest terms has a positive integral power equal to 1, |a| = b and |a/b| = 1.
   This is also the elementary proof in api-1f4.md F2:91-98.

Check: oracle check_images; check_exact; check_kernel. These cover negative content valuations.

### IL5. All finite places, and a finite ball of refinements

Let J_p be the exact images in IL2-IL4. For M > 0 the finite image set is product_p J_p.
For M = 0 it is the tuple of local singleton values. In either case it lies in

    D = 4 Z_2 x product over odd p of p Z_p, contained in 4 Zhat.

The unrefined answer 0 + 4 Zhat has the valid global fball triple (A,H,d) = (0,4,1).
It is an enclosure, generally neither the image nor its smallest global ball. In particular,
for M = 1 the image is D, which has strictly smaller factors at every odd prime.

Proof.

1. For M > 0 Lemma 5 gives independent input factors. Surjectivity in IL2-IL4 supplies a
   preimage for each selected local output. Their unit factors form a member of c U(M);
   multiplying by the fixed r gives a finite idele. Thus the product is the image itself.
2. For M = 0 there is only one finite input, the diagonal rational r c. Its image is the
   tuple of its local Log values, which need not be a diagonal rational.
3. Each J_p lies in p^d Z_p by the definition of Log, so all coordinates are integral and
   form a finite adele in D. At odd p, 4 is a unit and 4 Z_p = Z_p; at 2 its factor is 4 Z_2.
   This proves Proposition 12:402-405 and SPEC:598 for whole represented sets as well as points.
4. No positive-radius global ball can equal D or the image product for finite M: outside the
   finite supports of its centre, radius and M, its projection is Z_p while J_p = p Z_p.
   An odd prime outside any finite list exists by taking a prime divisor of one plus its product.
   A zero-radius global ball is a rational singleton and cannot equal the nonsingleton product.
5. Let S be finitely many named primes. Let B_p = b_p + p^K_p Z_p enclose J_p there.
   The literal enclosure "4 Zhat refined at S" is

       C_S = {z in 4 Zhat : z_p in B_p for every p in S}.

   It retains integral unlisted coordinates and the factor 4 at 2. It does not claim the
   stronger p Z_p conditions at all unlisted odd primes. An sball over S alone holds only
   product_(p in S) B_p; it has no coordinates or baseline outside S.
6. Put beta_2 = 2 and beta_p = 0 for odd p. Put L_p = max(beta_p, K_p) for p in S.
   When K_p <= beta_p, B_p contains the baseline factor: its centre agrees with a point of J_p
   modulo p^K_p, and that point belongs to p^beta_p Z_p. This intersection imposes no extra constraint.
   Otherwise retain b_p modulo p^L_p. Also impose z = 0 modulo 4 when 2 is not more refined.
7. Integer CRT gives one a modulo R = 2^L_2 product_(odd p in S) p^L_p, with L_2 = 2 if
   2 is not in S. Then C_S = a + R Zhat, stored as (a,R,1), 0 <= a < R. CRT is supported by
   refs/src/baker-padic/padicnotes.txt:374-387. Every local image lies in this set.
8. Exact local results must first be enclosed at requested N for this construction. A singleton
   condition at just one prime cannot be stored as a global finite ball. The _at form can retain
   exact zero; the _refine form deliberately uses 0 + p^N Z_p there. Taking N <= beta_p simply
   leaves the baseline at that prime. No negative-radius or fractional-radius CRT is needed.

Check: oracle check_crt verifies both membership directions in finite quotients, also for N <= 0.
The all-places answer is an adele, not an idele: logarithms can have zero coordinates and real value zero.

## 2. Proposed interface and decisions

Place these declarations in gfunc.h, with implementations later in gfunc.c. Names use the input type,
as f-slice10's adf_idele_root does. All-places forms have no _at suffix. Single-place forms mirror rfunc.h.
The separate _refine form is still an all-places enclosure; its named list contains primes only.
No new mathematical value type is needed for these operations.

1. Output. Recommend adf_adele for the conservative and refined all-places results; adf_sball for
   a single named place. Alternatives are a new hybrid type or an sball alone for refinements.
   CRT in IL5 avoids a new type and retains the unlisted coordinates. An sball alone cannot do that.
2. Real coordinate. Recommend Log on positive I and a separately named log_abs on either sign.
   Both use I, not the content r or idele norm. Alternatives are silently taking absolute value or
   rejecting negative ideles altogether. The two explicit names implement SPEC:599-600 and N-D10.
3. Precision. Recommend N in digits for named primes and prec in bits for the real coordinate.
   Finite M > 0 uses K = min(N,E), E = max(k,d) (IL2-IL4). Exact finite input uses N, except
   proved exact zero in the _at form. This agrees with F6 and N-D14. Alternative: ignore N for
   inexact inputs and always compute at E; that repeats the cost defect which N-D14 corrected.
4. Evaluation. Recommend removing p from r, treating unrestricted primes by their exact image,
   and using existing lball_Log only for centres. Alternative: construct the full local ball
   and call lball_Log. That method is correct at restricted primes and at 2, but can allocate
   p^k at huge k even when the caller asks for small N. The odd unrestricted hull is incorrect.
5. Public projection. Defer a public function returning the exact local input set to a type for
   multiplicative local cosets. Such a type can hold (m,a,k), meaning p^m a(1+p^k Z_p), with
   k = 0 meaning all units, plus an exact-rational case. It would mirror adf_sball_project's
   projection meaning. A function returning only adf_lball would need UNSUPPORTED at an odd
   unrestricted prime, or a separate tag and shell payload. Returning the additive hull under
   an unqualified "local component" name would change the set. The Log slice needs only an
   internal descriptor, not a public type or a new projection function.

### Common contract for the declarations

The following is part of every comment block by reference. Inputs are initialised canonical values;
invalid library values are undefined, checked and aborted under ADF_CHECK_INVARIANTS as usual.
Places are valid handles from place.h. NULL where is allowed, as in existing named-place wrappers.
Every value output is untouched on every status other than OK. where is untouched on OK and on a
failure without a particular place; otherwise it receives the place described below. Outputs cannot
alias each other or a part of an input. The output and input value types here differ, so no y = x
alias is supported or needed. The place array is borrowed only for the call and must not overlap outputs.

For an all-places call, prec > ADF_REAL_PREC_MAX gives LIMIT with where = infinity before any
allocation, any other check, or invariant checks (N-D8). prec < 2 is taken as 2. The local _at call
does this only when v is infinity. At a prime its last argument is N unchanged, even when N <= 0;
the real precision maximum does not apply. An arbitrarily large requested N is harmless when
min(N,E) is small. Limits test the resulting exponent and required work, not N alone.

Recommended local resource rules: the resulting ball exponent K must have |K| <= ADF_LBALL_EXP_MAX;
the valuation m of the finite input must fit the same bound. Do not form the absolute input exponent
m+k or a canonical input centre at that exponent just to take Log. At exact rational centres call
adf_lball_Log at K, with its existing working-power bound W bits(p) <= ADF_LBALL_BITS_MAX and its
exact-zero and no-sum shortcuts. These are N-D7 and N-D9. No full factorisation of M or r is needed.
The extraction of v_p uses repeated division at a named prime. Handle additions and comparisons of
exponents with checked arithmetic before converting to slong. A finite M > 0 always returns a ball,
even if its chosen rational centre has Log zero. It is never an exact singleton.

For _refine additionally propose ADF_IDLOG_PLACES_MAX = 65536 and
ADF_IDLOG_CRT_BITS_MAX = ADF_LBALL_BITS_MAX = 2^26. More than this many named primes is LIMIT
with where untouched. Reject it before allocating an array. Before CRT powers are formed, bound
the sum L_p bits(p) (including the factor at 2); exceeding the CRT bound is LIMIT with where
untouched, since the combined modulus is the cause. This conservative bound is part of the proposal,
not an assertion that every integer under the bit bound can be built by every algorithm.
Alternative: no additional limit; a named list then allocates an unbounded global modulus.

After precision and list-shape checks, combine real and prime computation statuses by their numeric
maximum; ties use infinity first, then increasing primes (conventions 3.3). In particular a prime
LIMIT wins over real DOMAIN on a negative I; it must not be hidden by returning early at the real place.
Shape errors (n < 0, repeats, infinity in a primes-only list) are checked before component evaluation.
The first repeated place in canonical order is reported; n < 0 has no offending place.

```c
/* adf_idele_Log(y, where, x, prec): x = (I,r,c U(M)). On OK, y is
   (real_log(I); 0 + 4 Zhat). This contains Log of every point of x (IL5;
   functions.md P12:386-406). The finite answer deliberately uses this same
   conservative ball also for M=0 and r=1; it is not promised smallest.
   Status: OK, y written; DOMAIN for negative I, where=infinity;
   NOT_DETERMINED if the real evaluator cannot certify a finite result,
   where=infinity; LIMIT for prec above ADF_REAL_PREC_MAX, where=infinity,
   decided first. Other statuses cannot occur on valid inputs. Common contract
   above applies, including no cross-type aliasing and output preservation.
   Cost: one adf_real_log and construction of the constant finite ball. */
int adf_idele_Log(adf_adele_t y, adf_place_t *where,
                  const adf_idele_t x, slong prec);

/* adf_idele_log_abs(y, where, x, prec): same finite result as adf_idele_Log;
   real coordinate adf_real_log_abs(I), enclosing log(abs(t)) for every t in I.
   Every valid idele excludes real zero, including a negative I, so no real
   DOMAIN is possible. Status: OK; NOT_DETERMINED for a non-finite computed
   real enclosure; LIMIT for prec above the maximum, decided first.
   On failure where=infinity and y is untouched. Common contract applies.
   Cost: one adf_real_log_abs and construction of the constant finite ball. */
int adf_idele_log_abs(adf_adele_t y, adf_place_t *where,
                     const adf_idele_t x, slong prec);

/* adf_idele_Log_at(y, where, x, v, prec): only the named place v remains in y.
   At infinity: real_log(I) at max(prec,2) bits, as adf_sball_Log_at.
   At p: the local image IL2-IL4 enclosed at K=min(prec,E) when M>0;
   E=max(v_p(M),d). If M=0, evaluate Log(r c) at prec digits; return exact
   zero for r=p^m as lfunc.h does, otherwise a ball of exponent prec.
   A prime not dividing M is always determined: centre 0, E=d. The raw
   stored case v_2(M)=1 has E=2. Use no additive hull at unrestricted odd p.
   Status: OK; DOMAIN at infinity for negative I; NOT_DETERMINED at infinity
   only if the real evaluator cannot certify a finite enclosure; LIMIT at
   infinity for excessive real precision, decided first, or at p for a local
   resource bound above. where=v on each failure. No missing-place status:
   an idele has a coordinate at every valid place. Common contract applies.
   Cost at p: removals of p from M and r, a centre Log at K where needed;
   unrestricted primes need no series or modular power. */
int adf_idele_Log_at(adf_sball_t y, adf_place_t *where,
                     const adf_idele_t x, adf_place_t v, slong prec);

/* adf_idele_log_abs_at(y, where, x, v, prec): at infinity, only the real
   coordinate log(abs(I)), as rfunc.h log_abs_at; OK for either sign.
   At a prime: UNSUPPORTED with where=v, matching rfunc.h:106; use Log_at
   there. NOT_DETERMINED for a non-finite real computed enclosure; LIMIT for
   excessive real precision at infinity, decided first. On these failures
   where=v and y is untouched. No DOMAIN on valid idele inputs.
   Common contract applies. Cost: one adf_real_log_abs at infinity. */
int adf_idele_log_abs_at(adf_sball_t y, adf_place_t *where,
                        const adf_idele_t x, adf_place_t v, slong prec);

/* adf_idele_Log_refine(y, where, x, primes, n, N, prec): an all-places
   enclosure (real_log(I); a + R Zhat). The finite ball is exactly C_S of
   IL5 for S=primes[0..n-1], with K=min(N,E) for M>0, K=N for M=0.
   Exact local zero is rounded to 0+p^N Z_p here, before intersection with
   4 Zhat. The result remains inside 4 Zhat even when N<2. n=0 permits
   primes=NULL and gives exactly the conservative form's set.
   Status: OK; LIMIT for excessive real prec first (where=infinity), for
   n>ADF_IDLOG_PLACES_MAX or CRT size above its bound (where untouched), or
   a local computation beyond lfunc limits (where=that prime); DOMAIN for
   n<0 (where untouched), a repeated prime (where=first repeated place in
   canonical order), infinity in the prime list (where=infinity), or negative
   I (where=infinity); NOT_DETERMINED if the real evaluator cannot certify a
   finite result (where=infinity). Shape checks precede evaluation; component
   failures combine by maximum with canonical place tie breaking. Values
   unchanged on any failure. Common contract and no cross-type aliasing apply.
   Cost: n local evaluations at capped precision, integer CRT, one real log. */
int adf_idele_Log_refine(adf_adele_t y, adf_place_t *where,
                         const adf_idele_t x, const adf_place_t *primes,
                         slong n, slong N, slong prec);

/* adf_idele_log_abs_refine(y, where, x, primes, n, N, prec): same finite
   ball, precisions, limits, shape statuses, failure preservation and cost as
   Log_refine; the real evaluator is adf_real_log_abs. Negative I is admitted.
   Thus DOMAIN occurs only for the stated list-shape errors on valid inputs;
   NOT_DETERMINED is only a non-finite computed real enclosure. Common
   contract and maximum/canonical-place status combination apply. */
int adf_idele_log_abs_refine(adf_adele_t y, adf_place_t *where,
                            const adf_idele_t x, const adf_place_t *primes,
                            slong n, slong N, slong prec);
```

No call returns NEEDS_SPLIT: every local Log image here is a ball or a singleton, including the
2-adic sign union. No finite DOMAIN, NOT_UNIT, UNIT_NOT_CERTIFIED or uncertain-zero failure can
come from a canonical idele. The real logarithm may contain zero; an adele result permits it.
Do not copy the sign-preserving idele-result invariant to this output type.

Recommended thin implementation order: conservative Log and log_abs plus one-place Log_at and
real log_abs_at first, then the bounded CRT _refine pair. Refined local values can already be held
in an sball by assembling one-place results with sball_set_arb_lballs. That is a named projection
only; IL5's global CRT construction is the form that retains 4 Zhat outside the named list.

## 3. Exact-integer oracle and its proof

The oracle is proto/idlog_checks.py. It uses only argparse, json, functools and math from the standard
library. Rationals are pairs of integers reduced by gcd. There is no floating arithmetic, p-adic package,
or dependency on a C implementation. All enumeration comparisons state H explicitly.

### IL6. Why the enumeration is complete

At output precision H >= max(d,k), enumerate every unit u modulo p^H satisfying u = c modulo p^k
when k > 0. For k = 0 enumerate all units; for M = 0 enumerate the one exact unit.
The comparison precision is H, the enumeration precision H' is H, and the normalized input precision
is also H (the precision of X_p / p^m). This corresponds to absolute exponent m+H on X_p itself.

Proof.

1. If two units a,b agree modulo p^H, then b/a = 1 modulo p^H. For H >= d, Proposition 11's
   isometry gives v_p(Log(b)-Log(a)) = v_p(b-a) >= H. Thus the unit residue determines its
   output residue at H. For H <= d every unit Log is zero modulo p^H instead.
2. The prime-free rational r' is known exactly. Multiplication by r' is a unit permutation and
   preserves the congruence exponent H. Thus Log(r' u) modulo p^H is determined by u modulo p^H.
   A positive or negative m has no effect because Log(p^m) = 0.
3. Every admissible residue extends to a local unit in the input factor. Conversely every point
   of that factor has one such residue. Lemma 5 extends it independently at the other primes.
   Hence enumeration contains every output residue, not only a sample of outputs.
4. For k <= H, a restricted unit class has p^(H-k) members. The unrestricted group has
   (p-1) p^(H-1) members. At 2 with k = 1 it is also the whole unit group.
5. IL2-IL4 predict exactly p^(H-E) distinct output residues, E = max(k,d), for M > 0.
   The program demands set equality and this cardinality. With H = 5 or 6, every k <= 4
   leaves at least one output digit free, so it distinguishes E from E+1.
6. For requested N < E the expected ball is coarser, K = min(N,E). It is a set of residues
   of cardinality p^(H-max(K,0)) in Z_p/p^H Z_p. For K < 0 this quotient shows only its
   integral part; the explicit exponent check tests the larger ball in Q_p as well.
   Set equality is demanded when N >= E; inclusion plus the exact capped exponent is demanded
   when N < E. No coarsened requested answer is called the smallest image ball.

Check: check_components, check_images, check_exact. All H = 5 and H = 6 image comparisons have H' = H.

### IL7. Exact series evaluation and independent routes

For log(1+z) with v_p(z) >= d, keep degrees 1 through T-1 with T = 2H.
Let D = floor(log_p(max(1,T-1))), W = H+D. All powers and divisions are integer modular operations.

Proof.

1. For any j >= 1, if e = v_p(j), then j >= 2^e >= 2e (the last inequality follows by induction
   for e >= 1; e = 0 is immediate). Thus e <= j/2. The degree-j log term has valuation
   j v_p(z)-e >= j/2. For j >= 2H it is at least H and tends to infinity with j.
   The ultrametric inequality bounds finite tails at H; completeness bounds the infinite tail.
2. For j < T, e <= D, so knowing z modulo p^W determines z^j modulo p^W.
   Indeed a^j-b^j = (a-b) sum_(i=0)^(j-1) a^i b^(j-1-i) for integral a,b.
   Dividing its residue by p^e loses at most e digits and leaves at least H.
3. The numerator z^j is divisible by p^e since j v_p(z) >= e. Its residue modulo p^W is
   therefore divisible by p^e as well. The remaining denominator j/p^e is prime to p;
   invert it modulo p^H. Alternate signs and sum modulo p^H. This gives the series exactly
   at the stated precision. H <= d may directly return zero by the valuation of Log.
4. At odd p, decompose a = w u. Then a^(p-1) = u^(p-1), and additivity gives
   Log(a) = log(a^(p-1))/(p-1). The latter divisor is a unit. Compute a^(p-1) modulo p^W,
   then the series above. The representative a only needs to agree with the true unit at H
   by IL6; using its arbitrary higher digits changes no result modulo p^H.
5. At 2 choose s in {+1,-1} from a modulo 4 so s a lies in 1+4 Z_2. Log(a) = log(s a).
   For an independent route, log(a^2) = 2 Log(a). Evaluate it at H+1 digits and divide its
   even integer residue by 2. This loses one digit and returns exactly Log(a) modulo 2^H.
6. The odd-prime cross-check instead lifts w^(p-1)=1 from w = a modulo p. At a solution t_j
   modulo p^j the derivative (p-1)t_j^(p-2) is a unit. For t_j + h p^j, expansion modulo
   p^(j+1) gives f(t_j) + h p^j f'(t_j). Exactly one h modulo p cancels the next digit.
   This proves the lift recursively, as functions.md Lemma 3:72-75 does. Compute log(a/w)
   by the principal series and compare it to the powered route. These routes remove torsion
   differently. The series proof, rather than agreement alone, establishes correctness.
7. check_kernel demands all units' images are exactly every multiple of p^d at H, each with
   multiplicity p-1 at odd primes or 2 at 2. It also checks lifting a to a+p^H leaves its
   output at H unchanged, and that a and a+p^j have Log difference of valuation exactly j
   for every d <= j < H. The latter checks the isometry used to justify H'.

Check: check_kernel. No approximation or transcendental nonrationality assertion is needed.

### IL8. Oracle function and fixtures

Import expected_ball(p, r, c, M, K), where r is an integer or pair (num,den), positive;
c is a unit residue for M > 0 or +1/-1 for M = 0. K is the requested absolute exponent.
The brief's modulus argument N is named M here to separate it from requested precision.

Returned keys are p, exact, center, exponent and image_exponent. exact=true always means the
proved local zero, center=0 and exponent=None. Otherwise exponent is min(K,E) for M > 0,
or K for M = 0; center is the least integral residue modulo p^exponent when exponent > 0,
and zero when exponent <= 0. Convert a nonzero center A to lball fields by extracting v_p(A);
u = A/p^v, N = exponent, exact = 0. Zero uses u = v = 0 with the same finite exponent.
Exact zero uses u = v = N = 0, exact = 1. This conversion compares fields, not printed approximations.

expected_refinement additionally rounds exact zero to the requested exponent and intersects with
the baseline exponent beta_2 = 2, beta_p = 0 at odd p. Its (center,exponent) pairs can be sent to
the CRT helper; include (0,4) when 2 is not already supplied. check_crt demands both membership
directions over two full periods of the resulting modulus. This proves each finite quotient check
is an equality check, not only containment of the known outputs.

The grid has p in {2,3,5,7}, m in {-2,-1,0,1,2}, and four prime-free contents at each m:
1, p+1, 1/(p+1), (2p+1)/(p+1). Its M = p^k times one of 1,11,143 with k = 0 through 4.
Residues are the valid members of {1,M,M-1,p,p+1,2p+1} in 1..M. In particular c can be
divisible by p when k = 0. The exact-unit grid has both signs. This finite grid is not an
exhaustive test of all rational contents or primes, nor a differential fuzz run of an implementation.

Failures are: any extra or missing output residue; an output count different from p^(H-E);
an input projection unequal to its claimed set; a wrong K or canonical centre; different images
for factor-2-equivalent pairs; an incorrect exact-zero flag; wrong independent torsion removal;
a failure of the isometry or precision-lift comparison; or disagreement of CRT membership.

Commands (all run under timeout; the run record is in section 5):

```text
timeout 120 python3 -B proto/idlog_checks.py --H 5 --fixtures lanes/d-idlog/fixtures.jsonl
timeout 120 python3 -B proto/idlog_checks.py --H 5 > lanes/d-idlog/oracle-H5.log
timeout 120 python3 -B proto/idlog_checks.py --H 6 > lanes/d-idlog/oracle-H6.log
timeout 120 python3 -B proto/idlog_checks.py --H 6 --checks components images
timeout 120 python3 -B proto/idlog_checks.py --H 6 --checks factor_two exact kernel crt faults
timeout 10 python3 -B proto/idlog_checks.py --expect 3 4 1 1 9 5
```

The --fixtures option writes 21880 JSONL rows: 21400 inexact-input rows and 480 exact-unit rows.
They use requested exponents 0,1,2,4,6 for inexact inputs and 0,2,6 for exact units. The row keys
are p, r, c, M, requested and expected. These fixtures contain mathematical local answers;
they carry neither real-coordinate results nor C resource statuses.

## 4. Implementation test plan

This section is a plan for the next implementation lane. None of these C, driver or Julia tests has run here.
Extend gfunc.h, gfunc.c and api-1f8.md only after the orchestrator assigns their ownership. The mathematical
oracle can be used unchanged; implementation tests must add the status, state and real-coordinate checks.

1. Generate the JSONL fixtures with the command in section 3. For every row build x with an exact positive
   real coordinate, r from the integer pair and the stored unit (c,M). Call Log_at and compare p, exact,
   u, v and N against the normalized expected fields. Test stored k = 1 at 2 as well as normal form.
   Test the whole image by the enumerated residues, not only the output centre. At K >= E require the
   smallest local image; at K < E require the unique capped ball. The 21880 rows include both kinds.
2. Add real fixtures I = {1}, {2}, [1/2,2], {-1}, {-2}, [-2,-1/2]. For Log the three positive
   intervals are OK, the three negative intervals DOMAIN with where=infinity. For log_abs all six
   are in the domain. I={1} and I={-1} for log_abs give real zero. Keep the finite part identical
   between the sign cases. Also vary I independently from r, e.g. I={2}, r=4, M=9, c=1.
   Compare the real interval with higher-precision arb_log or BigFloat endpoints; demand enclosure
   of both endpoint values, and inspect finite output equality separately. This tests wrapping, not arb.
3. Cover every promised status and output state. OK at unlisted odd primes and at 2; exact-zero and
   inexact-zero-centred finite outputs; DOMAIN from negative I, n<0, repeats and infinity in a
   primes-only list; UNSUPPORTED from log_abs_at at a prime; LIMIT from prec=2097153, K below
   -ADF_LBALL_EXP_MAX on a finite input, a centre needing an excessive working modulus, too many
   named primes and aggregate CRT size. Use already initialised output sentinels and compare them
   bitwise on failure. where must match or remain untouched as documented. NULL where must work.
   A non-finite computed real result is the only NOT_DETERMINED route; if no valid input induces
   it naturally, use an injected evaluator failure in a scratch build and label it as injected.
   Never forge an idele containing zero to test a status: it violates the input invariant.
4. Precision edge cases: N=-2,0,1,2, E-1,E,E+1 and large N with a small E; prec below 2 and above
   its real bound; exact local zero at huge requested N in _at (exact shortcut) and _refine
   (finite rounding, hence subject to limits). At an unrestricted prime, no series or large
   local modulus should be allocated. At a restricted prime with huge k but small requested N,
   do not construct p^k merely to obtain the local component. A prime-only _at call at
   N=2097153 with E=1 is OK; the same number as real prec gives LIMIT.
5. CRT tests: S empty, {2}, {3}, {2,3}, {3,5}, {2,3,5}, in shuffled order. Project the resulting
   adele back to these places and compare to expected_refinement, including exact inputs and
   N<=0. Every finite result is inside 4 Zhat. For (I={2},r=4,c=1,M=9), S={3}, N>=2, demand
   the finite triple (12,36,1): it means 12 + 36 Zhat, whose 3-factor is 3 + 9 Z_3 and whose
   2-factor is 4 Z_2. At an unlisted odd prime its projection is Z_p, as an enclosure promises.
6. Combined statuses: negative I together with a prime LIMIT must return LIMIT at that prime;
   excessive real prec together with any finite fault returns LIMIT at infinity first; equal
   prime failures name the smaller prime regardless of input array order. Successful calls leave
   where untouched. No same-type value alias exists in this proposed API; test repeated calls
   reusing outputs. Prohibit output/input-part overlap as in conventions 4.1 rather than testing it.

Plant these eight finite faults in a scratch implementation, one at a time. The Python mathematical
mutants demonstrate the distinguishing residue sets; they do not substitute for planting them in C.

| Fault | Witness (p,r,c,M,requested N) | Required result | Wrong rule rejected |
|---|---|---|---|
| 1 | (3,3,1,9,5) | Log centre 0, exponent 2 | final exponent m+k=3 instead of k |
| 2 | (3,1/3,1,9,5) | Log centre 0, exponent 2 | input exponent k instead of m+k, then image k-m=3 |
| 3 | (3,4,1,9,5) | 3 + 9 Z_3 | drop Log(r') and return 9 Z_3 |
| 4 | (2,1,1,1,5) | 4 Z_2 | k=0 returns 2 Z_2 |
| 5 | (3,1,1,1,5) | 3 Z_3 | unrestricted odd prime returns Z_3 |
| 6 | (2,1,1,8,1) | 2 Z_2 as the requested cap | ignore N and return exponent 3 |
| 7 | (2,1,1,2,5) | 4 Z_2 | treat stored k=1 as ordinary relative precision |
| 8 | (3,3,1,0,5) | exact zero | treat exact M=0 as an unrestricted finite unit group |

Additional wrapper faults worth planting are silently applying real absolute value in Log, taking real
log from r instead of I, dropping the factor 4 during CRT, using an odd unrestricted hull, or returning
before a higher-priority prime status. These are outside the oracle's eight finite mutants.

Proposed driver names are case-sensitive Log, Log_at, Log_refine, log_abs and log_abs_refine.
The existing lower-case log_at remains the series operation. Log_at dispatches an idele to the new
idele function. The refine command accepts an explicit finite prime list and uses the driver prec
setting as both N and real bits; the library retains separate N and prec arguments. Examples:

```text
prec 5
Log (2 ; 4 * [1 mod 9])
Log_at (2 ; 4 * [1 mod 9]) with 3
Log_at (2 ; 4 * [1 mod 9]) with 5
Log_at (2 ; 4 * [1 mod 9]) with 2
Log_refine (2 ; 4 * [1 mod 9]) with 3
Log (-2 ; 4 * [1 mod 9])
log_abs (-2 ; 4 * [1 mod 9])
```

Expected finite parts: 0 mod 4; `3: 3 + O(3^2)`; `5: 0 + O(5^1)`; `2: 0 + O(2^2)`;
12 mod 36; real DOMAIN; 0 mod 4. Compare real enclosures of log(2), not a fixed rounded string.
The last two use the same finite set. Put separate golden files around values and failures because
the driver exits 1 when any command fails. No command above is available before this API is built.

Future bounded commands, after the implementation files and golden files exist:

```text
timeout 120 python3 -B proto/idlog_checks.py --H 5 --fixtures lanes/f-idlog/fixtures.jsonl
timeout 120 make -j2 BUILD=lanes/f-idlog/build INV=1 lanes/f-idlog/build/test_gfunc
timeout 120 lanes/f-idlog/build/test_gfunc
timeout 120 make -C tools/adf -j2
timeout 60 build/adf tests/driver/idlog-values.cmd
timeout 60 build/adf tests/driver/idlog-status.cmd
timeout 120 sh tests/test_driver.sh
timeout 120 make -j2 BUILD=lanes/f-idlog/san SAN=1 lanes/f-idlog/san/test_gfunc
ASAN_OPTIONS=detect_leaks=1 timeout 120 lanes/f-idlog/san/test_gfunc
timeout 60 julia --startup-file=no tests/julia/idlog.jl build/libadelefeld.so
```

The driver Makefile fixes its output to build/adf (tools/adf/Makefile:28-30, :47-61); its build
is a later implementation-lane action, not an action authorised to this design lane. Stop and report
a timeout. Do not replace it by a longer run on this laptop.

A Julia example for the future tests/julia/idlog.jl follows. It uses only Libdl and Test and exported
size/init/clear/accessor functions, as tests/julia/ideles.jl and f_at.jl do. It parses an idele whose
real coordinate and content differ, refines at 3, and checks the exact finite CRT result. This is
example code for the proposal, not an executed Julia check or an assertion that the symbols exist.

```julia
using Libdl, Test
lib = Libdl.dlopen(ARGS[1])
ad(s) = Libdl.dlsym(lib, s)
struct Place
    opaque::UInt64
end
function value(sizefn, initfn)
    p = Libc.malloc(ccall(ad(sizefn), Csize_t, ()))
    ccall(ad(initfn), Cvoid, (Ptr{Cvoid},), p)
    p
end
x = value(:adf_sizeof_idele, :adf_idele_init)
y = value(:adf_sizeof_adele, :adf_adele_init)
f = value(:adf_sizeof_fball, :adf_fball_init)
try
    text = "(2 ; 4 * [1 mod 9])"
    @test ccall(ad(:adf_idele_set_str), Cint,
        (Ptr{Cvoid}, Cstring, Csize_t, Clong, Ptr{Cvoid}),
        x, text, sizeof(text), 128, C_NULL) == 0
    v = Ref(Place(0))
    @test ccall(ad(:adf_place_prime), Cint, (Ref{Place}, Culong), v, 3) == 0
    primes = [v[]]
    where = Ref(ccall(ad(:adf_place_inf), Place, ()))
    @test ccall(ad(:adf_idele_Log_refine), Cint,
        (Ptr{Cvoid}, Ref{Place}, Ptr{Cvoid}, Ptr{Place}, Clong, Clong, Clong),
        y, where, x, primes, 1, 5, 128) == 0
    ccall(ad(:adf_adele_get_fin), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}), f, y)
    len = Ref{Csize_t}(0)
    s = ccall(ad(:adf_fball_get_str), Ptr{UInt8}, (Ref{Csize_t}, Ptr{Cvoid}), len, f)
    try
        @test unsafe_string(s, len[]) == "(* ; 12 mod 36)"
    finally
        ccall(ad(:adf_str_free), Cvoid, (Ptr{UInt8},), s)
    end
finally
    for (p, clearfn) in ((f,:adf_fball_clear), (y,:adf_adele_clear), (x,:adf_idele_clear))
        ccall(ad(clearfn), Cvoid, (Ptr{Cvoid},), p)
        Libc.free(p)
    end
end
```

## 5. Findings and run record

No counterexample was found to SPEC 9.3.2:598-600, functions.md Proposition 12:386-406,
or ideles.md Lemma 5:94-108. IL1-IL5 prove the assertions used here stepwise.

The draft needs two qualifications. The claim in brief-draft.md:170-175 and :298-306 that an
unrestricted local unit set is not an additive ball, and that its hull contains zero, is false at 2.
Counterexample: r=1, M=1, c=1. The local set is Z_2^x = 1 + 2 Z_2 and the hull has this same
2-factor. Its Log image is 4 Z_2 and lball_Log is determined. At an unrestricted odd prime,
the draft's objection to the hull is correct. Also k=1 is absent from normal form, not from all
canonical stored values: the valid stored pair (c,M)=(1,2) is retained by constructors (CV-17).
It represents the same set as (1,1) and must be handled.

There is no implementation, exported declaration, driver command or Julia test in this lane.
The all-places real wrapper and the recommended resource limits remain a design for review.
The exact finite logarithm of a rational need not be a representable exact rational output;
no nonzero-rational-value theorem is claimed or required.

Sources pending, inherited explicitly from functions.md: the conventional name and normalisation
of Iwasawa Log (:343), the name Teichmueller representative (:101), and the real analytic facts
of Lemma 2 (:40-50). The finite statements here use the defined series and their proved identities.
FLINT's documentation for arb_log is present; it does not replace a proof of the real analytic facts.

The H=5 full run completed with these counts. The H=6 expanded checks are split into bounded commands;
two preceding full expanded H=6 runs hit timeout 120 after completing the component and image groups.

| Check | H=5 | H=6 |
|---|---|---|
| Local component set equalities | 4280 | 4280 |
| Local Log image set equalities | 4280 | 4280 |
| Unit residues in image enumeration | 4108800 | 27307600 |
| Capped precision checks | 34240 | 34240 |
| Factor-2 equivalent pairs | 1644 | 1644 |
| Exact-unit cases / exact zeros / sign pairs | 160 / 40 / 80 | 160 / 40 / 80 |
| Kernel units / output classes | 17084 / 3115 | 113860 / 20191 |
| Torsion lifts / square routes | 17068 / 16 | 113828 / 32 |
| Isometry differences | 68320 | 569268 |
| CRT cases / residue memberships | 120 / 91160 | 120 / 91160 |
| Planted finite mutants rejected (comparison H=5) | 8 of 8 | 8 of 8 |

CRT's requested N are -2,0,1,2,3; its quotients are the two full periods described in IL8.
The exact oracle commands, exit codes, all intermediate runs and timeouts are in report.md.
Finite enumeration is evidence for these quotients; IL1-IL7, not the counts, prove the infinite sets.

## 6. Open points for TJO

1. Result representation: adele for both global forms, plus sball for single-place results; or
   unrefined adele with only named partial results; or a new hybrid type. Recommend the first,
   with bounded integer CRT, so "refined" keeps its all-places meaning and needs no new type.
2. Real naming: Log and separately log_abs with the same finite Iwasawa image; or implicit absolute
   value. Recommend explicit names. Recommend log_abs_at remain real-only like rfunc.h.
3. Exact inputs: always the specified 4 Zhat in the conservative form; or detect the global zero
   for r=1. Recommend the constant conservative ball, local exact-zero shortcuts in _at, and
   finite rounding to N in _refine. This gives a simple reproducible enclosure contract.
4. Local input accessor: publish a tagged multiplicative-coset type now; return UNSUPPORTED from
   an lball accessor at unrestricted odd primes; or defer. Recommend deferral and an internal
   descriptor. An eventual exact projection should return a shell at these primes and mirror
   sball_project's set semantics; it must never label a hull as the exact component.
5. Work and precision: compute compact centres at min(N,E) and bound the CRT by 2^26 bits and
   the prime list by 65536 entries; or construct full input balls, or leave aggregate work
   unbounded. Recommend compact centres and both explicit bounds. This follows N-D14's reason
   for capping work and avoids allocating huge input precision just to produce a coarse answer.
