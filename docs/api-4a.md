# Slice 4a

This slice implements docs/api-4.md sections 1-4 and slice 4a of section 9.
It uses the fixed layout of conventions 5.11 and the negative finite kernel of conventions 6.3.
No normal form is introduced. D1-D3 were taken in SPEC 15.4, N-D23. No specification text is changed.

## Lifecycle and representation

adf_ffun_init returns the owned representation (1,1,[0]). adf_ffun_clear releases the DM initialized entries.
After clear only init is allowed. These calls have no status. They use _acb_vec_init and _acb_vec_clear.
adf_ffun_set makes an exact deep copy. Self-copy returns immediately. adf_ffun_swap exchanges the three fields.
Copies and printing have no D1 length cap. A valid allocated input can exceed the arithmetic caps.

adf_ffun_is_canonical checks positive dimensions, DM<2^62, a non-NULL live pointer, and finite entries.
Readable initialized entries and their live capacity are caller preconditions. The predicate never aborts.
adf_ffun_identical checks the dimensions and acb_equal in each corresponding cell. It is representation identity.
It does not decide equality of functions. adf_sizeof_ffun and adf_alignof_ffun are header-inline and exported.

Check: tests/test_ffun.c lifecycle(), aliases_and_precision(), invariants(); tests/julia/ffun.jl.

## Setter and text

adf_ffun_set_acb_vec returns DOMAIN for zero dimensions, invalid length, NULL input, or a nonfinite entry.
It returns LIMIT if the array or byte count is too large. It checks the projected product by division first.
After preflight, it validates every raw entry, then allocates, copies exactly, and swaps once. No rounding occurs.
The input array must not alias any member of the destination. Nonfinite final entries also preserve the output.

adf_ffun_set_str implements the value grammar of design section 2 and conventions 9.2-9.4.
It validates the full syntax before checking exponents, item counts and the dimension product.
The positive dimensions and matching number of entries are checked after the limits. Decimal values use the
existing exact-decimal reader and outward rounding at max(prec,2). The stored dimensions are retained.
Caller text limits apply here. D1's raw-setter cap is not substituted for the caller's max_items.
An unsigned dimension is converted by a bounded digit accumulator, after checking for zero factors.

adf_ffun_get_str prints ffun(D=D, M=M; (real) + (imag)*i, ...), using the decimal printer of conventions 9.5.
The caller owns the returned string and uses adf_str_free. Length excludes NUL. A refused field returns NULL
and length zero before decimal conversion. Printing traverses any valid allocated input without a D1 cap.
The printer's digits precondition is the one in text.h. The six valid golden rows have exactly representable
input decimals, so set_str at 128 bits followed by get_str agrees byte for byte with each expected column.

Check: text() reads all 18 fixed goldens and all 18 generated text records. It checks malformed-status output
bytes, exact representation, syntax-before-limit order, limit-before-domain order, item and exponent limits,
embedded NUL, precision precedence, and refusal of a large stored exponent. No golden file was edited.

## Refinement and sum

adf_ffun_refine implements F1 completely. It requires positive D2,M2 with D dividing D2 and M dividing M2.
It returns DOMAIN otherwise. D1 failure returns LIMIT before allocating. With r=D2/D, its cell k is zero if
r does not divide k. Otherwise it copies f[(k/r) mod DM] exactly. Initialized zero cells supply the holes.
Divisibility detects membership in the old support. Reduction modulo DM selects the old coset. Increasing M
repeats the old array. These are the two separate operations in F1, not alternative refinement rules.

adf_ffun_add computes D2=lcm(Dx,Dy) and M2=lcm(Mx,My), with projected multiplication checked first.
Each cell fetches the corresponding F1 values or exact zero, then uses acb_add at max(prec,2).
Where one value is zero, the other value is copied exactly. This preserves all old members and their sums.
Repeated input balls are independently represented after refinement; correlations are lost as F1 permits.
Inputs may be the same object. The output may be either input or both. A temporary array makes these calls safe.
OK commits. LIMIT and nonfinite-result NOT_DETERMINED preserve the destination.

Check: sum() checks every oracle cell of both refined arrays and their sum, zero identity, representation
commutativity, invalid divisibility, aliasing and preserved failures. aliases_and_precision() adds radii,
self-sum, nonzero imaginary parts, a 2002-bit value, and a projected lcm cap failure.

