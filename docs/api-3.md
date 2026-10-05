# Milestone 3: quotient sets and the Tate additive character

Design by lane d-quotient, 2026-10-05; repaired after the review q-review1 by lane d-quotient-repair,
2026-10-06. No public header or implementation is changed. D3-1, D3-2 and D3-3 were taken by the
orchestrator on 2026-10-05 and are written into section 6 as taken; they become N-D21 in SPEC 15.4.
SPEC 6 and its decisions remain authoritative. References to Q1 to Q5 mean statements proved below.
The executable oracle is `proto/quotient3_checks.py`; examples E1 to E4 are printed by that program.

The distinction throughout is between a represented set and an unknown point in that set.
Reduction preserves the image set before real rounding. The stored result encloses that image.
An `acb` result is a rectangle enclosing points of the unit circle. The rectangle itself is not on the circle.

## 1. Type, ownership, and common contracts

Proposed header `adelefeld/qclass.h` includes `adele.h`, `rat.h`, and `status.h`.
Proposed header `adelefeld/psi.h` includes `qclass.h`, `lball.h`, `place.h`, and FLINT `acb.h`.
Use the conventions 5.10 struct unchanged:

```c
#define ADF_QCLASS_LIFT 0
#define ADF_QCLASS_PIECES 1
typedef struct {
    int form;
    slong len;
    adf_adele_struct *piece;
} adf_qclass_struct;
typedef adf_qclass_struct adf_qclass_t[1];
typedef adf_qclass_struct *adf_qclass_ptr;
typedef const adf_qclass_struct *adf_qclass_srcptr;
```

Let pi be `A -> A/Q`. LIFT means `pi(piece[0])`, with len = 1 and any canonical adele.
PIECES means `union_i pi(piece[i])`, with len >= 1. The coordinates of each adele are independent.
There is no implicit clipping of its real ball at 0 or 1. The empty set is not representable.
Every operation declared here takes nonempty sets to nonempty sets, so no empty tag is needed.

For PIECES, each adele is canonical, its real midpoint is in `[0,1]`, and its finite canonical global
triple has `d = 1`, `H >= 0`: `0 <= A < H` for H > 0, arbitrary integer A for H = 0.
These tests also apply to a local backend after CRT and canonical cancellation, not to its raw fields.
Sort by exact `(real lower end, real upper end, H, A)` and remove equal set pieces.
Equal keys identify equal pieces even if their finite backends differ. The constructor retains the first
input occurrence on a tie. This is storage canonicality, not a canonical representation of a quotient set.
It certifies neither origin by reduction nor a bound on rounding error (CV-45, conventions:725-739).

The struct suffices for lifts, enclosures, set queries, group arithmetic, and numerical character values.
It cannot remember the exact rational endpoints before rounding, or whether a union arose from a
fractional-radius lift. Those are not fields of this type. Findings F2 and F3 explain the consequences.
No provenance or exact-real extension is introduced silently.

The class owns the array and each adele, allocated with `flint_malloc`; it borrows all modulus contexts.
Clear each element and free the array. Never create, free, or mutate a context during quotient operations.
Lift, copy, and constructors preserve borrowed pointers. Reduction produces global finite balls;
arithmetic below also produces global finite balls. This is an explicit allowed fallback (conventions 4.6).

64-bit layout: form at 0, len at 8, piece at 16; size 24, alignment 8.
Each array entry has the 96-byte layout of `adf_adele_struct`. These are ABI acceptance expectations;
the exported size and alignment queries, not these numbers, are what Julia should use.

All inputs are initialized and canonical unless a constructor says it validates raw input.
Outputs may alias inputs of the same type; inputs may alias each other. No output aliases an input's member.
Two outputs never alias. On a non-OK status every ordinary output, including a truth value, is untouched.
Implement status-bearing calls with temporary values and swap only after all checks succeed.
Use `p = max(prec,2)`. For numerical calls, `prec > ADF_REAL_PREC_MAX` gives LIMIT first.

Resource policy of D3-2, taken 2026-10-05: before constructing exact rational endpoints, reject an
absolute binary
exponent above `2^20`, and a projected numerator or denominator above `2^21` bits, with LIMIT.
Call these `ADF_QCLASS_EXP_MAX` and `ADF_QCLASS_BITS_MAX`. Apply the bit bound to intermediate exact
rationals, common moduli, and finite phases as well. Check projected sizes before shifts or products.
These are explicit implementation bounds, not a claim about the largest mathematically decidable input.
Lifts, copying, identity translation, and dumping retain their existing unrestricted representation contract.
The dump's existing exponent bound applies only to PIECES (SPEC M1-D9), independently of this policy.
The algebraic Python oracle has no such bit bound. Tests of C must cover the limit boundary separately.

Costs below count arithmetic on b-bit integers as `M(b)` for multiplication, with gcd/division costs stated
where relevant. No cost claim treats arbitrary-precision gcd as a machine-word operation.

## 2. Functions of work package 3.1

### 2.1 Lifecycle, lift, and access

```c
/* Init: LIFT of (0 ; 0), one initialized adele. Clear releases all owned storage.
   Neither fails. Source: conventions 5.10:758. Init O(1); clear O(total owned storage).
   init requires uninitialized storage; clear requires initialized storage. */
void adf_qclass_init(adf_qclass_t x);
void adf_qclass_clear(adf_qclass_t x);

/* Copy the representation, including backends and borrowed contexts. Same set.
   Never fails; y may equal x. Cost: len adele copies. */
void adf_qclass_set(adf_qclass_t y, const adf_qclass_t x);

/* Exchange the representations. Never fails; x may equal y. O(1). */
void adf_qclass_swap(adf_qclass_t x, adf_qclass_t y);

/* Return 1 exactly for the storage invariant in section 1, else 0. Never aborts
   for initialized member objects and live pointer fields; a bad tag, len < 1,
   or NULL piece gives 0. A positive len requires that many live array entries.
   Cost: len adele predicates, canonical triples, and exact adjacent key comparisons.
   An exponent-sensitive comparison may be expensive; this is not a reduction call. */
int adf_qclass_is_canonical(const adf_qclass_t x);

/* Return 1 iff form, len, and corresponding adele representations are identical.
   No status, no writes; cost len adele identity tests. Set equality uses another call. */
int adf_qclass_identical(const adf_qclass_t x, const adf_qclass_t y);

/* Set y to LIFT of x: pi(x), exactly as a represented set, with no real rounding.
   Source: quotient.md P10.1:236 and conventions 5.10:720.
   Never fails; cost one adele copy. x cannot be a member of y. */
void adf_qclass_set_adele(adf_qclass_t y, const adf_adele_t x);

/* Set y to the zero class for every exact diagonal rational q. No conversion to arb.
   Source: quotient.md P10.1:236. Never fails. Constant output size.
   q and y have different types and cannot overlap. */
void adf_qclass_set_rat(adf_qclass_t y, const adf_rat_t q);

/* Return the stored form, or the number of stored adeles (one for a lift).
   No status, no writes, O(1). These do not count canonical fundamental-domain fibers. */
int adf_qclass_form(const adf_qclass_t x);
slong adf_qclass_length(const adf_qclass_t x);

/* Copy stored entry i to a. OK writes a; DOMAIN for i outside [0,len) leaves it untouched.
   This is a representative set whose image contributes to x, not a section on points.
   No member aliasing; cost one adele copy. This accessor is in the constructor/accessor
   status class, not the quotient reduction row of conventions 3.2. */
int adf_qclass_get_piece(adf_adele_t a, const adf_qclass_t x, slong i);

/* Return sizeof and alignment of the struct. Header-inline and exported; O(1), no status. */
size_t adf_sizeof_qclass(void);
size_t adf_alignof_qclass(void);
```

### 2.2 Construction and reduction

