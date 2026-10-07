# Slice 3.1-a: quotient lifts

The type and represented sets are those of `docs/api-3.md` section 1.
Write pi for A -> A/Q. A LIFT stores pi(piece[0]); PIECES stores union_i pi(piece[i]).
This slice does not construct reductions, expose set queries, or evaluate the character.
Its canonical predicate accepts both forms because a binding can populate the public struct.

1. `adf_qclass_init`: one owned initialized adele (0 ; 0), LIFT, representing {pi(0)}.
   Check: `test_qclass lifecycle`, `identity_lift`; Julia `qclass.jl`.
2. `adf_qclass_clear`: releases every owned adele and the array. No value remains after clear.
   Borrowed contexts remain caller-owned (conventions 4.6).
   Check: `test_qclass lifecycle`, `canonical`, `identity_lift` under SAN and INV.
3. `adf_qclass_set`: the same represented set and corresponding storage fields, with separate owned arrays.
   Same-object copying has no effect. Backends and borrowed pointers are retained.
   Check: `test_qclass lifecycle`, `identity_lift`.
4. `adf_qclass_swap`: exchange the represented sets and all owned storage in constant time.
   Self-swap has no effect.
   Check: `test_qclass lifecycle`; Julia `qclass.jl`.
5. `adf_qclass_is_canonical`: 1 exactly for the storage invariant of design section 1 and CV-45.
   PIECES uses strictly increasing exact endpoint/H/A keys; equal keys across backends are duplicates.
   The predicate certifies neither a unique representation of a quotient set nor reduction provenance.
   Check: `test_qclass canonical`, including 2000-bit midpoint temporaries, 20 allocation-balanced
   comparisons, huge exponent cancellation and canonical ordering of nonzero finite centres.
6. `adf_qclass_identical`: 1 exactly for equal form, length, and corresponding adele representations.
   Identity implies equality of represented sets. The converse need not hold.
   Check: `test_qclass identity_lift`, `translation`.
7. `adf_qclass_set_adele`: pi of the exact supplied stored adele set, preserving every field.
   There is no additional real rounding. See quotient P10.1 and design section 1.
   Check: `test_qclass identity_lift` (global, local, finite points); Julia `qclass.jl`.
8. `adf_qclass_set_rat`: {pi(q)} = {pi(0)} for every exact rational q, by P10.1.
   The result is identical to init, including for a rational that cannot be represented exactly in Arb.
   Check: `test_qclass identity_lift` (0, 1/2, -7/3, 2000-bit numerator); Julia `qclass.jl`.
9. `adf_qclass_form`: the stored tag. It does not change the represented set.
   Check: `test_qclass lifecycle`; Julia `qclass.jl`.
10. `adf_qclass_length`: the stored array length, rather than a count of fundamental-domain fibers.
    Check: `test_qclass lifecycle`; Julia `qclass.jl`.
11. `adf_qclass_get_piece`: an independent copy of the stored representative i whose image contributes to x.
    DOMAIN for an invalid index leaves the initialized output unchanged (CV-06).
    Check: `test_qclass identity_lift` (negative, len, WORD_MAX and a PIECES entry); Julia `qclass.jl`.
12. `adf_sizeof_qclass`: sizeof the public struct; no change of represented set or storage.
    Check: `test_qclass lifecycle`, Julia `qclass.jl`, exported-symbol check.
13. `adf_alignof_qclass`: alignment of the public struct; no change of represented set or storage.
    Check: `test_qclass lifecycle`, Julia `qclass.jl`, exported-symbol check.
14. `adf_qclass_add_rat`: y represents x + pi(q) = x, by P10.1, and is identical to x.
    It copies storage rather than translating an adele, because outward rounding after a non-dyadic
    translation only gives containment (P10.3).
    Check: `test_qclass translation` (1000 random q, 2000 membership witnesses), `identity_lift`;
    `tests/driver/qclass-lift.cmd`; Julia `qclass.jl`.
15. `adf_qclass_set_str`: a LIFT with pi of an adele enclosure of the exact decimal input interval.
    This is the same set contract as adele reading (conventions 9.5), followed by design section 1's pi.
    Status failures preserve all output fields. Union syntax is UNSUPPORTED in this slice.
    Check: `test_qclass text` (all 29 golden rows, byte/grammar/exponent/count/status precedence).