## Direct transform and enclosure

adf_ffun_fourier implements F4 and analysis Proposition 4, docs/proofs/analysis.md:150-173.
The convention source is refs/src/tate-poonen/notes.txt:693-700,733-740. Project conventions conjugate that
character in the finite transform. The output layout is (M,D), and its cell k encloses

    (1/M) sum_j f[j] E(-jk/L), L=DM.

1. Precision and array size are checked before debug predicates or allocation. The direct work bound is
   L^2<=2^20. It gives j,k<1024, so the machine-word product jk cannot overflow.
2. Reduce the negative residue modulo L. Build and canonicalize its exact rational angle in [0,1).
   No floating point index is used. adf_phase_get_acb evaluates E(theta); it does not choose the Fourier sign.
3. Multiply the full coefficient ball by that phase ball using acb_mul. This includes coefficient uncertainty,
   phase uncertainty and midpoint multiplication rounding. Add using acb_add, which includes accumulator
   and addition rounding. These operations enclose all independent choices at every step.
4. Divide by the exact integer M with acb_div_ui, including final division rounding. The dimensions are
   exchanged. Swap only after every phase call and every output cell has succeeded.
5. Private cleanup also handles a nonfinite intermediate under INV. Public clear still checks its precondition.

The operation returns OK, LIMIT or NOT_DETERMINED as design section 4 states. Phase status failures propagate.
The acb enclosure contract is refs/src/flint-3.0.1/arb.rst:6-12 and acb.rst:6-12. Cardinal phases are exact.
The next transform uses 1/D, so P4's geometric sum gives reflection with coefficient L/(DM)=1.

Check: fourier() reads every function fixture, including the six valid golden inputs and a nonsymmetric complex
array. Exact reference coordinates are reduced cyclotomic polynomials. Other reference coordinates are certified
320-bit balls, or 96-bit balls for the L=1024 delta. Every tested result contains these certificates. Uncertain
inputs additionally enclose four joint corner assignments computed with independent 512-bit sin/cos arithmetic.
The tests include delta_1 at (2,3), exact cardinal reflection, weighted Parseval, and all in-place transforms.
Full 96-bit uncertain oracle rectangles are also stored and checked for overlap; their excess width is not
mistaken for mandatory C width. The loader adds the exact rational conversion error to the declared radius.

The tests also assert a quantitative conservative consequence of F4, coordinate by coordinate. Put
r_j=rad_re(f[j])+rad_im(f[j]), and A=max(1,max_j upper_abs(f[j])). At precision p they require

    radius_coordinate(g[k]) <= 4 sum_j r_j/M + 128 L^2 A 2^-p/M.

Each disk radius is at most r_j. A rectangular enclosure can duplicate a disk allowance into two coordinates.
The input factor 4 covers that enclosure and upward mag rounding. Each phase coordinate's error is at most
(4+2^-27)2^-p by psi.h. Four scalar product terms, accumulator addition, final division, and their directed
errors are bounded by a constant times L^2 A 2^-p. For p>=2 the factor 128 covers the phase allowance and
those scalar errors; the tested L<=1024 mag accumulation has inflation less than 2. This is a conservative
coordinate allowance, not a claim that an acb rectangle's circumscribed disk attains the ideal disk bound.
It excludes arbitrary huge output discs. On exact non-cardinal data, radii strictly shrink at 64,128,212 bits.
For uncertain data the input-radius floor persists. Corner containment detects dropping those radii.

## Limits, costs and choices

The raw setter, refinement and addition have array/work limit 2^20. Direct Fourier also requires L^2<=2^20.
Numerical precision is capped at ADF_REAL_PREC_MAX=2097152, before every other check. Exact index products and
lcm products here fit a word after preflight, hence also meet the 2^20-bit D1 integer limit. Allocation counts
fit slong and byte products fit SIZE_MAX. Canonicality itself does not impose these algorithm limits.
Raw zero dimensions give DOMAIN; otherwise oversized shape gives LIMIT before an inconsistent raw length.
This agrees with the fixed text goldens' limit-before-domain rule. Failure never swaps or changes a count.