```c
/* Set y to PIECES, union_i pi(pieces[i]), preserving each real ball exactly.
   Validate each raw adele and the piece constraints, sort and remove equal keys.
   OK writes y; DOMAIN for n < 1 or a bad entry; LIMIT for n > piece_limit or the
   exact-key resource bounds. piece_limit < 1 gives LIMIT before reading the array.
   The limit counts input entries before deduplication. No input array overlaps y.
   Source: conventions 5.10:722-752; quotient P3:71 gives the boundary meaning.
   Cost: n copies and O(n log n) exact comparisons, plus canonical-triple formation. */
int adf_qclass_set_pieces(adf_qclass_t y, const adf_adele_struct *pieces,
                         slong n, slong piece_limit);

/* Reduce the images of every stored adele of x by algorithm R below; write PIECES.
   OK: y encloses x's represented set, with one rounded enclosure per constructed piece,
   sorted and deduplicated. The exact pre-rounding union equals x's represented set.
   LIMIT: precision, exact-arithmetic bounds, or raw construction count > piece_limit;
   all outputs untouched. piece_limit < 1 gives LIMIT. No other status.
   y may equal x. Source: quotient P6:129, P8:181, P10:249; rounding is Q1 below.
   Cost: O(K) rational operations and O(K log K) sorting; K is the raw construction
   count. Memory O(K b). Canonical finite triples may require CRT for local inputs. */
int adf_qclass_reduce(adf_qclass_t y, const adf_qclass_t x, slong piece_limit, slong prec);
```

Algorithm R, per stored adele `[lo,hi] x (a + N Zhat)`:

1. Read the *exact stored dyadic* endpoints and canonical finite centre and radius.
   This includes any spill in a PIECES input. Do not intersect the stored interval with `[0,1]`.
2. For N = 0 use one finite point a. Otherwise write N = A/B in lowest terms and use
   `a_j = a + j A/B`, `0 <= j < B`, each with radius A (quotient P8).
   For N = 0 set A = 0 and B = 1. Do not factor A or B.
3. Set `l_j = lo-a_j`, `h_j = hi-a_j`. This subtracts the rational that makes the entire finite
   piece integral. For l_j < h_j enumerate `floor(l_j) <= n < ceil(h_j)`; for equality use n = floor(l_j).
   Construct `[max(l_j,n)-n, min(h_j,n+1)-n] x (-n + A Zhat)`.
   Canonicalize -n modulo A when A > 0; retain the integer -n when A = 0.
4. Count with arbitrary-precision integers before allocating. B already lower-bounds the count;
   reject B above the remaining limit before looping. Sum the integer-range lengths with saturation
   at limit+1. Only then convert to slong and check the allocation byte product for overflow.
   This preflight is the one of R5, the second repair of section 2.3; D3-2, taken 2026-10-05,
   requires it wherever a count can be huge. The limit counts this construction, before rounding and
   deduplication. There is no full-image shortcut.
5. Apply Q1 to each exact interval, then sort by the *stored rounded* keys and remove duplicates.
   No merging is required. The invariant is on the midpoint, not on the exact endpoints of the stored ball.

An open shifted interval containing k integers yields k+1 closed pieces. An integer upper endpoint
does not create an additional degenerate piece. A singleton at an integer is represented at real 0.
E1 is the SPEC wrapping example; E2 is `[0,1] x (0 + 2 Zhat)` and constructs one closed piece.
E3, `(0 ; 0 mod 1/2)`, constructs two real points, at 0 and 1/2, both with finite radius 1.
E4 is the zero class, with finite radius 0 throughout. No exact finite point becomes Zhat.

There is deliberately no single-piece-only reduction function in this design. Every reduction result
has a list, even when len = 1. A limit of 1 is a resource request and gives LIMIT when exceeded,
not NEEDS_SPLIT (conventions:181). NEEDS_SPLIT remains the specified status for any future function whose
*result type* permits only one piece. Determining a minimal one-piece representation is not confused with
counting R's construction: quotient P6:148-151 already warns that pieces can merge.

### 2.3 Comparison of represented sets

D3-1 was taken on 2026-10-05 (section 6): these three declarations are an explicit exception to the
status-free predicate row of conventions 3.2, and the same decision amends that row. They are not
declarations of point comparison. All finite canonical input data determine an answer.

```c
/* Write 0 or 1 to truth on OK: respectively equality, first set inside second,
   or nonempty intersection of the represented subsets of A/Q.
   LIMIT leaves truth untouched when Q2's work_limit or exact-arithmetic bounds fail.
   NOT_DETERMINED is not used for represented sets; no unknown endpoints occur in arb.
   Inputs may alias; truth cannot alias any member. Source: quotient P9:212-226,
   extended by Q2 for spill, containment, overlap, and zero finite radii.
   Cost: K exact pieces; O(K log K) endpoint sorting; `(2E+1) K L` fiber work, which is
   `O(K^2 L)` because `E <= 2K`; memory `O(K+L)` plus exact integers. L is the lcm of the positive
   finite moduli. */
int adf_qclass_equal_set(int *truth, const adf_qclass_t x, const adf_qclass_t y, slong work_limit);
int adf_qclass_contains(int *truth, const adf_qclass_t x, const adf_qclass_t y, slong work_limit);
int adf_qclass_overlaps(int *truth, const adf_qclass_t x, const adf_qclass_t y, slong work_limit);
```

Use exact R without real rounding as an internal normalization. Q2 decides all three relations.
The normalization is itself bounded: before the loop of R step 2, reject `B > work_limit`, and
accumulate the integer-range lengths of R step 4 with saturation at `work_limit + 1`. Only a value
that passes this preflight is normalized. The cost of the normalization therefore does not depend
on the numeric value of `denominator(N)`, and a LIFT whose finite radius is `1/10^100` is refused
before the loop rather than after it.
Budget: require raw K <= work_limit, L <= work_limit, and `(2E+1) K L <= work_limit`, where E is
the number of distinct real endpoints. Compare these integer expressions before allocation.
This deliberately conservative budget has no undocumented wall-clock promise. Return LIMIT even if a
special shortcut could decide the same case. A later faster algorithm may preserve or explicitly revise it.
No `prec` argument can improve an already stored ball's endpoints. For uncertain underlying points,
disjointness certifies difference; overlapping nonsingleton sets do not certify equality of those points.

### 2.4 Translation and group arithmetic

SPEC 6 asks for reduction and the character. It does not explicitly require addition or negation APIs,
nor a projection to `R/Z x (finite quotient)`. Addition and negation below are useful optional 3.1 slices.
There is no natural independent finite coordinate at the glued boundary, so no such projection is declared.
Choosing a half-open section does not remove the carry. A quotient ring multiplication is not proposed.

```c
/* y = x + pi(q) = x exactly, preserving representation and all contexts.
   Never fails; y may equal x. Cost one class copy. Do not call adele_add_rat.
   Source: quotient P10.1:236. q is validated by its ordinary canonical precondition.
   Status row: open. conventions:199 lists the types whose ring arithmetic is void (adf_rat,
   adf_fball, adf_adele, adf_cadele); adf_qclass is not among them, while adf_qclass_add
   returns a status. This call stays void until conventions 3.2 is amended; the amendment is
   requested in section 9, R8. */
void adf_qclass_add_rat(adf_qclass_t y, const adf_qclass_t x, const adf_rat_t q);

/* y encloses {-u : u in x}. Negate each stored real interval and finite ball,
   then apply R and Q1. All radii stay exact in the finite coordinate.
   OK writes y; LIMIT for precision, arithmetic bounds, or construction count.
   y may equal x. Source: Q3; quotient P6/P8 applied to the negated representatives.
   Cost: negation of len entries plus reduction, with its count and sorting. */
int adf_qclass_neg(adf_qclass_t y, const adf_qclass_t x, slong piece_limit, slong prec);

/* z encloses {u+v : u in x, v in y}, independently varying even if x == y.
   Pair each stored entry of x with each of y. Before rounding, add real endpoints
   exactly and finite balls tightly: (a+b) + gcd(N,M) Zhat. Then R and Q1.
   OK writes z; LIMIT as for reduce, including the raw pair count > piece_limit.
   z may equal either input, including both. Source: Q3 and precision.md P1:27.
   Cost: len(x)*len(y) rational additions/gcds plus reduction and sorting. */
int adf_qclass_add(adf_qclass_t z, const adf_qclass_t x, const adf_qclass_t y,
                  slong piece_limit, slong prec);
```

Addition deliberately rounds only after rational endpoint addition and splitting. An early arb addition
can change the raw number of crossings. Do not assert exact equality of the *stored* output and exact sum
when Q1 has widened it. Rational translation has a separate exact implementation for precisely this reason.