16. `adf_qclass_get_str`: LIFT value text whose exact decimal represented set contains the stored image.
    The printer is the ordinary adele printer followed by + Q. It allocates a string owned by the caller.
    PIECES is not printable in this slice. A C reread can widen (conventions 9.6); P10.3 concerns
    translations, but the same containment under projection applies to this real rounding.
    Check: `test_qclass text` (exact reference expectations and enclosure on reread), driver and Julia.

## Choices where the slice design is silent

- `qadd_rat X with R` is the driver spelling. Alternative: `qclass` as a conversion command.
  The brief asks for a translation call; lifts already enter through their value text.
- Union grammar, decimal exponents and max_items are checked before UNSUPPORTED, but union value
  semantics are deferred. Alternative: validate all piece semantics now. That would implement part of 3.1-d.
- max_items counts union entries, not the single stored adele of a LIFT. Alternative: count every lift as
  one piece. Conventions 8.4 names pieces, and design section 1 distinguishes lifts from PIECES.
- The PIECES printer returns NULL and length 0, as the only failure channel of this signature.
  Alternative: declare a status-bearing printer, or implement the later full printer now.
- The common numerical precision cap applies to the reader before byte access (api-3.md section 1).
  Alternative: inherit the older adele reader's lack of that explicit cap.
- Exact endpoint ordering uses the sign of a correctly rounded four-term arf_sum at two bits.
  Alternative: form exact endpoints with ARF_PREC_EXACT, which may allocate very large mantissas.
  For any nonzero finite dyadic sum, rounding toward zero at two bits preserves its sign because arf
  exponents are unbounded. A zero sum remains zero. Thus the comparison is exact.
  Source: `refs/src/flint-3.0.1/arf.rst:24-38`, `:65-68`, `:638-645`.

## Findings against the design

The fixed-point wording in api-3.md 2.5 and its acceptance notes must be read with conventions 9.6.
For the lift `(3.14159 +/- 1e-5 ; 5/3 mod 6) + Q`, at 128 bits and six printed digits the first C
print has radius 1.1e-5. Rereading and printing gives radius 1.2e-5. The exact reference's printed
fixed point does not apply to the C parser's rounded radius. No specification or golden file was changed.

## Slice 3.1-b: integer and exact finite radii

17. `adf_qclass_reduce(y,x,piece_limit,prec)` implements algorithm R in `docs/api-3.md` 2.2.
    The exact closed pieces have the same image as the input by quotient.md P6:129-151 and P10:249-250.
    The result is PIECES. Q1 with review R1 encloses each exact interval before sorting and deduplication.
    The stored image therefore contains the input image; equality after rounding is not asserted.
    Check: `test_qclass_reduce`, integer.jsonl (65 stored dyadic lifts, 2600 rational point labels).

The code first reads the exact midpoint and mag radius, with no clipping of stored spill.
It subtracts the exact canonical finite centre from both endpoints before any real rounding.
This keeps the diagonal translation exact (P10:242-250). For a non-point interval the integer range
is floor(l) <= n < ceil(h); for a point it is the one integer floor(l).
The closed construction uses [max(l,n)-n,min(h,n+1)-n] and finite centre -n, modulo A when A > 0.
These are P6's closed pieces. Its endpoint gluing proves that an integral upper end needs no extra point.
At radius zero the finite radius stays zero (P10:249-250).
Check: all exact pieces are contained in same-finite-part stored pieces; boundary labels use rational translations.

Before allocating the piece array, a first pass sums counts in fmpz and compares with piece_limit.
The byte product is checked before conversion to storage. A second pass constructs the admitted pieces.
Q1 rounds the midpoint to nearest even at max(prec,2) bits; its required radius is d=max(m-l,h-m).
For d>0 the least RU30 radius and its least successor are constructed directly as a mag.
For d=0 the radius stays zero. This avoids mag conversions that may add several ulps
(`refs/src/flint-3.0.1/mag.rst:6-15`; correctly rounded division: `arf.rst:24-35`, `:662-681`).
Both endpoint containment and rho-d <= 2^-28 d are tested. The exact Q1 storage is tested as well.
Check: 2, 20, 53, 128-bit oracle storage, including radius binade carries and non-dyadic boundary spill.