Lifecycle and setter cost O(DM), except O(1) init/swap/layout queries. The predicate and identity cost O(DM).
Refinement and addition cost O(D2 M2), counting zero cells. Direct Fourier costs O(L^2) phase evaluations,
products and additions, with O(L) owned output storage and constant scalar temporaries.
Text cost is linear traversal plus the existing exact decimal conversion/printing work.

Choices where the design is silent: sum fetches refined cells directly instead of allocating two intermediate
refinements; this gives the same F1 entries with fewer arrays. Exact zero summands use acb_set rather than
rounding a redundant acb_add. Each direct phase is evaluated afresh; a cached phase table is an alternative.
The simple direct implementation and exact copies were retained. No unweighted FFT is used.

Check: caps() reads every cap fixture. allocation_preflight() observes zero FLINT allocation calls for
L=1025 Fourier and DM=2^20+1 raw setter refusal. Precision 2,53,cap,cap+1 and negative precision are tested.
Under INV, invalid canonical inputs abort, while cap+1 still returns LIMIT before invalid predicates.
A delta at (1,3) at the precision cap returns NOT_DETERMINED when its noncardinal phase cannot be certified.
Both distinct-output and in-place failure preserve the complete original representation. One thousand
init/clear cycles also observe exactly 1000 FLINT allocations and 1000 corresponding frees.

## User calls and scope

The driver supplies ffun F, ffun_add F with G, and ffun_fourier F in scripts and direct shell calls.
Expected driver lines were derived before adding the commands. The delta at (4,1) transforms to [1,-i,-1,i].
Julia allocates aligned byte storage from both layout queries, protects it with GC.@preserve, calls Fourier
at 128 bits, checks reflection and LIMIT preservation, and frees each returned string and every object.

The brief's bare-scalar shell example is outside the fixed grammar; its equivalent complex-entry form works.
Product, other finite algebra, dump/load, integral/norm, real functions, evaluation and Poisson are later slices.

## Findings

No counterexample to SPEC 7 was found. No declaration needs a HEADER-FINDING.
The brief's scalar-entry command conflicts with the complex-entry grammar and golden rejected scalar row.
The implemented equivalent uses (1) + (0)*i and the corresponding complex zeros.

The text oracle has an extra significant-digit heuristic at proto/text_grammar.py:601-604.
With D=0 and M equal to fifty decimal nines, full syntax and caller limits permit the text and DM=0.
The dimension domain rule then gives DOMAIN. The oracle instead gives LIMIT because the digit lengths exceed 40.
The C reader follows the stated product/domain rules. No oracle or fixed golden was changed.

# Slice 4b

This slice implements the remaining finite algebra of docs/api-4.md section 3 and section 9, item 2.
F1-F3 are the finite-index proofs. The public type, text grammar, Fourier kernel and caps stay as in slice 4a.
Every operation uses initialized canonical inputs. Whole-object aliases are permitted. Member aliases are not.
Every failure leaves the destination's fields, storage pointer and stored balls untouched.

## Product and its enclosure

adf_ffun_mul uses D2=lcm(Dx,Dy) and M2=lcm(Mx,My), with the existing lcm and shape preflight helpers.
It fetches the two F1 cells directly, as adf_ffun_add does. It does not allocate two refined input arrays.

1. Put rx=D2/Dx and ry=D2/Dy. A cell k is a support hole when rx or ry does not divide k.
   Its product is exact zero. Otherwise the input indices are (k/rx) mod DxMx and (k/ry) mod DyMy.
2. These are the common-refinement values by F1. Multiply the two complete complex balls at max(prec,2).
   The enclosure contract is refs/src/flint-3.0.1/arb.rst:6-12 and acb.rst:6-12.
3. Order the operands by their stored real midpoint, real radius, imaginary midpoint and imaginary radius.
   This makes the result representation identical when the inputs are exchanged. FLINT radius rounding can
   otherwise differ in its final bits when the operand order changes.
4. If the two source pointers coincide, copy one ball to a scalar temporary before acb_mul.
   The same-pointer shortcut in refs/src/flint-3.0.1/acb.rst:463-468 assumes one mathematical quantity.
   This product instead encloses independently selected input members, also when the objects alias.
   Copying one operand makes that choice independent of storage identity.