### 2.5 Value text and dump: missing typed declarations

The classifiers already recognize qclass; typed entry points are absent from `text.h` and `dump.h`.
The following reuse their existing conventions, limits, stage order, string ownership, and binding rules.

```c
/* Read (r ; F) + Q as LIFT, or union(X, ...) + Q as PIECES. Real input is enclosed
   at prec. Validate the raw rational midpoint of each union member, then the
   stored midpoint; normalize finite parts, sort, remove duplicate keys.
   OK writes x. PARSE, LIMIT, UNSUPPORTED, DOMAIN leave x untouched, in the existing
   text stage order. lim->max_items bounds entries before deduplication.
   No member aliasing. Cost: input parsing, exact decimals, O(n log n) sorting.
   Source: conventions 9.2:1191, 9.3:1223, 9.4:1261, 9.5. */
int adf_qclass_set_str(adf_qclass_t x, const char *s, size_t len, slong prec,
                      const adf_text_limits_t *lim);

/* Allocate the value form with flint_malloc; write byte length excluding NUL.
   Sort by exact endpoints of PRINTED real balls and remove equal printed pieces.
   Caller uses adf_str_free. As adele_get_str: NULL and *len=0 if not printable
   under M1-D6; otherwise an enclosure, not a lossless representation.
   Cost: per-piece decimal printing plus O(n log n) printed-key comparisons. */
char *adf_qclass_get_str(size_t *len, const adf_qclass_t x, slong digits);

/* Strict canonical dump load. OK writes x; all other outputs untouched on
   PARSE, LIMIT, UNSUPPORTED, DOMAIN. No normalization or sorting on load.
   binds supplies one matching context per local occurrence, in traversal order;
   the one-context form repeats ctx. Contexts stay caller-owned. No member aliasing.
   Source: conventions 10.1:1411 and 10.2:1427-1480; cost linear in input plus
   exact canonical-key checks. Validate all bytes before any FLINT load call. */
int adf_qclass_load_str(adf_qclass_t x, const char *s, size_t len,
                       const adf_modctx_struct *ctx, const adf_text_limits_t *lim);
int adf_qclass_load_str_binds(adf_qclass_t x, const char *s, size_t len,
                             const adf_modctx_struct *const *binds, size_t nbinds,
                             const adf_text_limits_t *lim);

/* Lossless dump, preserving form, stored order, real bits, backends and contexts.
   Allocate a string; *len excludes NUL; caller frees it. No status. Cost output size.
   Body: qclass lift arch fb, or qclass pieces h (arch fb repeated h times).
   Prefix adf1 Q; arch count is 1. No conversion to the value form. */
char *adf_qclass_dump_str(size_t *len, const adf_qclass_t x);

/* Validate and inspect contexts without constructing a qclass. With descs=NULL,
   write count on OK. Otherwise incoming *nctx is initialized descriptor capacity;
   insufficient capacity gives LIMIT without writes. Other failures and stage
   order match load. Source: conventions 10.2; cost as the strict loader. */
int adf_qclass_dump_inspect(size_t *nctx, adf_ctx_desc_t *descs, const char *s, size_t len,
                           const adf_text_limits_t *lim);
```

`tests/golden/qclass.tsv` is a text test, not a reduction oracle. In particular it accepts balls spilling
past the domain whenever their midpoint is legal. Use conventions 11.3's exactness/enclosure split for
real decimals. The parser must not demand that the real interval itself lies inside `[0,1]`.
For dumps use `tests/golden/dump.tsv` qclass rows, strict byte identity, multiple context bindings,
and M1-D9 exponent limits before semantic validation. No change of the existing dump grammar is needed.

## 3. The additive character, work package 3.2

### 3.1 Definition, phase type, and sources

The sign convention is fixed, not a new decision. Conventions:844 says:

    psi_p(x) = E(fp_p(x)), psi_inf(x) = E(-x), psi(x) = psi_inf(x_inf) product_p psi_p(x_p).

Here `E(t) = exp(2 pi i t)` and fp_p is the p-primary fractional part in `[0,1)`.
Opened source `refs/src/tate-poonen/notes.txt:693-694` says "If F = R, let" followed by
`psi(x) := e^(-2 pi i x)`. Lines 695-700 define the quotient map at Q_p and specify
`psi(1/p^n) = e^(2 pi i/p^n)`, trivial on Z_p; line 699 holds the raised n lost by plain-text extraction.
Lines 733-740 define the unconjugated transform and explicitly distinguish Tate's conjugated convention.
This project keeps `conj(psi(xy))`, so its real kernel is positive and finite kernel negative
(conventions:852-873, CV-54). These are two distinct functions, not two spellings of psi.
The thesis's own section number remains [source pending: lawful readable Tate thesis, section 2.2].
It is not required to establish the project definition, which has the on-disk exposition above.

Analysis Lemma 2:76-101 proves additivity, triviality on Q, and the ball image. Step 4:92-94 proves
that psi descends to `A/Q`: subtract `sum_p fp_p(x_p)`, then `floor(x_inf - q)`, and use the
uniqueness of the representative in `[0,1) x Zhat`. Step 2:87-88 proves only `psi(q) = 1` for
rational `q`. No isolated local factor descends to the quotient: rational translation
changes it, although the product cancels. Accordingly there is no `adf_qclass_psi_tate_at`.

Numerical outputs use ordinary `acb_t`, not `adf_cadele` and not `adf_char`.
An exact root of unity is represented by a canonical `fmpq_t theta` with `0 <= theta < 1`, meaning E(theta).
There is no new owning phase struct or algebraic-number ABI. Exact multiplication is addition modulo 1;
conjugation is negation modulo 1. This is a mathematical phase, not a diagonal rational adele.
Exact evaluation requires a separate phase getter: an acb of a root of unity is generally rounded.
The exact phase need not have prime-power denominator. At a single prime it does.

For `x = (m +/- r ; a + N Zhat)`, set B = denominator(N), including B = 1 when N = 0.
Its exact image is `union_(0<=k<B) E([b+k/B-r,b+k/B+r])`, where b = (a-m) mod 1.
The oracle returns b itself when r = 0 and B = 1. Otherwise it returns
`PhaseImage(base=b, rad=r, order=B)`, a compact exact union of arcs, not its convex hull.
`phase_arcs` expands small cases; `hull` uses Q4 without expansion.

### 3.2 Declarations and statuses