The result sorts stored exact lower/upper/H/A keys and removes only equal keys.
There is no merging or full-image shortcut. The limit counts construction before rounding and deduplication.
All failures are LIMIT: precision above ADF_REAL_PREC_MAX, piece_limit below 1, count overflow of the limit,
or exact-work bounds. The initialized output stays untouched; y may equal x.
Cost is two O(K) passes of rational arithmetic, O(K log K) comparisons, and O(K b) memory.
Exact-work bounds are ADF_QCLASS_EXP_MAX=2^20 and ADF_QCLASS_BITS_MAX=2^21.
Projected cross-products use conservative unreduced bit lengths, with a carry bit, before arithmetic.
Local inputs also check their raw H (the CRT modulus), A and d before canonical reconstruction.
Check: K and K-1 status tests with representation sentinels; aliased output on every vector.

18. `adf_qclass_get_str` now prints PIECES as union(X, X, ...) + Q.
    Each ordinary adele string encloses the stored ball. Sorting and deduplication use the exact decimal
    printed endpoints and canonical H/A, as conventions 9.4 requires. Projection preserves the enclosure.
    The reader still returns UNSUPPORTED for union syntax after its existing syntax and text-limit checks.
    Check: `test_qclass_reduce printer`, `test_qclass text`, `tests/driver/qclass-reduce.cmd`.

The driver spelling is `qreduce X with LIMIT`, where LIMIT is an integer rational fitting a slong.
A fractional operand gives DOMAIN; an out-of-range integer gives LIMIT before conversion.
Check: driver fixtures, Julia `adf_qclass_reduce` ccall, including an aliased output and an untouched LIMIT output.

## Slice 3.1-c: fractional finite radius and stored spill

Algorithm R writes positive N=A/B in lowest terms and first uses the B fibres a+jN+A Zhat.
Quotient.md P8:181-198 proves that this is the original finite ball, without any factorization.
Each fibre then follows the same exact translation, closed splitting, and Q1 rounding as 3.1-b.
The union before rounding equals the input image by P8, P6, and P10.
Check: fractional.jsonl (60 vectors, 2400 rational point labels), E3 in driver and Julia.

The first pass compares B with the remaining limit in fmpz before converting B or looping over fibres.
Each range count is added and checked before entering its integer loop or allocating pieces.
The same preflight covers multiple stored entries by summing their raw construction counts.
Check: a lift with exactly 10^30 fibres and a width of 2^101 both return LIMIT within a 1 s CPU guard.
A small K-1 refusal invokes zero allocation callbacks. Representation and member bytes stay unchanged on LIMIT.

A PIECES input means the union of its entire stored balls, including spill.
The code reads their exact dyadic endpoints and reduces them without intersection with [0,1].
It follows the same P6 construction for each stored entry, so re-reduction cannot shrink its represented set.
This is not a fixed point of storage: the newly constructed pieces each incur Q1's outward margin again.
The test asserts exact Q1 equality, containment, and rho-d <= 2^-28 d for EVERY newly constructed piece,
where l,h are the exact R pieces of the previous stored balls. It asserts no bound on a merged hull or
Hausdorff distance between entire quotient unions, and no representation identity after re-reduction.
Check: spill.jsonl (26 PIECES inputs, 1040 rational labels, including spill translated through the boundary).
Two are isolated legal spill pieces, with no adjacent piece to hide a clipping loss: midpoint 0 with
radius 1/16 and finite radius 2; midpoint 1 with radius 1/16 and exact finite centre -7.

Additional checks cover canonical local data with raw d cancelling to 1, a negative 2001-bit midpoint,
exact finite points, midpoint exponent edges of both signs, the finite bit cap, the numerical precision cap,
and a debug entry check even when the requested piece limit is zero.
Check: `test_qclass_reduce bounds`, `local_and_glue`, `debug_entry` in SAN, INV and clang builds.
The modulus-3 boundary witness tests (0,2) present and (0,1) absent for [1/2,1] x (0 mod 3).
The other half [0,1/2] x (2 mod 3) contains the shared glued point (0,2), as P3:71-73 requires.

