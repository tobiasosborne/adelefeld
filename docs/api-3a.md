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