```c
/* z encloses every psi(xi) for xi in the adele x. Use the rectangular hull H of
   Q4 and its specified numerical error allowance, not an arbitrary enclosing square.
   OK writes z, including for fractional finite radius. LIMIT for precision or
   D3-2 exact-arithmetic bounds; NOT_DETERMINED if the numerical certificate cannot
   be obtained as specified below. No output is written on a failure.
   Source: analysis L2:76-91, Q4. Different types: no member aliasing.
   Cost: canonical finite triple, rational modular arithmetic, four real cosine
   evaluations; independent of the numeric value of denominator(N). */
int adf_adele_psi_tate(acb_t z, const adf_adele_t x, slong prec);

/* Same enclosure and cost, but NOT_DETERMINED if denominator(N) > 1, before numerical
   evaluation. Integer N includes zero. Real uncertainty is allowed. LIMIT precedence
   for precision and preflight size checks remains first. CV-07 and CV-59 already
   decide this behavior; it is not an open question. */
int adf_adele_psi_tate_strict(acb_t z, const adf_adele_t x, slong prec);

/* z encloses psi of the whole represented class set: union the exact numerical
   coordinate extrema of all stored entries, then form one rectangular enclosure.
   OK, LIMIT, NOT_DETERMINED as above; every constituent must succeed. Statuses
   combine by maximum. Cost O(len) rational phase reductions and cosine evaluations.
   Source: analysis L2 step 4:92-94 (descent to A/Q) and Q4. No member aliasing. */
int adf_qclass_psi_tate(acb_t z, const adf_qclass_t x, slong prec);

/* D3-3, taken 2026-10-05: apply the strict finite-radius certificate to EACH STORED entry.
   NOT_DETERMINED if any entry has fractional finite radius; otherwise the same
   enclosure as the default. This certifies those representatives, not a singleton
   phase set. A legal PIECES value always passes this particular certificate.
   The status may change under an exact reduction that preserves the image set: the lift of
   E3, whose finite radius is 1/2, is NOT_DETERMINED, and its exact two-piece reduction has
   integral radii and passes, and both have the image {+1,-1}. That sentence is part of the
   contract of this function, not a remark. Other statuses, outputs, cost, aliasing: as
   qclass_psi_tate. */
int adf_qclass_psi_tate_strict(acb_t z, const adf_qclass_t x, slong prec);

/* Local character on a local ball a+p^e Z_p (or an exact rational).
   For e >= 0 or exact input, one phase fp_p(a); for e < 0, all p^(-e) roots
   times that phase. Default encloses the entire rectangular hull; strict returns
   NOT_DETERMINED for e < 0. OK writes z; failures leave it untouched.
   Source: analysis L2:85-86, Q4. Costs: valuation removal, modular inverse, four
   trig evaluations. Check power bit sizes before forming p^k. No member aliasing.
   LIMIT for prec, D3-2 bit bound, or lball's existing exponent bounds; numerical
   certificate failure gives NOT_DETERMINED. Strict e < 0 is tested after bounds. */
int adf_lball_psi_tate(acb_t z, const adf_lball_t x, slong prec);
int adf_lball_psi_tate_strict(acb_t z, const adf_lball_t x, slong prec);

/* psi_v on the projection of x at v. At infinity use E(-I); at a prime use
   a + p^v_p(N) Z_p with N = 0 exact. Both variants permit real uncertainty.
   v is a canonical place handle, an opaque 8-byte value (conventions 7:1028-1037).
   DOMAIN, with where = v, if v is not a place of x. adf_adele has no arch tag and always
   has the archimedean place and every prime (conventions 5.5:569-582, adele.h:119-128), so for
   a canonical v that case is empty for this signature; arch = ADF_ARCH_NONE = 0 belongs to
   adf_sball, whose place set is {infinity} only if arch != 0 (conventions 5.9:697-710), and the
   same sentence covers that type if a later slice adds it. where may be NULL and is untouched
   on OK. No factorization: remove powers of the selected prime from a and N.
   LIMIT precedence and costs as above; no member aliasing. The product over all
   places recovers the adelic character, but arbitrary acb products may be wider. */
int adf_adele_psi_tate_at(acb_t z, adf_place_t *where, const adf_adele_t x, adf_place_t v, slong prec);
int adf_adele_psi_tate_strict_at(acb_t z, adf_place_t *where, const adf_adele_t x, adf_place_t v,
                                 slong prec);

/* Exact phase getters: OK writes canonical theta iff the ENTIRE image is a
   singleton; NOT_DETERMINED otherwise. LIMIT for D3-2 arithmetic bounds, first.
   fball: N integer; adele: also rad(inf)=0; qclass: every entry satisfies that
   condition and all resulting angles agree modulo 1; lball: exact or e >= 0.
   No floating-point evaluation and no prec argument. Source: analysis L2 and Q4.
   Costs: rational reduction per entry; at a prime, one modular inverse after
   removing p-powers. No member aliasing. All failures leave theta untouched. */
int adf_fball_psi_tate_phase(fmpq_t theta, const adf_fball_t x);
int adf_adele_psi_tate_phase(fmpq_t theta, const adf_adele_t x);
int adf_qclass_psi_tate_phase(fmpq_t theta, const adf_qclass_t x);
int adf_lball_psi_tate_phase(fmpq_t theta, const adf_lball_t x);

/* Evaluate E(theta), with theta canonical in [0,1), into an acb enclosure.
   OK writes z; LIMIT for prec or phase size; NOT_DETERMINED for certificate failure.
   One sin/cos evaluation, reduced rational argument, no member aliasing.
   Exact cardinal phases should be assigned as exact complex integers.
   Source: refs/src/flint-3.0.1/arb.rst:1125-1138. */
int adf_phase_get_acb(acb_t z, const fmpq_t theta, slong prec);
```

The four calls `adf_qclass_psi_tate`, `adf_qclass_psi_tate_strict`, `adf_qclass_psi_tate_phase` and
`adf_phase_get_acb` belong to the row "Characters, Gauss sums, local factors" of conventions 3.2
(conventions:223), not to the row "Quotient by `Q`" (conventions:222), because they return
`NOT_DETERMINED`, which the latter row does not contain. conventions:886-887 says the class
character functions are the same functions as the adele ones, which points to the character row.
The status row of `adf_qclass_add_rat`, which is `void`, is a separate open question (section 2.4).

The criterion in the exact phase getters is deliberately stronger than `_strict` on an adele.
For example, E1 passes the strict finite-radius check but has a real arc of possible values.
No nonfinite acb is stored. All numerical evaluators reduce rational angles modulo 1 before conversion,
so a large centre cannot erase a small phase through a low-precision real subtraction.

For a rational local centre a, let p^h be the p-part of its reduced denominator, d the remaining part.
Then `fp_p(a) = (num(a) * d^(-1) mod p^h) / p^h`; h = 0 gives 0.
For an lball whose centre is factored as p^v u, compute this from the negative valuation and the unit
modulo p^(-v), without forming a huge positive power for an integral centre.
Never use the ordinary real fractional part as the local fractional part. `check_phases` checks this
formula against sum of local phases and local additivity at primes including 2.

### 3.3 Numerical accuracy and the rectangular hull

Use Q4 to obtain four exact rational angular distances. A real extremum is a cosine of such a distance,
or its negative. Use `arb_cos_pi_fmpq` on twice that distance, preserving rational argument reduction
(`refs/src/flint-3.0.1/arb.rst:1125-1138`). Treat distances 0, 1/4, 1/2 exactly.

Let epsilon = 2^(-p). Evaluate each extremum with enough guard precision to certify rational lower/upper
bounds no more than epsilon from it, by checking that the resulting arb width is at most epsilon.
Start at min(p+32, ADF_REAL_PREC_MAX), double up to that cap; stop with NOT_DETERMINED if
this certificate is not obtained. A requested p above the limit is LIMIT before all other tests.
The internal cap is not an assertion that the correct phase is undefined.

Clip certified cosine bounds to [-1,1]. For each coordinate take the minimum lower bound and
maximum upper bound over constituents.
Round their midpoint to p bits and the required radius upward to 30 bits, followed by one successor,
as in Q1, using the second form of Q1: a hull coordinate lies in `[-1,1]` and is not a stored piece,
so the midpoint is unrestricted there and the invariant of section 1 does not apply to it. Only the
claim about the midpoint of a stored piece needs `h <= 1`. A zero required radius stays zero.
If the true coordinate width is W, every stored endpoint exceeds its true hull endpoint by at most

    4 epsilon + 2^-28 (W/2 + 2 epsilon).

This follows from Q1: endpoint evaluation adds at most epsilon, midpoint rounding at most epsilon
on [-1,1], and radius storage adds at most `2^-28` times the required radius. The displayed bound
has slack. For a singleton W = 0 it tends to zero with p. For a genuine arc or several roots, the
30-bit radius storage can leave a nonzero excess even as p grows. Do not test convergence to zero
without accounting for that term. Check precision-dependent excess, not just overlap with the hull.

The oracle's `hull` computes the exact-extremum formula numerically at 90 decimal digits. Its independent
check enumerates small arcs and their critical angles, using margin 1e-75. This is not an interval proof
of mpmath. The production certificate must come from arb and the bounds just specified.

### 3.4 What this fixes for 3.3 and 3.4

The existing `adf_char_struct` is `(ulong q, ulong n, int parity, acb_t s)`
(conventions:785-809). It stores the primitive character and conductor q, `t^s chi(u')`, with no hidden
conjugation. Its unit values can reuse a rational phase; its zero value on non-units needs a distinct
zero branch, since zero has no angle. Its nonunitary value t^s belongs in acb, not a phase object.
The future conductor/parity constructor and evaluation API are outside this lane.
[source pending: FLINT dirichlet exponent and Conrey-pairing documentation under refs/]
The oracle's exact `chi_exponent` interpretation follows the existing golden generator and is tested
against those vectors. It is finite evidence, not a sourced proof of the labeling convention.