Twelve existing union golden rows are built through reduction and compared as exact printed text at two digits.
Seven use their reader-built lifts. Five use tighter dyadic input radii: choose the 30-bit predecessor
of RD30 of the golden decimal radius, so Q1's successor is RD30 of that radius. Their dyadic midpoints
remain exact at 128 bits, and their decimal printed balls have the required golden radius.
This checks singleton unions, finite points, stored and printed duplicate removal, mixed moduli,
wrapping, and an order reversal caused by the decimal rounding of printed endpoints.
The remaining valid union row has midpoint 1 and radius 1/2, which is legal PIECES storage but cannot be a
Q1 output: a rounded midpoint 1 requires an exact interval midpoint at least 7/8, so l>=3/4 and d<=1/4.
The Q1 margin cannot increase that radius to 1/2. No golden row is changed.
Check: `test_qclass_reduce golden_unions`, `test_qclass text` (all 29 reader/status rows).

The exact-work preflight has component tests as well as public status tests.
At the exact dyadic denominator bit cap a point at full precision must stay an exact point.
An input needing one more denominator bit must fail the conversion preflight itself.
A two-entry PIECES input whose first entry consumes all slots must reject the second B=1 before copying
its centre into a fibre. These checks compile qclass.c in the reducer test unit; the lifecycle test links
the archive normally. They distinguish an early refusal from a later refusal returning the same LIMIT.
Check: `test_qclass_reduce bounds`, `remaining_preflight`; four original mutation survivors are killed.

### Q1 radius bound used by this code

Let d>0 and choose e with 2^(e-1) <= d < 2^e. The 30-bit spacing in that binade is s=2^(e-30).
Let u be RU30(d), and rho its least 30-bit successor.
If u<2^e, then u-d<s and rho=u+s. Therefore rho-d<2s <= 2^-28 d.
If u=2^e, then d>u-s and rho=u+2s, because the next binade has spacing 2s.
Thus rho-d<3s, while 2^-28 d>4s-2^-28 s>3s.
These two cases prove rho-d <= 2^-28 d, including a carry onto a binade boundary.
The stored midpoint is RN_p((l+h)/2) with p>=2. Monotonicity and exact 0,1 give a midpoint in [0,1].
Writing eta=abs(m-(l+h)/2), d=(h-l)/2+eta, so each endpoint excess is at most 2 eta+2^-28 d.
This proof avoids the incorrect inequality u<=d printed in the review's proposed R1 replacement.
Check: every exact oracle piece is contained; its required radius and stored rho satisfy the bound exactly.

## Slice 3.1-e

19. `adf_qclass_equal_set(truth,x,y,work_limit)` decides equality of represented subsets of A/Q.
20. `adf_qclass_contains(truth,x,y,work_limit)` decides whether the first represented set is inside the second.
21. `adf_qclass_overlaps(truth,x,y,work_limit)` decides whether their represented intersection is nonempty.

These are Q2 of `docs/api-3.md:546-574`, extending quotient.md P9:212-226 to zero finite radius and spill.
They implement the status-bearing exception of D3-1 and SPEC 15.4 N-D21.
OK writes exactly 0 or 1. LIMIT leaves truth untouched. There is no NOT_DETERMINED result.
Both inputs are canonical initialized classes. They may alias each other; truth aliases no input member.
INV checks both inputs before checking even a nonpositive work limit.
Check: every vector in q-slice6/sets.jsonl, both argument orders, identity aliasing, truth sentinels,
noncanonical input aborts, driver fixtures, and the Julia ccall signature of design section 7.

Normalization follows exact R steps 1-4, with no Q1 or other real rounding.
Read exact midpoint and mag radius, and the canonical finite centre/radius, including local CRT data.
For positive N=A/B, use all B fibres of radius A by P8:181-198.
Translate each fibre diagonally by its exact centre and split using floor(l) <= n < ceil(h).
A real point uses one n=floor(l). Its finite radius remains zero when the input radius is zero.
P6:129-151 proves equality of the closed-piece image with the input image, including the integral upper end.
P10:249-250 gives the same construction for a finite point.
Read every stored real endpoint, including spill, so the construction also preserves arbitrary legal PIECES.
Check: lift versus exact reduction, fractional radii 1/2, 2/3 and 7/360, isolated spill, local raw d cancelling
to 1, finite points, negative centres, and 2000-bit centres with small moduli.