5. A private array retains every result until the final finite-entry checks succeed. Commit by one swap.
   A nonfinite result returns NOT_DETERMINED. The private scalar and array are cleared on that branch.

Repeated uncertain values can be chosen independently in the refined representation. That loses correlations,
as F1 allows. Every original pair of functions and its product still belongs to the resulting enclosure.
Exact dyadic products that fit the working precision are exact. No general promise of minimal ball width is made.

Check: tests/test_ffun_algebra.c product() checks all 14 product records, both refined input arrays, every cell,
16 exact rational corner products per cell, radius bounds, commutativity, and all whole-object aliases.
numeric_product() checks 2001-bit coefficients, nonzero radii, precision 2, 53, 4096 and the precision cap.
It also compares aliased inputs with distinct copies of equal complex rectangles.

## Translation, reflection and conjugation

adf_ffun_translate_rat implements F2: D2=lcm(D,den(q)), M2=M, b=D2*q in Z, and output cell k reads k-b.
The denominator is checked before conversion to a word. The numerator can have thousands of bits.
Reduce that numerator modulo D2*M before multiplying by D2/den(q). Both remaining factors are bounded words.
The reduced product is below 2^40; no huge numerator*D2 is formed.

If den(q) divides D, read the original array by this permutation. Otherwise call the existing F1 refinement
operation on a private array, then permute its cells into another private array. This reuses the slice-4a helper.
The extra refinement pass is charged against the work cap. All copied ball bits and exact holes are retained.
The result is f(x-q) also off the grid: F2 proves that no point outside (1/D2) Zhat enters the old support.

adf_ffun_reflect reads cell -k modulo DM, with cell 0 fixed. It does not conjugate values.
adf_ffun_conj conjugates each cell without moving it. Both are exact rectangle bijections and use private arrays.
They preserve the represented family, including radii, and applying either operation twice restores the input.

Check: unary() reads all 218 unary records. Translation includes 0, 1/2, 1/3, -7/6, 5 and a 2000-bit numerator.
It checks each result and each in-place call, then translates by -q and compares with the refined original.
The reflection and conjugation records include a nonsymmetric complex array and delta_1 at (2,3).
Conjugation of real balls is identical. Involution checks compare every stored cell, including radii.

## Rational and idele dilation

adf_ffun_dilate_rat implements F3 with q=s/t reduced, t>0. Its layout is (|s|D,tM), deliberately nonminimal.

1. Compare |s| and t with the allowed dimension quotients before forming either dimension product.
   After those bounds, s fits a signed word and its absolute value is safe to compute.
2. Cell k is initialized zero. Copy a value only when t divides k.
3. For a surviving cell, reduce k/t modulo the old length L=DM. Multiply by the certified unit residue
   modulo L. Rational dilation uses residue 1. For negative s negate this index modulo L.
4. F3 shows that these surviving cells are exactly the old support's inverse image under multiplication by q.
   The new period maps to sM Zhat, inside the old period. All other cells are support holes.
5. Copy complete balls and swap once. This preserves the family exactly for exact inputs and encloses it when
   repeated uncertain values lose correlations. q=0 gives DOMAIN, including when the input function is zero.

adf_ffun_dilate_idele reads only the positive content r and unit coset c U(N). Its finite function ignores inf.
Debug entry validation still requires the whole idele, including inf, to be canonical.

1. Project the content's rational dilation layout with the same helper as rational dilation.
2. If N=0, the exact unit residue is c mod L. No residue enumeration is needed.
3. Otherwise put g=gcd(N,L), calculated after reducing N modulo L. Enumerate v in 0..L-1 with gcd(v,L)=1
   and v=c mod g. This is F3's full compatible image R, with a CRT lift proof in design section 3.
4. Stop and return NOT_DETERMINED upon finding a second residue. A canonical unit coset has a nonempty image.
   A singleton fixes every index; use it in the shared dilation cell helper and commit once.
5. Charge L enumeration indices and every new output cell together before enumeration or output allocation.
   The exact-unit branch charges only output cells. Ambiguity never writes to the destination.

The test is an index certificate. It rejects ambiguous units even if a particular array is constant or zero.
It accepts singleton images beyond the sufficient condition L|N: L=6,c=1,N=3 has the unique residue 1.
The content is r, not its inverse. The sign of inf has no effect on a finite function.