Keep CV-60's names: `adf_char_gauss_sum` returns
`tau(chi) = sum_(a mod C) chi(a) E(+a/C)`; `adf_char_root_number` returns
`tau/(i^parity sqrt(C))`. The phase getter/evaluator supplies E(+a/C) here.
The local Fourier sum is separately named `G_minus`, using E(-a/p^h).
Conventions:931-940 defines it on the inverse unit character in the local gamma formula.
Never route `adf_char_gauss_sum` through a helper called merely "the Fourier kernel".
An internal helper named `_adf_phase_tate_eval` should mean E(theta); its caller negates theta for
the finite Fourier kernel. A real Fourier kernel likewise negates psi_inf's negative angle.

Opened source `refs/src/flint-3.0.1/acb_dirichlet.rst:358-362` defines the positive exponential sum.
Lines 369-380 distinguish the primitive assumptions, algorithms, and Conrey-number input.
The scope here fixes that sign, exact phase interchange, and `tests/golden/gauss.tsv` acceptance;
it does not design character lowering, Gauss-sum algorithms, or local gamma/epsilon functions.

## 4. Statements to add to quotient.md

These are own proofs, not changes to that file. Q4 also supplies statements absent from analysis Lemma 2.

### Q1. Closed-piece rounding with a legal midpoint

Given exact rational `0 <= l <= h <= 1`, p >= 2, let m be round-to-nearest-even at p significant
binary bits of `(l+h)/2`. Put d = max(m-l,h-m). If d = 0 use radius 0.
Otherwise let u be the least 30-bit binary number >= d and let rho be its least 30-bit successor.
Store `[m-rho,m+rho]`. It encloses `[l,h]`, has midpoint in `[0,1]`, and, writing
eta = abs(m-(l+h)/2), each endpoint excess is at most `2 eta + 2^-28 d`.
The statement and its excess bound use only `l <= m <= h`, so they hold unchanged for
`-1 <= l <= h <= 1`; only the claim about the midpoint needs `h <= 1`. Section 3.3 uses the second
form, for which the midpoint is unrestricted. The hypothesis of the first form is unchanged.

1. Binary rounding to nearest is monotone and fixes 0 and 1. Thus its rounded midpoint stays in `[0,1]`.
   Correct rounding is the on-disk contract `refs/src/flint-3.0.1/arf.rst:24-35`.
2. `d = (h-l)/2 + eta` and rho >= d, so both exact endpoints are enclosed.
3. Let `s = 2^(e-30)` be the spacing of the binade `[2^(e-1), 2^e)` holding `d`; then
   `2s = 2^-28 2^(e-1)`. Since `u` is the least 30-bit number at least `d`, we have `2^(e-1) <= d`
   and `u - d < s`. If `u < 2^e` then `rho = u + s`, so `rho - d < 2s = 2^-28 2^(e-1) <= 2^-28 d`.
   If `u = 2^e` then `d > u - s` and `rho = u + 2s`, so `rho - d < 3s`, while
   `2^-28 d > 2^-28 (2^e - s) = 4s - 2^-28 s > 3s`. So `rho - d <= 2^-28 d` in both cases.
4. Replacing the midpoint creates at most 2 eta of excess at either end. Adding rho-d proves the bound.
5. rho has a 30-bit mantissa and is representable in mag (its format is documented at
   `refs/src/flint-3.0.1/mag.rst:6-15`). Construct it exactly from its mantissa/exponent. Do not assume
   a general mag conversion returns the least bound; the documentation explicitly permits extra ulps.
6. A point with l = h still rounds outward if l is not dyadic. If it is exactly representable at p,
   d = 0 and it stays a point. For E1's right piece `[9/10,1]`, the oracle prints positive upper excess
   at p = 20, 53, 128. This specifies the chosen kernel; it is not a claim about all enclosing kernels.

Check: `round_piece`, `check_rounding`. The initial tight-RU-only candidate failed the p = 20 spill test.
The chosen successor rule makes the permitted outward margin explicit, without changing the test.

### Q2. Decidable set relations for stored balls, including points and spill

Any two finite qclass values have decidable equality, containment and overlap as sets, before a resource
bound is imposed. This includes all valid PIECES balls, not just intervals inside the closed domain.

1. Read exact stored endpoints. Apply P8 and P6 with exact rationals to every interval, including spill.
   P10:249-250 gives the same construction for radius zero. This terminates with finitely many exact
   closed pieces in `[0,1]`, without changing the image set.
2. In the half-open section F, an endpoint `(1,m+N Zhat)` contributes `(0,m-1+N Zhat)`.
   Keep all other real membership inside `[0,1)`. This is precisely P3:71-73, also for N = 0.
3. Collect all real endpoints and 0,1. Membership in each piece is constant on every open gap.
   It suffices to test each endpoint below 1 and one midpoint of every gap. Endpoint inclusion
   must be tested separately; an open-gap sample cannot detect a missing glued singleton.
4. Let L be the lcm of all positive moduli, or 1 if none. At a tested real coordinate, refine each
   positive coset to residues modulo L as in P9:217-219. Store their union R. Store also the finite
   set E of exact integer centers of zero-radius pieces, deleting those covered by R.
5. A positive coset modulo L cannot be covered by finitely many integer points. It contains infinitely
   many distinct integers in that residue, whereas E is finite. Thus a fiber `(R,E)` is inside `(S,D)`
   iff `R subset S` and each e in E is either in D or has residue in S. Mutual inclusion decides equality.
6. Intersection is nonempty iff `R intersect S` is nonempty, `E intersect D` is nonempty, or an exact
   point from one side has residue in the other side's positive cosets. These tests also decide
   intersection in Zhat: positive coset intersections are whole cosets, not only their integer samples.
7. Apply step 5 at every real cell for containment/equality; apply step 6 at any cell for overlap.
   All endpoint and residue comparisons are exact finite computations. A resource limit can prevent
   completion, but there is no mathematical uncertainty caused by the positive radius of an arb.

Check: `normalize`, `compare`, `direct_member`, `check_sets`. The finite tests use rational witnesses;
step 5, not a numerical sample, proves the assertion about all profinite points.

### Q3. Operations on quotient sets

For nonempty representative families X,Y, `pi(X)+pi(Y)=pi(X+Y)` and `-pi(X)=pi(-X)`.

1. If x' = x+q and y' = y+r with q,r rational, then x'+y' = x+y+(q+r).
   Negating changes x' to -x-q. Thus both operations are independent of representatives.
2. Choose x and y independently. Every sum of classes has a representative x+y and conversely.
   Distributing over finite unions gives the pair construction in section 2.4.
3. The real interval sum is `[lo_x+lo_y,hi_x+hi_y]`. For finite balls, the exact sum has radius
   gcd(N,M), including zero, by precision.md P1:27. Negation changes the centre sign and reverses
   the real endpoints. R preserves the quotient set; Q1 only enlarges it.
4. For a diagonal rational q, pi(q) is zero, so exact rational translation is the identity on every set.
   A conversion of q to an adele may lose the diagonal correlation and has no such identity guarantee.

Check: `add`, `neg`, `check_arithmetic`; translation in `check_reduction` and the F2 witness below.

### Q4. Phase extrema, additivity, and width

For the image with centre phase b, radius r >= 0, and order B >= 1, define for each rational t:

    d(t) = max(0, dist(B(t-b), Z)/B - r),
    dist(u,Z) = min(frac(u),1-frac(u)).

Its exact rectangular hull is

    real: [-cos(2 pi d(1/2)), cos(2 pi d(0))],
    imag: [-cos(2 pi d(3/4)), cos(2 pi d(1/4))].

1. Analysis L2:78-91 gives the B evenly spaced centre phases and the real uncertainty r.
   The closest centre to t has circle distance `dist(B(t-b),Z)/B`. Thickening by r makes the
   distance to the image the stated maximum with zero. All these distances are in `[0,1/2]`.