In the half-open section, real 1 contributes at real 0 with finite centre m-1, by P3:71-73.
At each endpoint below 1 and each open-gap midpoint, positive cosets refine to all residues modulo L.
Exact finite points remain integer points; points covered by positive residues are deleted.
Inclusion requires residue inclusion and membership of every residual point in the other fibre.
A finite collection of points cannot cover a positive coset: infinitely many integers in that residue remain.
Intersection checks common residues, common exact points, and points in the other side's positive residues.
This is precisely Q2 steps 4-6. Endpoint and gap membership is constant as required by P9 step 4.
Check: all centres for moduli 2-8 in the glued family, wrong-sign partners, boundary-only overlaps,
N versus its two-piece 2N refinement, and a positive coset against five finite point samples.

The construction preflight rejects B above the remaining work limit before any fibre loop.
It accumulates raw integer-range counts in fmpz, saturating at work_limit+1, including at LONG_MAX.
K counts both arguments before deduplication. L is the lcm of positive integer moduli, or 1.
E counts distinct endpoints of the exact constructed pieces, so E <= 2K.
Require K <= work_limit, L <= work_limit and (2E+1) K L <= work_limit as arbitrary-precision integers.
No identity, full-image or disjointness shortcut bypasses this budget.
Discover E by streaming minimum selection before allocating any piece, endpoint, residue or point array.
The O(K E) discovery cost is within the total O(K^2 L) bound. Afterwards endpoints and point candidates
are sorted once in O(K log K); each cell then uses O(K L) work. Memory is O(K+L) plus exact integers.
Artificial cells outside the exact interval extent are empty on both sides and are skipped.
A glued real zero is retained even outside that extent. At most 2E+1 fibre tests remain.
Allocation byte products, exact exponents and projected bit lengths have the existing R bounds.
An impossible byte product is refused after counting and before the endpoint-discovery stream.
Check: budget and budget-1 for every vector and all calls, zero bulk allocations on budget refusal,
1/10^100 refused within a one-second CPU guard, huge widths, exponent refusals and 2000 pieces with
L=LONG_MAX-1. Direct saturation checks distinguish early count handling from a later LIMIT.
The tests also check an impossible 2^59 construction in under one CPU second, the exact denominator bit cap,
and balanced bulk allocations and initialized integer objects on every query call.

There is no prec argument. The exact balls already determine sets.
Two real point fibres separated by 2^-100 are disjoint here. Q1 reduction at two bits widens one to overlap
the other, demonstrating why rounded reduction cannot substitute for exact internal normalization.
Check: exact-gap vector and `exact_and_local` before and after the deliberately rounded reduction.

### Choices where the query design is silent

- E counts actual piece endpoints. Artificial 0 and 1 used for real-cell enumeration do not add to E.
  Alternative: count all cell boundaries, which would revise the budget and contradict E <= 2K.
- Exact R keeps raw duplicate pieces internally. Alternative: deduplicate after construction.
  The represented union is the same; K must still count before deduplication.
- Endpoint discovery uses repeated streaming minima before allocation.
  Alternative: allocate an endpoint array to discover E, then check the final budget.
  Streaming obeys the explicit pre-allocation contract and retains its total cost bound.
- The driver requires two qclass operands and an integer rational limit, following qreduce's range policy.
  Alternative: coerce adeles and rational operands implicitly to classes.
  The three new verbs make the represented-set question explicit.
- Opposite input-piece orders are sorted in the vector generator before entering the public calls.
  Alternative: pass unsorted storage, which violates section 1's canonical precondition.

## Slice 3.1-f: negation and addition of classes

22. `adf_qclass_neg(y,x,piece_limit,prec)` writes PIECES y enclosing -pi(X) = pi(-X), where X is the union of
    the stored entries of x (Q3, `docs/api-3.md:575-589`). Each entry `[lo,hi] x (a + N Zhat)` is negated
    exactly to `[-hi,-lo] x (-a + N Zhat)`: the end points are reversed, the finite centre changes sign,
    the radius is unchanged (Q3 step 3). Algorithm R then reduces each negated entry exactly, and Q1
    rounds each constructed piece. Sorting and deduplication are those of `adf_qclass_reduce`.