Check: idele() reads all 53 idele records with contents 1, 2, 1/3 and 6/5, exact units 1 and -1,
precise cosets and ambiguous images. It tests both output aliases and changes inf to a negative ball.
It also compares every unit c at N=1..16 and L=1..16 with independent CRT lifts modulo lcm(N,L).
All permitted singleton cases, including L=1, L=2 and L=6,N=3, are checked cell by cell.

Check: covariance() reads both oracle-transform arrays for four exact delta cases, including (2,3) and (4,1).
It compares Fourier after dilation with dilation by 1/q after Fourier, scaled by |q| as a positive rational.
For the finite rational idele |q|_f=1/|q|, so this scalar is |q|_f^(-1).
The C arrays overlap the independent 96-bit certificates and have radii below 2^-90.
Their pairwise differences contain zero. The reference certificates can be wider than the 128-bit C balls;
containment of the whole reference interval is not asserted as a required output width.

## Status, caps, cost and choices

Product first checks the precision cap ADF_REAL_PREC_MAX, then the projected lcm and output dimensions.
The exact operations check array/work sizes and integer bit lengths without forming unbounded products.
Every rational numerator/denominator and each unit integer has at most 2^20 bits.
Dilation compares dimensions by division, then checks DM<=2^20, slong counts and SIZE_MAX byte products.
Preflight precedes INV predicates and allocations. Canonicality itself does not impose these algorithm caps.

Translation uses one charged output pass when the denominator divides D. Otherwise it uses two charged passes:
refinement and permutation. Therefore a refined length above 2^19 gives LIMIT in this implementation.
This is a work-limit choice; its new array shape could fit the separate 2^20 allocation limit.
Non-exact idele units charge old length plus new length; exact units charge only the new length.
All charged loops count zero cells. No unit-image shortcut or equality solver is needed.

Product returns OK, LIMIT or NOT_DETERMINED. Rational dilation additionally has the q=0 DOMAIN case.
Idele dilation returns OK, LIMIT or NOT_DETERMINED. Translation, reflection and conjugation return OK or LIMIT.
All failures preserve the output. Invalid canonical-input preconditions abort under INV when preflight allows entry.

Product costs O(lcm(D)*lcm(M)) acb operations and O(new length) output storage, plus one scalar ball.
Translation costs O(D2*M) copies, with one private output array and an extra refined array when needed.
Reflection and conjugation cost O(DM) copies and output storage. Rational dilation costs O(new length).
Idele dilation adds O(old length) unit-residue tests; all bounded index products are below 2^40.
Large-integer input reductions add their FLINT bit-arithmetic cost. No factorization is performed.

Check: caps() reads all five cap records and checks complete destination snapshots on LIMIT and DOMAIN.
It tests 2000-bit dilation refusals, integer bits above the cap, projected common-refinement failure,
the combined idele work cap, the translation two-pass cap, and valid allocated arrays above the arithmetic cap.
The exact integer-bit boundary is accepted. A projected lcm of 2^64+1 is refused before INV and multiplication.
The INV child tests cover every input and output position and invalid rational/idele components.
They check the operation's entry diagnostic, so an abort in a later swap does not satisfy the test.
guarded_fourier() places a protected page before each four-cell array. It exposes reads before the array
inside the uninstrumented FLINT shared library. Hooks are restored after all guarded allocations are freed;
the allocator interface is refs/src/flint-3.0.1/memory.rst:9-24.

## User calls and findings

The driver adds ffun_mul F with G, ffun_translate F with q, ffun_dilate F with q,
ffun_dilate_idele F with A, ffun_reflect F and ffun_conj F. Direct shell calls use the same parser.
The hand-derived fixture includes a delta with D!=M dilated by 2/3 and an ambiguous unit-image refusal.
Julia calls all six symbols with storage allocated through the exported layout queries and frees every object.

No counterexample to SPEC 7 or F1-F3 was found. No declaration needs a HEADER-FINDING.
The brief's unrestricted identity f*1_Zhat=f is false. For delta_1 at (2,3), f(1/2)=1 but 1_Zhat(1/2)=0.
The product is zero. The identity is tested only for functions supported in Zhat, and the counterexample is tested.
No oracle or fixed golden was changed. The analytic source obligations of later slices remain outside this slice.