2. Cosine decreases from 1 to -1 as circle distance from 0 increases from 0 to 1/2, and for every
   real `t` one has `dist(t,Z) = 1/2 - dist(t-1/2,Z)`: write `u = t mod 1`, then
   `dist(t,Z) = min(u,1-u)`; for `u < 1/2` it is `min(u+1/2,1/2-u) = 1/2-u`, and for `u >= 1/2` it
   is `min(u-1/2,3/2-u) = u-1/2`, which in both cases equals `1/2 - min(u,1-u)`. The largest circle
   distance from 0 over the image is therefore `1/2 - d(1/2)`, and `cos(2 pi (1/2 - x)) = -cos(2 pi x)`.
   The real maximum occurs at the image point nearest 0 and its minimum is minus the cosine of the
   distance to `1/2`. Sine is cosine shifted by 1/4, and the same identity with `1/4` gives the
   imaginary coordinate. This proves all four extrema.
3. In particular the image is the full circle iff `2r >= 1/B`: the B equal arcs close all the
   gaps. Otherwise the displayed formula still holds for separated arcs and for r = 0.
4. For B = 1, angle interval width w = 2r. The Euclidean diameter of the image is
   `2 sin(pi min(w,1/2))`. If w <= 1/2, two points separated by delta <= w have chord length
   `2 sin(pi delta)` by expanding their squared distance; the maximum occurs at the endpoints.
   If w >= 1/2, the interval contains two antipodal phases and the diameter is 2.
   Each rectangular coordinate width is at most that diameter, but usually smaller.
5. For B > 1, the four extrema give exact coordinate widths without an input-independent disk bound.
   For E3, the width is 2 in the real coordinate and 0 in the imaginary coordinate.
   `check_hulls` compares this formula to enumeration of all arc endpoints and critical angles.
6. Local additivity is analysis L2:85; triviality on Q is :87-88. Hence for independent sets
   `psi(X+Y) = {uv : u in psi(X), v in psi(Y)}`. Multiplying rectangular enclosures may widen this
   set, so tests compare exact phases or exact images before rounding, not acb bit identity.
7. A singleton image occurs iff r = 0 and B = 1. For a union, each image must be a singleton and
   all those singletons must agree. This proves the exact-phase getter criteria.
8. At p, write a = n/(p^h d) with p not dividing d. The difference between a and
   `(n d^(-1) mod p^h)/p^h` is p-integral. That rational lies in `[0,1)` and has p-power
   denominator, hence equals fp_p(a). The quotient `p^e Z_p / Z_p` for e < 0 has all
   residues k/p^(-e), giving the local image and strict criterion.

No external inequality is needed for this width proof. The trigonometric identities follow by multiplying
E(delta) by its conjugate; monotonicity follows from the derivative -sin on `[0,pi]`.

### Q5. Full image with fractional or zero finite radius

For N > 0 rational, `pi([lo,hi] x (a+N Zhat)) = A/Q` iff hi-lo >= N.
For N = 0 and any bounded real interval the image is not all of A/Q.
The positive fractional case extends P7, whose stated hypothesis is a positive integer radius.

1. Let N = A/B be reduced, positive. For `(s,z)` in F, decompose the integral z modulo A Zhat:
   choose an integer c with z-c in A Zhat. This residue decomposition is the same one used in P8.
2. Rational translations q meeting the finite constraint are exactly `a-c+N Z`.
   Indeed z-c lies in A Zhat, hence in N Zhat. The remaining rational difference is in
   `N Zhat intersect Q = N Z` (precision Lemma 1, used already by P7).
3. An interval of length at least N contains a member of every translate of N Z. This proves sufficiency.
4. For necessity suppose width < N. Membership for integer z=c depends on whether
   `[lo-s-a+c,hi-s-a+c]` meets N Z. As s ranges over `[0,1)` and c over all integers,
   s-c ranges over all real numbers. Choose a translate of that shorter closed interval strictly
   inside one gap between successive multiples of N. It has no member of N Z, giving a missing point.
5. For N = 0, step 2 becomes q = a-c for integer finite point c. Taking s in `[0,1)` fixed,
   choose an integer c so that s+a-c is outside the bounded real interval. This class is missing.

The oracle checks positive fractional thresholds separately; E4 refutes extending the test to N = 0.
The design does not implement a full-image shortcut, so this statement cannot silently bypass a piece limit.

## 5. Acceptance tests and faults to plant

These are implementation acceptance requirements, not claims that C or Julia was tested by this lane.
The oracle checks its own exact algorithms against separately expressed membership and extrema tests.
It does not prove correct allocation, aliasing, parser stage precedence, ABI layout, or C rounding.
Every C status test must seed all outputs with sentinels and check unchanged bytes on failure.
Every same-type unary operation runs in-place; binary operations run with each output/input alias and
with all inputs identical. Use nonzero finite radii in the self-alias case to detect lost correlations.


**init, clear, set, swap**. init is LIFT zero; dump identity after copies and swaps. len 0, shallow ownership,
lost local context pointer; allocator checks on clear.

**is_canonical**. forge one tag, len, null pointer, midpoint, triple, order, or duplicate at a time. accepting
midpoint outside range; rejecting legal spill; reading raw local A instead of CRT.

**identical**. copied objects versus different backends of the same set. confusing set equality with identical
representation.

**set_adele, set_rat**. adele copy; every exact diagonal rational maps to zero. rounding during lift; finite
radius zero changed to 1; rational 1/3 converted before quotient.

**form, length, get_piece, layout**. exact fields, index edges, initialized destination; ABI static
assertions. borrowed mutable array accessor, wrong offsets, clobber on bad index.

**set_pieces**. stable first duplicate, global canonical keys, multiple local contexts. wrong tie order,
raw-key sort, limit applied after duplicate removal.

**reduce**. `reduce` and `check_reduction`; exact interval containment after Q1. missing/extra integer
crossing; fractional pieces missing; finite points widened.

**reduce limits**. `check_count_limit`; limit K-1 and K; huge denominator and width. slong overflow,
allocation before count, LIMIT confused with NEEDS_SPLIT.

**reduce rounding**. `round_piece`, Q1 excess bound; p = 20,53,128 on non-dyadic endpoint. Compare the
radius with the exact kernel of Q1, not only with the bound: least 30-bit number at least the required
radius, then its successor, including cases whose nearest 30-bit value lies below the required radius.
inward bound, midpoint outside range, clipping spill, unbounded extra radius.

**equal_set**. `compare`; arbitrary mixed moduli, exact fibers, glued endpoints. The pair
`[1/2,1] x (0 mod 3)` against `[0,1/2] x (2 mod 3)`, which meets only in the glued class, and the
family of it for every centre and modulus from 2 to 8. P9 refinement restricted to
one residue; point comparison substituted.

**contains**. `compare` second result, both directions. reversing first-inside-second; a coset covered by
finite point samples.

**overlaps**. `compare` third result; boundary-only intersections. missing endpoint glue or confusing overlap
with containment.

**add_rat**. representation identical to input for signed and non-dyadic q. any real rounding or change to
pieces.

**add, neg**. `check_arithmetic`, then containment and Q1 excess per constructed interval. wrong finite gcd,
sign of finite shift, early real rounding changing construction.

**set_str, get_str**. all qclass.tsv rows; conventions 11.3 real rules; printed fixed point. range checked on
endpoints, duplicate removal/order before rather than after printing.

**load, load_binds, dump, inspect**. qclass dump.tsv rows, exact dump bytes, binding counts and lifetimes.
loader normalizes bad order; context binding lost; unsafe FLINT call on malformed bytes.

**adele psi default/strict**. `psi`, `hull`, all psi_phases.tsv rows. wrong sign at (0 ; 1/3), wrong root
count, strict writes on ambiguity.

**qclass psi default/strict**. union of per-entry exact extrema; E3 before/after reduction. only first piece
evaluated; rounding spill ignored; unspecified strict behavior.

**lball psi default/strict**. `local_image`; e below, at, above 0, including p = 2. ordinary fractional part
used; branch at e = 0 wrong.

**adele psi_at variants**. product of exact local angles versus global phase. prime and real signs disagree;
finite ball denominator inverted at a bad modulus.

**four phase getters**. exact fractions mod 1; union agrees only if every singleton agrees. ignoring real
radius; returning one phase from a finite family.

**phase_get_acb**. cardinal phases exact, other phases enclosed with section 3.3 excess. confusing pi with
2pi; only overlap checked instead of enclosure.