23. `adf_qclass_add(z,x,y,piece_limit,prec)` writes PIECES z enclosing pi(X)+pi(Y) = pi(X+Y), the sums of
    independent points of x and y, even when x and y are the same object (Q3 steps 1-2). For each pair of
    stored entries the exact sum is `[lo1+lo2, hi1+hi2] x ((a1+a2) + gcd(N1,N2) Zhat)`; the finite part is
    the exact set of sums by `docs/proofs/precision.md:27-31` (Proposition 1, through Lemma 2 at :20).
    Then R and Q1 as for negation; sorting and deduplication run over all pairs together.

Why the result encloses the set. Q3 step 2 distributes the operation over the finite unions of stored
entries, so the union of the exact pair sums (or exact negations) represents exactly the result set.
The exact real end points are rationals, added or negated with no rounding. R maps each exact sum to closed
pieces whose image is the image of that sum: P8 (`quotient.md:177-198`) splits a fractional radius A/B into
B fibres of radius A; P10 (`quotient.md:232-250`) makes the diagonal translation by the fibre centre exact,
also for radius zero; P6 (`quotient.md:125-151`) gives the closed pieces with the gluing, so an integral
upper end adds no piece. Q1 (`docs/api-3.md:516-544`) then encloses each exact piece by one stored ball with
a midpoint in [0,1], an excess of at most 2 eta + 2^-28 d at either end, and no other change. So the exact
pre-rounding union equals the result set, and the stored union contains it. Equality after rounding is
not asserted, as for reduction.

Fractional radii. The brief asked what precision.md P1 says when a radius is fractional. Its notation
(`docs/proofs/precision.md:9-11`) takes N as a rational and defines gcd of rationals as the non-negative
generator of the subgroup of Q they generate, 0 exactly when all are 0. Lemma 2 and Proposition 1 are proved
for that gcd, so `(a + 1/2 Zhat) + (b + 2/3 Zhat) = (a+b) + 1/6 Zhat` is covered, and gcd(N,0) = N.
FLINT's `fmpq_gcd` computes this generator: gcd(p/q, r/s) = gcd(ps, qr)/(qs), canonicalised
(`refs/src/flint-3.0.1/fmpq.rst:510-522`). The oracle's `rgcd` (gcd of numerators over lcm of
denominators) agrees for canonical fractions.

Why the real end points are added before rounding. An arb sum of two stored balls rounds the midpoint at
prec and adds the radii as a mag rounded up, so its enclosure is wider than the exact sum. When an exact end
point of the sum is an integer, that widening adds a crossing to R's construction and changes the raw count
(`docs/api-3.md:257-260`). The vectors contain four such pairs: an upper end at 1 whose exact midpoint
1-2^-99 is not representable at 53 bits; a lower end at 2 at 2 bits; two exact points summing to 2^60+1/2;
and two 30-bit radii whose exact sum needs 61 bits, with an upper end at 1.

Count and limit. The limit counts R's construction over all pairs (or entries), before rounding and
deduplication (SPEC 15.4 N-D21, D3-2). The order of checks is: prec > ADF_REAL_PREC_MAX; piece_limit < 1;
len(x) > piece_limit; for add, len(y) > piece_limit / len(x), which decides len(x) len(y) > piece_limit
without forming the product (each pair constructs at least one piece, so this is necessary). Then, per pair,
the fibre count B is compared with the remaining limit in fmpz before its fibre loop (R5), and each integer
range is added to an fmpz total compared with the limit before its loop. Only the first pass's total sizes
the allocation, after a byte-product check; the second pass constructs. All failures are LIMIT; there is no
NEEDS_SPLIT. The output is untouched on LIMIT; z may equal x, y or both, and y may equal x for negation.

Exact-work bounds (D3-2). The stored end points and finite data are read with the bounds of reduction:
exponents within 2^20, numerators and denominators within 2^21 bits, checked before conversion. Every
rational sum is checked by its projected cross-product size before it is formed. The rational gcd is
checked by the sum of the denominator sizes before it is formed (its denominator divides the product).
Local inputs are read through their canonical global centre and radius; the output is global, as for
reduction (conventions 4.6 fallback; `docs/api-3.md` section 1).