For phase additivity, test exact rational angles first. Then compare exact unions of arcs for independent
sum inputs with positive finite and real radii. Finally verify each C rectangle against the resulting hull
with the section 3.3 excess bound. A square containing every phase must fail tightness when the hull is a line.
Use a denominator too large to enumerate to distinguish Q4's constant-count formula from an unbounded loop.

The local backend needs at least one reducible raw triple whose canonical d becomes 1. A PIECES
constructor must accept it after canonicalization; a dump must preserve the raw local data and its context.
Do not manufacture an inverse of a denominator modulo a block with which it shares a factor.

Six quotient faults, limited to the implementing slice's files:

1. Use k rather than k+1 pieces. Kill with E1 and `check_count_limit`.
2. Include a spurious upper-end singleton by using floor(hi) instead of ceil(hi)-1. Kill with E2.
3. Glue m to m+1 instead of m-1. Kill with the modulus-3 boundary witness in `check_fault_witnesses`,
   which distinguishes m+1 from m-1. A deleted glue is a different fault: kill it with the glued-pair
   family of the `sets` group, which fails without the two lines of Q2 step 2.
4. Keep only one fractional-radius branch. Kill with E3 and the denominator-3 fault witness.
5. Refine a modulus N to only one residue modulo L. Kill with the mixed-modulus membership witness.
6. Treat finite radius zero as modulus 1. Kill with exact-fiber versus Zhat containment and equality tests.

Six character faults:

1. Reverse the finite sign. Kill with the exact 1/3 phase.
2. Reverse the real sign. Kill with the exact real 1/3 angle in the rational oracle and dyadic C analogues.
3. Use the ordinary fractional part at p. Kill with the exact local 1/6 at 2 witness.
4. Use A rather than B roots for N = A/B. Kill with the 2/3 radius witness.
5. Evaluate only arc endpoints, omitting interior extrema. Kill with the arc about angle 0.
6. Return the whole square for every ambiguous input. Kill with E3's real line hull.

`check_fault_witnesses` evaluates these twelve explicit alternatives. It is not a mutation run on future C.
The implementation lane must actually plant the faults and show rejection before claiming mutant kills.
Do not run a repository-wide mutation sweep. Long fuzzing is left to the orchestrator's overnight run.

The qclass golden check in this oracle verifies the exact set enclosure of each expected printed value,
plus the malformed golden subset. It does not reproduce the library's decimal printer or dump loader.
The phase golden check evaluates all valid finite angle rows exactly. Invalid phase syntax belongs to
the existing finite-ball reader. The Gauss boundary check reads every gauss.tsv row, obtains exact
character angles from python-flint, lowers by exact angle matching on units, and sums at 90 digits.
Its margin is 1e-75. This checks signs and primitive-conductor use, not the future Gauss implementation.

## 6. Decisions D3-1 to D3-3, taken 2026-10-05

Taken by the orchestrator on 2026-10-05 after the review q-review1. They become N-D21 in SPEC 15.4.
The table states what was decided; the questions and the rejected alternatives are kept for the record
and are not open.

| ID | Question | Decision | Alternatives not taken |
|---|---|---|---|
| D3-1 | Bounded set queries? | Status plus truth | Unbounded bool; bounded undecided |
| D3-2 | Construction policy? | R, Q1, explicit bounds | Merged counts; tighter kernel; unbounded work |
| D3-3 | Class strict meaning? | Per stored entry | Singleton image; new provenance fields |

**D3-1**. The three quotient set queries `adf_qclass_equal_set`, `adf_qclass_contains`,
`adf_qclass_overlaps` return a status and write `*truth`: `OK` or `LIMIT`, and `truth` is untouched on
`LIMIT`. The budget is the one of section 2.3, with the preflight of R5. The point comparisons keep
their `CMP` codes. Amend conventions 3.2 so that the row "Set predicates" reads `OK`, `LIMIT` for
these three functions, with `truth` untouched on `LIMIT`; the point comparisons keep their `CMP`
codes. (The amendment is requested here; conventions.md is not changed by this lane.) The rejected
alternatives: status-free 0/1 with potentially very large exact work, or a separately named bounded
query with `NOT_DETERMINED` and a documented status-row exception.

**D3-2**. The construction count is taken before rounding and deduplication, and there is no
full-image shortcut. The rounding kernel is Q1: the required radius rounded up to 30 bits, then its
successor. The explicit work and bit bounds are those of section 1, with the preflight of R5 wherever
a count can be huge. The optional group arithmetic uses the same policy. `LIMIT` is added to the raw
qclass-constructor status row explicitly. The rejected alternatives: count the merged output, which
needs a merging algorithm that does not exist; a tighter radius kernel, which would revise the exact
rounding vectors; unbounded bit work, which contradicts D3-1.

**D3-3**. `_strict` on a class certifies each stored representative: `NOT_DETERMINED` if any stored
entry has a fractional finite radius, and otherwise the same enclosure as the default. It certifies
those representatives, not a singleton phase set, so a legal PIECES value can pass. The status may
change under an exact reduction that preserves the image set, and that sentence is part of the
contract of `adf_qclass_psi_tate_strict` (section 3.2). The exact phase getter keeps its invariant
singleton semantics. The rejected alternatives: define class strict as singleton image, which rejects
real uncertainty that adele strict accepts; or extend the struct and the dump with provenance, which
conflicts with the fixed current form (conventions 5.10) and needs a separate design.


Already decided: two qclass forms and their layout (5.10); closed pieces and midpoint invariant (M0-D4,
CV-45); LIMIT for an explicit piece limit and NEEDS_SPLIT for a single-piece-only result (3.1);
transactional outputs and aliasing (CV-05 to CV-07); default versus strict adele phase (CV-59);
signs and kernel conjugation (M0-D11, CV-54); distinct Gauss/local sum names (CV-60).
These are cited, not reopened. No new choice of Fourier sign is offered.

## 7. Thin slices

The commands here are proposed driver grammar, not commands implemented by this design.
Use the existing driver expression conventions and introduce only the verb needed by each slice.
Each slice adds a Julia test using the exported layout functions and `GC.@preserve` for storage and strings.
No Julia binding may guess a context lifetime or call into an uninitialized output.


**3.1-a, about one lane-hour**. lifecycle, layout, set_adele/set_rat, form/length/get_piece, add_rat;
LIFT-only text printing. `adf qclass '(0 ; 1/3)'`; `psi` predicts its later phase, lift preserves the supplied
fields. Decision: none.

**3.2-a**. fball and adele exact phase getters, phase_get_acb, adele psi default/strict.
Call `adf psi '(0 ; 1/3)'`; use `psi`, `hull`, phase golden rows.
Decision: D3-2 numerical limits and hull rounding, taken.

**3.1-b**. Integer-radius and exact-radius reduction, all lifecycles and access retained.
Call `adf qreduce '(1 +/- 0.1 ; 0 mod 2) + Q' with 2`; E1,E2,E4, count and rounding checks.
Decision: D3-2, taken.

**3.1-c**. fractional-radius reduction and re-reduction of stored spill. same qreduce with E3 and explicit
limit 2; `reduce`, direct membership. Decision: D3-2, taken.

**3.1-d**. raw pieces, full text, dump/inspect/bindings. `adf print 'union((0.5 ; 7)) + Q'`; qclass and dump
goldens. Decision: D3-2 constructor bounds, taken.

**3.1-e**. three bounded set queries. `adf qcontains Q1 with Q2 with LIMIT`; `compare`, exact-cell membership,
translation pairs. Decision: D3-1, taken.

**3.2-b**. class default/strict and phase getter. `adf psi 'union((0 ; 0 mod 1), (0.5 ; 0 mod 1)) + Q'`; E3
image. Decision: D3-3, taken.

**3.2-c**. local character and adele place variants. `adf psi_at '(0 ; 1/3)' with 3`; `local_phase`,
`local_image`. Decision: D3-2, taken.

**3.1-f, optional**. class neg/add. `adf qadd Q1 with Q2 with LIMIT`; `add`, `neg`, constructed-piece
enclosure. Decision: D3-2 scope and policy, taken.


Slice 3.1-a is intentionally a lift-only end-to-end feature. It does not ship a parser that pretends to
handle PIECES: the full qclass grammar remains the separate 3.1-d task. A Julia caller can already inspect
the copied adele, translate its class exactly, and print it through that first slice.
Slices 3.1-b/c can initially receive lifts through set_adele and expose PIECES through get_piece;
their driver needs only lift input and a result printer. Full arbitrary-union input follows in 3.1-d.