Cost: len(x) len(y) pair sums and gcds (len(x) negations), each read twice (count pass and construction
pass), plus R's O(K) rational operations, O(K log K) comparisons and O(K b) memory, K the raw count.
An avoidable cost: each pair re-reads both stored entries (canonical triple, CRT for local data) in both
passes; caching the len(x) + len(y) exact reads would remove it. The simple version is kept.

Check: `test_qclass_arith` over `tests/ref/vectors/q-slice7/arith.jsonl` (64 add and 15 neg records from
the oracle's `add`, `neg`, `reduce` and `round_piece`): stored list equal to the oracle's after Q1 and
deduplication (this pins Q1's kernel and the stored count); every exact piece enclosed by a stored piece
with the same finite part and rho - d <= 2^-28 d; 40 sampled input points per record, checked in C to lie in
the inputs, whose sum or negation, formed in C, lies in the result; LIMIT at K-1, 0, -1, WORD_MIN and
prec cap+1 with struct and member bytes unchanged; aliasing z = x, z = y, z = x = y, y = x for negation;
y + x identical to x + y; `add_rat` before or after `add` identical; neg(neg(x)) contains x and
x + neg(x) contains the zero class, decided exactly by `adf_qclass_contains` where its budget allows.
Check: x + 0 and 0 + x identical to `adf_qclass_reduce(x)`, and neg of a lift identical to the reduction
of the exactly negated adele (`adf_adele_neg`, exact by `adele.h:138`), for 30 lifts: the result with the
zero class is not wider than one Q1 margin, and the copied R and Q1 code agrees bit for bit with reduction.
Check: the hand example below, the pair-count limit (3 x 2 pieces), 10^30 fibres and a 2^101 width refused
within 1 s CPU, the midpoint exponent bound on a sum 2^(2^20-1) + 2^(-2^20+1), the centre-sum bit bound,
local inputs with blocks 8, 9, 5 identical to their global equivalents, and INV aborts for a non-canonical
argument of either call. Driver: `tests/driver/qclass-arith.cmd`; Julia: `tests/julia/qclass_arith.jl`.
Check: the test compiles `src/qclass_arith.c` into its unit, as `test_qclass_reduce` does with `src/qclass.c`,
and checks the copied helpers at their boundaries: exponent 2^20 and 2^20+1 on both sides, a denominator of
2^21 and 2^21+1 bits, equal 2^21-bit denominators admitted, a numerator carry refused, Q1's binade carry
stored as a normalised mag (mantissa 2^29), and Q1 returning failure on an oversized midpoint sum. Every
stored radius is checked to be a normalised mag. A 2^62-piece construction under a WORD_MAX limit is
refused by the byte product before allocation.

Hand example (driver and test): `(0.75 +/- 0.25 ; 0 mod 3) + (0.5 +/- 0.5 ; 1 mod 3)` is exactly
`[1/2,2] x (1 + 3 Zhat)`. With centre 1, l = -1/2 and h = 1, so n = -1, 0: `[1/2,1] x (1 mod 3)` and
`[0,1] x (0 mod 3)`, two pieces because the sum crosses 1. At 2 digits the driver prints
`union((0.5 +/- 0.51 ; 0 mod 3), (0.75 +/- 0.26 ; 1 mod 3)) + Q`; with limit 1 it prints `error: LIMIT`.

### Choices where the design is silent

- The exact read, Q1 and the R loop are copied from `src/qclass.c` into `src/qclass_arith.c`, as lane
  q-slice6 copied R into `src/qclass_sets.c`. Alternative: share them through `src/qclass_internal.h`. That
  needs new hidden adf_-prefixed names, so it edits several lines of `src/qclass.c`, which lane q-slice4 is
  editing at the same time, and `tests/test_qclass_reduce.c` compiles `src/qclass.c` into its own unit. The
  risk of the copy (a second, drifting Q1) is covered by the bit-for-bit identity checks against reduction.
- Negation of a LIFT also returns PIECES after R and Q1, as the design says. Alternative: return the exact
  lift of the negated adele, which `adf_adele_neg` gives with no rounding. The design's comment block
  specifies R and Q1, and every arithmetic result then has one form.
- The pair count is compared with the limit before any entry is read. Alternative: let the per-pair count
  find it. The early check is implied by the count and avoids forming len(x) len(y) as a product.
- The checks of the header are made in reduce's order; INV checks every input before any status check.