Concrete Julia call signatures (after init, with live aligned buffers and the documented C integer widths):

```julia
ccall((:adf_qclass_set_adele, libadf), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}), q, x)
ccall((:adf_qclass_add_rat, libadf), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}), y, q, r)
ccall((:adf_qclass_reduce, libadf), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Clong, Clong), y, q, lim, prec)
ccall((:adf_qclass_equal_set, libadf), Cint,
      (Ref{Cint}, Ptr{Cvoid}, Ptr{Cvoid}, Clong), truth, q, y, work)
ccall((:adf_adele_psi_tate, libadf), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Clong), z, x, prec)
ccall((:adf_qclass_psi_tate, libadf), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Clong), z, q, prec)
ccall((:adf_lball_psi_tate, libadf), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Clong), z, localx, prec)
ccall((:adf_qclass_neg, libadf), Cint, (Ptr{Cvoid}, Ptr{Cvoid}, Clong, Clong), y, q, lim, prec)
ccall((:adf_qclass_add, libadf), Cint,
      (Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}, Clong, Clong), y, q, q2, lim, prec)
```

The foreign test must free every returned string, clear all initialized values on all paths, and compare
status before reading output. The first slice needs none of the D3 decisions.
The 3.3/3.4 lanes should consume phase angles and the sign tests after 3.2-a; they need not wait for
the optional quotient group arithmetic.

## 8. Findings against the specification and conventions

All witnesses are computed by `check_examples_findings`; none changes a source document.
There is no refutation of quotient P2, P3, P5, P6, P8, or P9 under its stated hypotheses.
The old k versus k+1 wording is already repaired. It is not reported again as a new defect.

**F1. The unqualified full-image sentence needs N > 0.** SPEC:402-403 says the image is all of `A/Q`
exactly when the real width is at least `N`; conventions:745 states only the sufficient direction
("when `hi - lo >= N` the image is all of `A/Q`"), not "exactly when". Radius zero is admitted by
the types and P10:249.
E4 has width = N = 0 but is just the zero class; `(1/2 ; 0)` is missing. P7:97,158 assumes a
positive integer radius and is not false. Q5 supplies the positive fractional extension and excludes zero.
If the sentence is read under the preceding positive-radius hypothesis, this is a scope clarification.

**F2. Stored-set equality is confused with comparison of unknown points.** Conventions:753-755 says
equality of unions on balls can return CMP_UNDECIDED. Conventions:200 says set predicates return 0/1;
SPEC:413-415 also permits an undecided ball comparison. The identical non-singleton set
`[0,1] x (0+2 Zhat)` certainly equals itself as a quotient set, and Q2 decides every finite stored case.
There is no uncertainty in its dyadic endpoints. A comparison of two unknown points is a different question.
The correction (D3-1, taken 2026-10-05) is a bounded SET query returning OK with truth or LIMIT,
not a silent use of CMP values as statuses. The existing struct suffices for these set questions.

**F3. Class `_strict` success has no specified representation rule.** Conventions:883-887 fixes a strict
finite-radius criterion for adeles and says the same functions exist for classes. E3's lift has radius 1/2
and fails that criterion. Its exact two-piece representation has integer radii and each piece passes.
Both images are `{+1,-1}`. Thus the rule cannot simultaneously be a per-entry certificate and have success
depend only on the represented subset of A/Q. The struct has no ambiguity provenance. This is a missing
contract, not a refutation of the well-defined default character or analysis Lemma 2. D3-3, taken
2026-10-05, resolves it.

**F4. Literal translation equality needs exact translation before rounding.** SPEC:413-414 and
conventions:756-757 say translation does not change the pieces. P10.2 proves this for exact intervals;
P10.3:240 explicitly gives only containment after outward rounding. Converting a translated zero adele
at q = 1/3 to the oracle's 8-bit midpoint gives m = 171/512 with positive radius
715827883/1099511627776. Its quotient is strictly larger than E4. The oracle checks proper inclusion.
The PLAN equality test must translate exact rational intervals, use dyadic translations exactly
representable in the C test, or call the identity `adf_qclass_add_rat`. It must not assert equality after
an inexact adele translation. The type cannot recover the discarded exact endpoints or diagonal correlation.

**F5. The explanation of CV-45 overstates impossibility.** Conventions:735-739 says that an arb enclosure
of an interval with a non-dyadic endpoint cannot keep inside `[0,1]`. The dyadic ball with midpoint 15/16
and radius 1/16 is `[7/8,1]`; it encloses `[9/10,1]` and stays inside the domain.
What is true is that a generic rounding kernel can spill, and that storing the *exact interval* is
impossible with a non-dyadic endpoint. The chosen Q1 kernel deliberately permits and tests spill.
CV-45 and M0-D4 remain valid, useful enclosure policies; this witness disputes only the universal explanation.

No projection to `R/Z x (finite quotient)` is named in SPEC 6. No API for it is missing from this scope.
The absence of zero-radius and spill cases in P9 is an extension obligation, discharged by Q2, not a
counterexample to P9's explicitly positive-radius, inside-domain hypotheses.

## 9. Repairs after review q-review1

Applied by lane d-quotient-repair, 2026-10-06, to this file and to `proto/quotient3_checks.py`.
One line per repair: the tag, the place, what changed. R11 needed no text change.

- R1, Q1 step 3: replaced the binade argument with the two cases `u < 2^e` and `u = 2^e`. Changed:
  the repair text of the review had the false chain `2s = 2^-28 u <= 2^-28 d` (`u >= d` makes the
  last step false); the applied text is `2s = 2^-28 2^(e-1) <= 2^-28 d`, which is exact, with
  `2^(e-1) <= d`. The conclusion `rho - d <= 2^-28 d` and the numbering are unchanged.
- R2, Q1 statement and section 3.3: added that the enclosure and the excess bound use only
  `l <= m <= h` and therefore hold for `-1 <= l <= h <= 1`, while only the midpoint claim needs
  `h <= 1`; replaced "Use sign symmetry for negative midpoints" in 3.3 by that second form.
- R3, Q4 step 2: inserted the proof of `dist(t,Z) = 1/2 - dist(t-1/2,Z)` with both cases of `u`.
- R4, F1: `SPEC:402-403` is cited for the "exactly when" sentence; `conventions:745` is now cited for
  the sufficient direction only.
- R5, section 2.3: inserted the preflight (reject `B > work_limit`, saturate the R step 4 sum at
  `work_limit + 1`, normalize only what passes) before the budget sentence.
- R6, section 2.3: the cost line now reads `(2E+1) K L` fiber work, `O(K^2 L)` because `E <= 2K`.
- R7, section 3.2: `adf_adele_psi_tate_at` and `_strict_at` take `adf_place_t *where` after the value
  output, return `DOMAIN` with `where = v` if `v` is not a place of `x`, and `where` may be NULL.
  Changed: the review's premise "`conventions 5.5` allows `arch = 0`" is false for `adf_adele`, whose
  struct has no arch tag (`conventions:569`, `adele.h:119-128`); `arch = ADF_ARCH_NONE` belongs to
  `adf_sball` (`conventions:697-710`). The applied text says so and keeps the `DOMAIN` rule.
- R8, section 3.2: added the sentence that the four calls returning `NOT_DETERMINED` belong to the row
  "Characters, Gauss sums, local factors" (`conventions:223`), not to the row "Quotient by `Q`"
  (`conventions:222`). Added the open status-row question of `adf_qclass_add_rat` in section 2.4.
- R9, F4: the line reference is `SPEC:413-414`.
- R10, section 6 and D3-1: the amendment of the row "Set predicates" is part of the decision as taken.
- R12, section 3.1 and `adf_qclass_psi_tate`: the descent to `A/Q` is cited to step 4:92-94, not to
  step 2:87-88, which proves triviality on `Q`.

The decisions D3-1 to D3-3 were taken on 2026-10-05 and are written into section 6 as taken; no
declaration of sections 2.2, 2.3 and 3.2 says "proposal" or "if D3-x is taken" any more.
