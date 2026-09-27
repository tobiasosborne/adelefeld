# Milestone 0 gate review

Date: 2026-09-28. Reviewed: conventions 0.2; SPEC, PLAN and PERF 1.1.
This review changes no specification, prototype, golden vector or reference implementation.

## Verdict

GATE NOT PASSED.

The public header must wait for G1 and G2. G1 leaves scaled arithmetic without a possible result context.
G2 leaves context construction, failure and allocation insufficiently specified for C and foreign callers.
G3 also requires a loader interface decision before that interface is frozen. The repaired interface
contracts need another review. No repaired enclosure formula was refuted. G16 requires a local
repair to the sentence that names the complete canonical triple.

Counts: 2 BLOCKER, 8 MAJOR, 6 MINOR. The numbered findings below are the complete finding set.

## Findings

### G1. BLOCKER: scaled arithmetic cannot obey the context rules

Locations: `docs/conventions.md:284`, `:289`, `:402`, `:433`; `docs/PLAN.md:208`.

Evidence. A scaled value has a non-NULL context pointer and no global backend. The operation table says
to convert contexts K and K' to lcm(K,K') and operate. CV-11 forbids creating that context. CV-12 says
different pointers produce a global result, which this type cannot store. Take contexts 2 and 3, and
two values of scale 1 and residue 0. Their tight sum is Zhat. Neither input context is the prescribed
context 6. The contract does not say that an existing output context is a target argument.

Replacement for the last row of 5.4 and the scope of CV-11/CV-12:

> The implicit global fallback applies only to adf_fball and types containing it. A default binary
> operation on adf_scaled requires the same context pointer in both inputs; otherwise it returns
> ADF_DOMAIN with its value output untouched. This check precedes writes, including aliased writes.
> The result borrows that input context. Exact operands follow the same context rule.
> To combine different contexts, the caller constructs a context with modulus lcm(K,K'), converts
> both operands to it without loss, and then calls the ordinary operation. No operation creates this
> context. A separately named target-context operation may take an explicit caller-owned context;
> its conversion loss and output context must be documented.

Add DOMAIN to the scaled arithmetic status row and add an explicit exception to 3.1 for failure of a
function's stated context-compatibility requirement. Keep other ring arithmetic void and predicates int.
This changes a public function signature and requires checking the resulting header proposal.

### G2. BLOCKER: opaque context construction has no complete life-cycle contract

Locations: `docs/conventions.md:108`, `:220`, `:268`, `:674`, `:683`, `:1339`.

Evidence. Every init is said to establish a valid canonical value. Context init functions can fail on
raw parameters, while every non-OK output is said to be untouched. There is no stated init value for
adf_modctx and no state in which a failed context init may be cleared. An opaque C type also has no
specified allocation API or alignment API: the text offers a size query and an unspecified exported
adf_modctx_t. This is insufficient to define a C stack object or an aligned foreign allocation.

Replacement for context lifetime/construction, preserving caller ownership:

> An adf_modctx_struct is incomplete in the public header. The library exports explicit context
> constructors named adf_modctx_new_*, taking adf_modctx_struct **out as their first argument, and
> adf_modctx_free(adf_modctx_struct *ctx). A successful constructor allocates and fully initializes
> an immutable context and writes its pointer to *out. On failure *out is untouched and no allocation
> is retained. The caller owns the returned context and frees it only after its borrowers are gone.
> Passing NULL to free does nothing. A constructor does not replace or free a previous *out.
> No public by-value or array-of-one context type requires the layout of the incomplete struct.
> Value init functions remain non-failing and require successfully constructed contexts where stated.

If inline context allocation is retained as an alternative, specify size AND alignment, signatures,
and the successful/failed initialization states explicitly. Do not leave this as a binding choice.

### G3. MAJOR: a valid quotient dump can need more contexts than the loader accepts

Locations: `docs/conventions.md:591`, `:916`, `:1220`, `:1264`.

Evidence. The following accepted dump has two canonical pieces, with local contexts 2 and 3:

```text
adf1 Q qclass pieces 2 1 1 -2 0 0 l 1 2 1 2 0 1 3 -2 0 0 l 1 3 1 3 0
```

The midpoints are 1/4 and 3/4. Both finite denominators are 1. No single ctx argument can match both
recorded contexts. A constructor that extracts "the context" from any value dump is likewise undefined.
Also, matching blocks alone cannot restore pointer identity when identical() distinguishes pointers.

Replacement:

> A dump loader takes an array of caller-owned context bindings and its length. There is one binding
> per local-fball or scaled occurrence, in dump traversal order, including nested pieces. Every binding
> must match that occurrence's modulus and ordered blocks. Repeated occurrences may share a pointer.
> A validation/inspection function reports the number of occurrences and their context descriptors
> without constructing a value. The caller explicitly constructs any missing contexts before loading.
> Value fields and backend are restored exactly relative to those bindings. identical() with the
> original value additionally requires binding each occurrence to its original context pointer.

The one-context API may remain a convenience only for dumps whose occurrences all use that context.

### G4. MAJOR: real value text is not a fixed point through an Arb parser

Locations: `docs/conventions.md:1118`, `:1156`; `docs/PLAN.md:158`.

Evidence. `checks/flint_probe.c` runs against installed FLINT 3.0.1. At precision 128, reading
`1 +/- 0.13` produces dump `1 0 10a3d70b -1f`. The specified printer gives `1 +/- 0.14`.
Reading that gives `1 0 23d70a3f -20`, which prints `1 +/- 0.15`. See `checks/roundtrip.out`.
The initial text is also produced by printing the exactly representable ball `1 +/- 1/8`.

This is not specific to arb_set_str. Any stored ball m +/- r containing [0.87,1.13] must have
r >= 0.13 + |m-1|. Its dyadic radius cannot equal the non-dyadic number 0.13, so r > 0.13.
The specified printer's radius is at least r. It cannot print the original radius 0.13.

The proof of fixed-point printing uses an exact rational parser. Arb rounds the decimal radius upward.
Increasing midpoint precision does not make a radius with denominator 100 dyadic. The stated C identity
therefore fails even on this small ordinary ball. The golden-test exception at 11.3 acknowledges the
rounding, but does not repair the unconditional round-trip claim or PLAN's one-change claim.

Replacement for 9.6's second bullet, and the corresponding PLAN sentence:

> With real or complex parts, parsing a printed value encloses the original stored value. The
> print-read-print fixed-point statement applies only to the exact-rational reference parser.
> Repeated C value-text round trips may widen the value and change its text on every pass.
> Dump text is the identity-preserving form. Constrained printing preserves the exact decimal
> interval's sign; at a specified C precision the parser may still return NOT_DETERMINED as in 9.3.

This preserves enclosure. It withdraws a false identity, not an arithmetic test.

### G5. MAJOR: ordinary Arb multiplication need not preserve the idele invariant

Locations: `docs/conventions.md:161`, `:246`, `:510`; `docs/SPEC.md:316`.

Evidence. Let both real input balls be `1 +/- (1 - 2^-30)`. They are finite and positive. With separate
input objects, arb_mul at precision 128 gives `1 0 30000001 -1c`, which contains zero. The probe records
`input_nonzero=1 product_nonzero=0`. The exact product set is positive, with lower endpoint `2^-60`.
The listed statuses for idele/class arithmetic are only OK and exact-zero NOT_UNIT. The ordinary
rounded result cannot be stored in either type, and NOT_UNIT would be false. Higher working precision
alone need not remove this excess from the generic midpoint-radius multiplication algorithm.

Replacement:

> Idele and idele-class arithmetic validates the required real sign on its result before committing
> it. A kernel may use sign-preserving endpoint bounds and a suitable enclosing midpoint-radius ball.
> If it cannot produce a finite ball with the required sign, it returns ADF_NOT_DETERMINED and leaves
> the value output untouched. Failure to preserve a sign under enclosure is never ADF_NOT_UNIT.

Add NOT_DETERMINED to their arithmetic row. Apply the rule to inverse, powers and class conversion too.

### G6. MAJOR: the meaning of a pole status conflicts with the status table

Locations: `docs/conventions.md:140`, `:164`, `:166`, `:246`, `:800`;
`docs/SPEC.md:663`; `docs/proofs/catalogue.md:184`.

Evidence. The real local factor on the real interval `[-1/4,1/4]` contains the pole 0 and regular
points. The definition of DOMAIN requires every input point outside the domain, and explicitly assigns
mixed balls NOT_DETERMINED. The table's only code annotated "a pole" is DOMAIN. Thus two programmers
can return different codes for exactly the same certified pole-containing interval. It cannot receive
a finite enclosure, but it also does not prove that the unknown point is a pole.

Replacement:

> For a local factor or integral, an exact input at a proved nonremovable pole returns ADF_DOMAIN.
> An input ball meeting a pole and also containing regular points returns ADF_NOT_DETERMINED.
> Both leave the value output untouched. The same rule applies if exclusion of poles is undecided.
> No non-finite ball is stored. A separately documented report may distinguish proved intersection
> with a pole from undecided intersection. DOMAIN retains its definition in 3.1.

Use "DOMAIN for an exact pole; NOT_DETERMINED for a mixed/undecided ball" in the affected status rows.

### G7. MAJOR: the polynomial storage and text contracts disagree on exact zero coefficients

Locations: `docs/conventions.md:638`, `:641`, `:1016`, `:1237`;
`proto/text_grammar.py:850`, `:1332`.

Evidence. The accepted reference text for P=[0] retains one coefficient. The C acb_poly probe stores
that polynomial with length 0, violating the proposed length>=1 predicate. P=[1,0] is also preserved
by the parser, while the C polynomial stores length 1. A strict dump containing the supplied lengths
cannot be loaded and dumped byte for byte by these acb_poly operations. Zero polynomials also arise
from ordinary cancellation, not just deliberately non-canonical input.

Replacement for the polynomial invariant and associated grammar:

> P is a normalized acb_poly with length >= 0 and finite coefficients. Its last coefficient, if any,
> is not the exact zero ball. Length 0 represents the zero polynomial. Terms may retain a zero P;
> term order is unchanged. The value grammar admits P=[] and removes exact trailing zero
> coefficients on input; the printer prints the resulting coefficient list. Dump coefficients must
> already have this normalized length; a nonempty list with exact-zero last coefficient is DOMAIN.

Update both parsers and golden vectors together. A ball merely containing zero must not be trimmed.

### G8. MAJOR: dump context counts bypass the promised resource limit

Locations: `docs/conventions.md:955`, `:970`; `proto/text_grammar.py:1235`.

Evidence. With max_items=1, the reference accepts each of these dumps instead of returning LIMIT:

```text
adf1 Q modctx 6 2 2 3
adf1 Q fball l 1 6 2 2 3 0 0
adf1 Q scaled s 1 1 0 6 2 2 3
```

The validator checks piece, term and local-place counts, but never checks the block count in any ctx.
The nested case must be checked before any semantic error, as required by the stage order.

Replacement for the implementation requirement at 8.5 stage 4:

> Walk every context occurrence, including contexts nested in finite balls, adeles and quotient
> pieces. Compare its block count with max_items before semantic validation. Apply all count limits
> to every occurrence regardless of whether another field will later fail a semantic check.

Add the three examples above and a nested-qclass example to tests with a nondefault max_items.
The new probe also confirms the same failure on the nested-qclass dump printed in G13.

### G9. MINOR: the reference does not enforce its shared-context precondition consistently

Locations: `tests/ref/adfref/policies.py:96`, `:105`; `tests/ref/README.md:68`.

Evidence. scaled_add(ScaledBall(1,0,2), ScaledBall(1,0,3)) returns ScaledBall(1,0,2).
The inputs permit the rational choices -2 and 3. Their sum 1 is excluded by the returned ball.
scaled_mul rejects the same context mismatch. README states a shared-K precondition, so this is a
failure to enforce that documented boundary, not a refutation of the same-context sum formula.
It is dangerous as an oracle for the two-context C path promised in conventions 5.4.
Calls satisfying the documented shared-K precondition are not refuted by this example.

Replacement implementation requirement:

> Before the non-exact/non-exact scaled_add path, reject x.K != y.K with ValueError, as scaled_mul
> already does. Add the same regression for scaled_sub. Test separately the lossless conversions
> into a caller-selected common modulus and the subsequent same-context arithmetic.

The current cap and reconstruction conventions are implemented correctly; they do not need changing.

### G10. MAJOR: the phase golden-test contract requires an impossible small ball

Locations: `docs/conventions.md:1319`; `tests/golden/psi_phases.tsv`.

Evidence. The vector `(* ; 0 mod 1/2)` has phases at angles 0 and 1/2, namely +1 and -1.
Any single complex rectangle containing both has real radius at least 1. The test requires that one
returned enclosure contain every phase and also have small radius because the phases are points.
The latter statement confuses each point with their entire image. SPEC 6 requires the whole image.

Replacement for test 11.3 item 6:

> The default enclosure must contain every listed phase. For a single phase, bound numerical error
> by a shrinking precision-dependent tolerance. For several phases, compare with the rectangular
> hull of all listed phases and bound only the excess due to numerical evaluation. The unavoidable
> width of that hull is not an error. The strict variant returns OK exactly for singleton images.

### G11. MAJOR: PLAN 1.8 retains the canonical-data fallback rejected by the raw backend decision

Locations: `docs/PLAN.md:209`, against `:112`, `:128`; `docs/conventions.md:365`, `:389`.

Evidence. PLAN 1.8 says "results whose canonical triple leaves the context are global". For two local
values `(1 + 2 Zhat)/2`, addition produces raw residue 0, denominator 2, modulus 2. Its canonical
triple is (0,1,1). CV-55 and PLAN section 4 keep this result local, but the milestone work item sends
it global. This is the previously repaired distinction of policies P24, lost in one amendment row.

Replacement for that clause of PLAN 1.8:

> Local values retain raw numerator residues and their denominator. Results whose raw set cannot
> be represented in the shared caller-owned context, or whose inputs use different context pointers,
> are global unless the caller supplies a suitable target context. Canonical cancellation alone does
> not force a local value global. Equality and value printing use the canonical triple.

### G12. MINOR: the displayed global predicate has two Boolean readings

Location: `docs/conventions.md:328`.

Evidence. Write C for the common pointer/denominator conditions, P for the positive-radius case, and
E for the exact case. The displayed expression parses either as `(C and P) or E` under ordinary
Boolean precedence, or as `C and (P or E)` under the intended prose grouping. A=1,H=0,d=-1 passes
the first reading because gcd(1,-1)=1, and fails the second. Non-NULL unused fields can pass too.

Replacement:

```text
mctx == NULL and res == NULL and d > 0 and H >= 0 and
((H > 0 and 0 <= A < H and gcd(A,H,d) = 1) or (H = 0 and gcd(A,d) = 1))
```

### G13. MINOR: quotient-piece constraints must name the canonical triple

Locations: `docs/conventions.md:594`, `:615`; `proto/text_grammar.py:1318`.

Evidence. The reference accepts this piece with raw local d=2, although 5.10 literally requires d=1:

```text
adf1 Q qclass pieces 1 1 1 -1 0 0 l 2 6 2 2 3 0 0
```

Its finite set is 3 Zhat, with canonical triple (0,3,1), so it is mathematically a valid integral piece.
CV-55 makes the distinction material. The order keys N,m also cannot mean raw H,residue in one
backend and canonical values in another. The midpoint predicate alone additionally admits arbitrary
wide balls; it does not prove that excess beyond [0,1] came only from rounding an exact reduced piece.

Replacement:

> In the PIECES invariant and order keys, A,H,d always denote the finite part's canonical global
> triple, including when its storage is local. Require d=1, H>=0, and the global centre range of 5.2.
> The real midpoint must lie in [0,1]. These are storage predicates only. A reduction operation must
> separately ensure that each output encloses its constructed exact closed piece; the predicate
> alone does not certify provenance, tightness or a bound on the amount of rounding.

The reduction count and the gluing rule remain unchanged.

### G14. MINOR: returned strings and result channels need fixed signatures

Locations: `docs/conventions.md:153`, `:915`, `:926`, `:1164`, `:1353`; `docs/SPEC.md:716`.

Evidence. Returned strings have only a char* result, yet "contain no NUL"; no output length or explicit
terminator is specified. The promised pointer-and-length interface therefore has two readings.
adf_text_classify returns a type or a status, but no numeric type values or disjoint result channel
are assigned. A foreign caller cannot derive those from the status enum.
Also, 3.2 groups set predicates under "none: void", while 2.3 specifies an int predicate result.

Replacement:

> get_str and dump_str take size_t *len as an output and return an allocated char*. The len bytes
> are ASCII without embedded NUL; a terminating NUL is stored at s[len] and is not counted in len.
> The caller frees the pointer with adf_str_free. adf_text_classify returns an ADF status and writes
> an adf_text_kind enum through an output pointer only on OK. Define that enum explicitly in the
> public header, with one named constant for each supported start symbol.
> Set predicates return int 0 or 1, not a status. Remove them from the void arithmetic row of 3.2
> and give them a separate row with this return convention.

This also resolves the difference from SPEC's pointer-and-length wording without relying on strlen.

### G15. MINOR: the extra real-character root-number assertion lacks its signed evaluation

Locations: `docs/conventions.md:783`; `docs/proofs/analysis.md:564`.

Evidence. The quoted FLINT source defines the finite Gauss sum, and analysis Proposition 13 derives
W_chi=tau(chi)/(i^e sqrt(C)) and its functional equation. For a real character, the product identity
for Gauss sums gives W_chi^2=1; it does not choose between +1 and -1. The additional universal assertion
W_chi=1 needs the signed evaluation of primitive quadratic Gauss sums. The listed finite golden cases
are checks of examples, not a proof for every real primitive character. No cited local passage in
this assertion supplies that evaluation. This finding does not assert a counterexample to W_chi=1.

Replacement for that sentence:

> For a real primitive character W_chi=1
> [source pending: a local source or proof of the signed primitive quadratic Gauss sum evaluation].
> Until that evaluation is supplied, compute W_chi by the same finite Gauss-sum formula used for
> other characters. The listed real-character golden vectors are finite checks, not its proof.

### G16. MINOR: Summary 26 omits centre reduction when it names the full canonical triple

Location: `docs/proofs/policies.md:564`.

Evidence. The canonical-modulus column is correct, but its proof says the raw triple becomes
(A'/g',H'/g',d'/g') by cancellation alone. Squaring 3+4 Zhat gives raw (9,4,1), with g'=1.
The stated triple remains (9,4,1), violating 0<=A<H. The canonical triple is (1,4,1).
P24, which that sentence cites, assumes a lift already reduced to 0<=A<H; S26's products need not be.

Replacement for the first two sentences of the canonical-column proof:

> For a raw triple (A',H',d') with H'>0, first put R=A' mod H' in 0<=R<H'. Set
> g'=gcd(R,H',d')=gcd(A',H',d'). The canonical triple is (R/g',H'/g',d'/g').
> The canonical numerator modulus is therefore H'/g', as displayed in the table.

The remainder of the proof and every formula in the canonical-modulus column can stay.

## Decision table

Accept means accept that design choice; it does not waive a separate finding about its implementation
or an overbroad surrounding sentence. Every rejection has replacement text in the referenced finding.

| Decision | Verdict | Reason or replacement |
|---|---|---|
| CV-01 | accept | Ordered integer statuses permit one deterministic result code. |
| CV-02 | accept | Set predicates and representation identity remain distinct. |
| CV-03 | accept | Keep numbers 0 through 10; reconcile their uses as in G6. |
| CV-04 | accept | Maximum and canonical-place tie-breaking are deterministic. |
| CV-05 | accept | Same-type input/output aliasing works with temporary results. |
| CV-06 | accept | Transactional initialized value outputs; context construction needs G2. |
| CV-07 | accept | Default enclosure and strict variants are distinct operations. |
| CV-08 | accept | Finite-only storage is consistent; G5 extends the result checks to sign. |
| CV-09 | accept | Invariants are public preconditions; text and raw constructors validate. |
| CV-10 | accept | Caller ownership is usable once construction is fixed by G2. |
| CV-11 | reject | Limit the implicit global fallback to types that have it; G1. |
| CV-12 | reject | The global fallback cannot apply to adf_scaled; G1. |
| CV-13 | accept | Exact zero is representable and is the additive identity. |
| CV-14 | accept | The exact tag represents zero and all exact rational results. |
| CV-15 | accept | N=0, c=+1 or -1 gives precisely the required exact rational units. |
| CV-16 | accept | Residues 1..N change the representative, not the unit coset. |
| CV-17 | accept | Supplied modulus is storage; normalized modulus is used for set text. |
| CV-18 | accept | Opaque place handle and one-word Q primes are explicit restrictions. |
| CV-19 | accept | Exact zero at 2 is a valid documented local init value. |
| CV-20 | accept | No minimal function representation; internal acb_poly normalization needs G7. |
| CV-21 | accept | Family constructors know their prime powers; the order is reproducible. |
| CV-22 | accept | An unused A=0 is safe for local storage and uniform cleanup. |
| CV-23 | accept | A cap argument avoids hidden policy state. |
| CV-24 | accept | A fixed storage order; use canonical finite keys as in G13. |
| CV-25 | accept | Pointer-and-length input rejects embedded NUL without truncating. |
| CV-26 | accept | ASCII gives a single byte-level language. |
| CV-27 | accept | Explicit per-call limits; enforce context counts as in G8. |
| CV-28 | accept | Staged status precedence is testable; G8 must apply recursively. |
| CV-29 | reject | Enclosure algorithm accepted; its C fixed-point promise is false; G4. |
| CV-30 | reject | Keep syntax classification, but separate kind from status in the ABI; G14. |
| CV-31 | accept | Exact finite text stays distinct from a global rational. |
| CV-32 | accept | Bare a mod N has an unambiguous finite-ball start symbol. |
| CV-33 | accept | Printing content 1 keeps a uniform idele grammar. |
| CV-34 | accept | Z[1/p] representative and explicit exponent are unique. |
| CV-35 | reject | Repair the polynomial coefficient list and zero case; G7. |
| CV-36 | accept | Leading zeros on input do not create distinct printed numbers. |
| CV-37 | reject | Retain hex fields and counts; fix polynomial lengths and bindings; G3, G7. |
| CV-38 | accept | A strict loader validates; it must not repair a dump silently. |
| CV-39 | reject | One context does not cover all valid dumps; use bindings per occurrence; G3. |
| CV-40 | accept | Fixed value layouts and exported sizes can be used by foreign callers. |
| CV-41 | reject | Keep opacity, but complete the allocation and initialization ABI; G2. |
| CV-42 | accept | The 64-bit restriction is explicit, including FLINT limb types. |
| CV-43 | accept | Exported free function supplies the matching allocator boundary. |
| CV-44 | accept | The FLINT version check is a useful necessary compatibility check. |
| CV-45 | accept | Closed pieces and k+1 are correct; clarify the storage predicate as in G13. |
| CV-46 | accept | Escaped TSV is deterministic and accommodates malformed byte strings. |
| CV-47 | accept | Cap never changes an exact value, including zero times an inexact ball. |
| CV-48 | accept | Default and tight scaled products have distinct documented costs/results. |
| CV-49 | accept | Default and tight unit powers are separate; exponent zero is exact. |
| CV-50 | accept | The repaired smallest-ball division proof closes. |
| CV-51 | accept | The exact stored Arb endpoints define a closed reconstruction interval. |
| CV-52 | accept | Validate everything before passing any field to a FLINT loader. |
| CV-53 | accept | The exponent names fix both maps without unsourced naming conventions. |
| CV-54 | accept | Paired signs, conjugated transform and reflection conversion agree. |
| CV-55 | accept | Raw local data are unique in their context; cancellation is a separate view. |
| CV-56 | accept | A one-word struct can be passed by value without exposing integer meaning. |
| CV-57 | accept | R1 to R9 are all adopted in the intended Q-specific scope. |
| CV-58 | accept | Stored chi is literal; the Tate path explicitly forms conjugate chi. |
| CV-59 | accept | The name identifies the character convention; repair the width test in G10. |
| CV-60 | accept | Positive tau and negative local G_minus are named separately. |

Counts: 60 decisions, 52 accepted, 8 rejected. All 12 previously decided choices are accepted.

## Coverage and statements found in order

### Contracts, grammar, outputs and foreign calls

Read all of conventions 0.2, including every type predicate, init value, status row, aliasing rule,
ownership rule, grammar, normalization, decision and reported finding. Read the reference parser in full.

The global positive-radius canonical form is unique: the radius and the rational progression determine
the least denominator, and reducing the centre then determines A. The exact case is ordinary rational
normalization. The local raw predicate does not require gcd cancellation. Its radius K/d determines d
in a fixed context; the set then determines each numerator residue. Thus CV-55 does not introduce two
different local data records for the same set in that context. Arithmetic must still preserve backend
distinctions in dump text and use a canonical view for cross-backend equality and value text.

The exact scaled tag, exact unit tag, independent archimedean/finite coordinates, local uncertain zero,
sorted distinct places, positive content and class sign rule are consistent. Unit residue 1 at modulus
1 is an intentional representative change from the proof file. It does not change any coset identity.
The absolute-cap invariant R|C is for positive radii; exact values are its stated exception.

The 13 value-form start symbols are disjoint. In particular, a bracket starts either a numeric unit
coset or p=..., and a parenthesized real/complex adele is distinguished from an idele by the finite
field's '*' and unit-coset production. An adele followed by '+ Q' is consumed only by qclass. The
maximal-letter keyword rule and atomic rational/decimal tokens rule out whitespace inside numbers.
No two parses of one complete value text were found. Dump repetition counts resolve the apparent
variable-length groups. G12 concerns a Boolean predicate, not the text language.

The grammar tests pass all 26 tests, including current golden files, exact-reference fixed points,
byte fuzzing and mutations. This does not test the C round trip in G4. The new context-limit examples
in G8 are missing from that passing suite. Reference-only limits on large character moduli and on
materialized Arb exponents are disclosed in proto/text_grammar.py; they are not general C limits.

Transactional value outputs and aliasing are compatible: compute into temporary initialized objects,
validate the result, then swap. A caller can retain an aliased input on failure. G2 is the separate
uninitialized-context problem. For per-place reports, the maximum code and canonical-place tie rule
are deterministic. The finite-only result policy needs the pole clarification G6, not an infinity
sentinel inside finite storage. There is no need to change the numerical status ordering.

The FFI has exported functions, explicit contexts, 64-bit value fields and no variadic or closure
callback requirement. Its unresolved boundaries are G1-G3 and G14. A size query alone is not a proof
of all field offsets or cross-library ABI compatibility; fixed layouts still have to be checked by
the future binding. No Julia or production C ABI smoke test was possible: the public types are not
implemented, as required at this gate. Nemo layout/version compatibility remains explicitly unverified.

### SPEC, PLAN and PERF amendments; old findings

Read SPEC 1.1 and PLAN 1.1, including the review protocol and every changed milestone row. Read PERF 1.1
and checked its new profile against the saved measurement, the compiled multiply chain and cited local
instruction pages. Read earlier review verdicts and closure records to identify matters already settled.

The amendments carry the exact-unit decision, cap exception, closed reconstruction interval, piece
count, tight optional products/powers, idele division radius, conjugate Tate character, reciprocity
names and paired Fourier signs. G11 is the remaining raw/canonical conflict in the PLAN work item.
G4 is also a PLAN error. The source and seams amendments did not lose their three field-generalization
corrections. The symbols and known function domains were not reopened without a new counterexample.

The lane finding audit read every lanes/*/report.md section headed "Findings against the specification"
and the conventions part-B findings. The seven ideles/policies/quotient findings map to M0-D1 through
M0-D7 and the raw-storage amendment. The two Python-reference choices map to M0-D2 and M0-D3. The third
Python item, exact scaled values, is covered by CV-14. Conventions F1-F10 are either adopted decisions
or explicitly recorded differences of representative/naming; G11 is the one lost implementation rule.
The seams corrections to A_K and A_F are present. Sources-lane PERF P1/P2 are addressed by naming the
register form and measured/documented columns. Other lanes report no new specification counterexample.

The Intel figures are still provisional assumptions, not universal hardware floors. The saved multiply
path has the stated 18-cycle path under its stated zero-cost treatment of the immediate add and other
unmodelled instructions. A new own probe on CPU 2 compared 51,200,000 dependent instructions per run:
after the first trial, add-immediate-1 took 0.045736 to 0.046009 ns/instruction and register-add took
0.264163 to 0.264951. The immediate-add result is consistent with the adopted model on this machine;
it is not an independent core-clock measurement. No new arithmetic error was found in the PERF ratios
or I/O calculations. No full benchmark was rerun on a quiet machine, and no new performance claim is made.

### Every substantive SPEC quoted label

All ranges below were opened under refs/src/ and read at physical LF-based line numbers. The extraction
and SHA-256 hashes are in checks/source_audit.out. PDF control characters must not count as new lines.
There are 15 substantive quoted locations in SPEC, with decomposition cited at two of them. The change
log, label definition and catalogue introduction are uses of the label, not additional quotations.

| SPEC line | Local evidence | Result |
|---|---|---|
| 64 | hertogh-thesis/thesis.txt:231; milne-cft/CFT.txt:9373 | Projective limit and restricted product agree. |
| 292 | milne-cft/CFT.txt:9853 | Decomposition is compatible after choosing the positive finite generator. |
| 400 | tate-poonen/notes.txt:693,700,733,740; tate-kudla/kudla-1.txt:705,903 | Signs and conjugation agree. |
| 448 | tate-warwick/tatesthesis_notes.txt:493,512 | Gaussian/zeta factor agrees. |
| 460 | tate-poonen/notes.txt:1720; tate-warwick/tatesthesis_notes.txt:520 | Convergence and equation agree. |
| 660 | flint-3.0.1/ulong_extras.rst:456 | Jacobi routine and its odd-denominator condition agree. |
| 661 | pari-doc/usersch3.tex:9266 | Kronecker exceptional values agree. |
| 662 | hilbert-bristol/lecture19.txt:7,46,89 | Definition, local formulas and product formula agree. |
| 663 | tate-poonen/notes.txt:1733,1014 | Finite and real trivial factors agree. |
| 664 | tate-poonen/notes.txt:1053 | Defines the local Gauss integral; finite sum/sign is derived in the proof. |
| 665 | tate-poonen/notes.txt:933 | Local equation agrees after the declared kernel conversion. |
| 669 | milne-cft/CFT.txt:9853 | Same decomposition evidence as SPEC 292. |
| 671 | milne-cft/CFT.txt:9883,9904,1307 | Reciprocal map and uniformizer/Frobenius vector agree. |
| 755 | pari-doc/usersch3.tex:19478,19556 | GRH caveat and unconditional certification agree. |
| 764 | hertogh-thesis/thesis.txt:374,849,1525 | Tight rules, multiplicative precision and overlap agree. |

The package COMMIT file contains 1acd6362bbedccb35aac7a343eeead17d443b0d1, matching the stated prefix.
The cited Gauss and functional-equation sources do not replace this project's sign calculations.
The source-pending Tate section number and the arithmetic/geometric names remain pending, as stated.

### New and repaired proofs

The following conclusions include reading the repaired proofs step by step, not just running their
authors' tests. The independent searches are in checks/proof_refutations.py and analysis_refutations.py.

| Statement | Conclusion and independent evidence |
|---|---|
| functions 7b | Valid bound. k v-floor(log_p k) is nondecreasing; 711625 exact inequalities passed. |
| functions 15r | Valid. Odd powers permute odd units; 240 finite-ball image comparisons passed. |
| policies L6 | The repaired factor K is present; 1600 representability comparisons passed. |
| policies P10 | Tight scaled product agrees in 1600 cases; cost now correctly depends on modulus size. |
| policies P14 | Positive-radius cap and exact exception agree in 4800 checks. |
| policies P24 | Raw set and canonical triple are distinguished correctly; scalar and block checks passed. |
| policies P25 | Congruence counts and determined-residue criterion agree in 18560 checks. |
| policies S26 | Canonical modulus formulas agree in 4800 checks; full-triple sentence needs G16. |
| ideles P19 | The t,t+M difference repairs minimality; 512 local image-hull comparisons passed. |
| analysis L6 | Ratio is decreasing; its 1/2 margin closes the search. 108 numerical tail comparisons passed. |
| analysis P12 | Unimodal sum <= maximum + integral/spacing; 72 numerical grid comparisons passed. |
| analysis L14 | Both branches and equality threshold close; 72 numerical comparisons passed. |
| analysis P15 | Separation inequality and midpoint remainder close; 162 numerical comparisons passed. |
| catalogue 7 | Cofactor r' is required and now included; 784 conic-solvability comparisons passed. |
| catalogue 9 | Pole set/residues agree in 17 numerical limit checks; API status needs G6. |
| catalogue 13 | CRT centre repair is correct; 504 local image/centre comparisons passed. |
| catalogue 15 | The explicit missing-prime unit proves necessity; 1600 finite action comparisons passed. |

For 7b, "tight" is a name for the improved safe bound, not a proved minimal term count. For example
p=3,v=1,n=5 prescribes five terms, although four suffice by the actual valuations. Neither numbered
claim asserts minimality, so this does not refute it. The proof's two inequalities are valid.

For policies S26, its canonical-column H formulas are correct. G16 records the omitted centre reduction;
its claimed column is unaffected.
The explicit zero scalar is handled before the positive-radius local case. Exact cap preservation
also includes exact zero produced from an inexact operand by multiplication by zero.

For analysis L6, the factor 2 covers both signs of n. Re(C') and |q_j| are kept; the linear exponential
term is not discarded. For L14, the split point and interior maximum are chosen as written. P15 uses
n^2 t >= n^2+t-1, or n^2 t^2 >= n^2+t-1 in the general bound; overcounting the intersection of omitted
regions is harmless. Its complex-power derivative bound uses absolute values and U_l. None of these
repairs yielded a counterexample. The mpmath comparisons are numerical evidence, not certificates.
The author's suite separately ran its 42 certified continuation comparisons and 4 general-bound
comparisons successfully; the whole suite reports 3339 assertions and zero failed groups.

### Why check_general_splitting reports 2.8e-14

The group combines unlike errors in one maximum. Instrumenting its four close() calls gives:

| Comparison | Relative error | Tolerance |
|---|---|---|
| split(s) against independent direct Mellin/Hurwitz-zeta expression | 1.34561344753e-56 | 1e-27 |
| split(1-s) against transformed direct expression | 4.34259156292e-56 | 1e-27 |
| epsilon split(1+epsilon) against b | 4.15162935797e-15 | 1e-12 |
| epsilon split(epsilon) against -a | 2.81379739307e-14 | 1e-12 |

Here epsilon=1e-14. If J(s)=b/(s-1)+H(s) near 1, multiplying by epsilon at s=1+epsilon leaves
b+epsilon H(1+epsilon). Similarly J(s)=-a/s+K(s) near 0 leaves -a+epsilon K(epsilon). Thus these two
errors contain an intentional first-order analytic term. They are not a loss of 41 digits by the
integrator. The direct comparisons still reach the 55-digit arithmetic's scale.

The group is non-vacuous. It first verifies a and b differ. It compares with a direct expression whose
finite sum is expressed using Hurwitz zeta and whose real Mellin integral is computed independently.
Changing only its copied split formula to swap a and b gives relative error 1.01456050167 and is
rejected at tolerance 1e-27: one mutant killed, zero survived. The original file was not changed.
Limitations: this group uses a trivial character and numerical quadrature, and its fixed truncations
are not certified by this group's assertions. Other groups supply nontrivial-character and certified
bound checks. Report the four errors separately when assessing numerical accuracy.

### Seams R1 to R9

All nine recommendations are present in conventions sections 5-7 and 9-10. None is mathematically wrong
in its stated Q-specific scope. R1 and R9 avoid confusing the rational prime with a general place; R2
keeps ideal inclusion rather than scalar ordering; R3 and R4 distinguish archimedean components from
complexification; R5 does not export the rational sign decomposition as a general class coordinate;
R6 removes only residue-field-F_2 factors occurring once; R7 makes no number-field principal-scale
promise; R8 is the correct annihilator condition for the finite phase. The separate real coordinate
can still make a full adelic character uncertain. No missing recommendation was found.

The seams suite ran with zero failures. Its K and F examples include nonprincipal radius sums, the
different, the infinity valuation, finite piece counts and class-coordinate invariance. Its wrong
product-term mutants failed in 118/400 and 142/300 cases; the wrong infinity-residue sign failed in
72/300. This review does not claim to have independently supplied all of the pending background
theorems for later number fields or function fields.

### Python ring reference

Read every module in tests/ref/adfref, the vector format, relevant tests, and the mutation harness
results. The tight global formulas, denominator cancellation, independent rational membership and
closed reconstruction progression match the proofs. Exact values survive the cap. The independent
boundary probe checks 12 exact-cap cases and 27 endpoint cases. The existing suite passes 59 tests;
the mutation harness reports 58 baseline tests, zero failures/errors, 18 killed and zero surviving.
These counts describe different test entry points and are not a discrepancy.

The separate Exact wrapper matches the decided exact tag semantically. Conversion reports loss.
The reference's current scope lacks explicit cross-context conversion, the new tight scaled product
and raw local storage; it must not be treated as an oracle for those unimplemented paths. G9 is the
unsafe mismatch boundary in its existing addition. No generated vector file was rewritten.

## Checks: commands and numerical results

All commands use the repository root as working directory. Python imports were run with -B and, for
subprocess suites, PYTHONDONTWRITEBYTECODE=1. No test was edited outside this review's checks directory.
No package was installed. Processes used at most two cores in total; no computation exceeded 170 s.

Set `gate_checks=docs/reviews/m0-gate/checks` in the commands below. This is only a path abbreviation.
The .out files preserve unwrapped machine output; Markdown prose and check sources use <=116 columns.

| Command | Result |
|---|---|
| `python3 -B $gate_checks/contracts.py` | Exit 0; 14 records; 12 cap and 27 endpoint cases; 4 limit bypasses. |
| `python3 -B $gate_checks/proof_refutations.py` | Exit 0; 752732 checks, 0 failed; S26 counterexample. |
| `python3 -B $gate_checks/roundtrip.py` | Exit 0; 2 C read/print fixed-point counterexamples confirmed. |
| `python3 -B $gate_checks/source_audit.py` | Exit 0; 10 files, 30 source ranges opened and hashed. |
| `python3 -B $gate_checks/run_existing.py text ref mutants` | Exit 0; 26 tests; 59 tests; 18/18 mutants killed. |
| `python3 -B $gate_checks/run_existing.py analysis seams` | Exit 0; 3339 analysis checks; 0 failures. |

The runner executes these exact subprocess commands in sequence, each with timeout 170 seconds:

```sh
python3 -B -m unittest proto/test_text_grammar.py
python3 -B -m unittest discover -s tests/ref/tests
python3 -B tests/ref/mutants.py
python3 -B proto/analysis_checks.py
python3 -B proto/seams_checks.py
```

Their recorded wall times are 12.636, 5.797, 5.486, 54.094 and 10.532 s respectively.
The current golden inventory has 713 vectors. The text suite passes them under its reference rules.

Independent numerical analysis and instrumented splitting command:

```sh
timeout 170s python3 -B $gate_checks/analysis_refutations.py --splitting
```

Exit 0. Numerical bounds: 414 comparisons, 0 failed. Local pole residues: 17 comparisons,
maximum absolute residual error 2.268058839e-20. Splitting: 4 comparisons, errors in the table above;
the swapped-residue mutant is killed. Maximum sampled actual/bound ratios are 1.0 (rounded display)
for L6, 0.9711010642 for L14, 0.9946208754 for the P12 grid, 0.5469584311 and 0.7164913782
for the two P15 tails, and 0.1143523626 for midpoint quadrature. These ratios are not certificates.

Installed-FLINT probe commands:

```sh
cc -O2 -Wall -Wextra $gate_checks/flint_probe.c -o $gate_checks/flint_probe -lflint -lgmp -lmpfr
$gate_checks/flint_probe
```

Compile and run exit 0. FLINT version 3.0.1. Nonzero input/result flags are 1/0. The zero polynomial
has length 0; a [1,0] coefficient list has stored length 1. The two decimal parses have the exact
dump fields recorded in G4. All inputs sent to FLINT were well-formed; malformed loaders were not probed.

Instruction comparison commands:

```sh
cc -O2 -Wall -Wextra $gate_checks/add_immediate_probe.c -o $gate_checks/add_immediate_probe
$gate_checks/add_immediate_probe
```

Compile and run exit 0. Affinity status 0, CPU 2, 5 trials of each of 3 chains, 51,200,000 additions
per chain per trial, checksum 1. Complete timings are in checks/add_immediate_probe.out. This was
an attempt to refute PERF's immediate-add assumption; it did not refute it on this machine.

Execution notes. The first source extraction used Python splitlines(), which counted PDF form feeds
and C0 separators as lines. That diagnostic output was discarded and the reader corrected to split
only on LF before any source judgement was made. The final source audit above uses physical lines.
The initial contract diagnostic had a manually written record total; it was replaced with a counter.
After adding the nested-context and block-cancellation cases, only those two check programs were rerun.
The final structural audit counts the decisions/findings, checks line lengths and hashes the inputs;
its command is `python3 -B $gate_checks/final_audit.py` and its output is checks/final_audit.out.
The intermediate runs found 2, then 1 overlong Markdown rows; they were shortened before the final run.

## Sources pending and work not done

No new external theorem was assumed as a proved library contract. G15 names the added missing source.
Existing pending items were not silently promoted: Tate's own section number/sign attribution;
the arithmetic/geometric reciprocity names; the analytic background and Gamma bounds listed in
analysis.md; catalogue's background Hensel/reciprocity/Gamma/Haar/unit-group imports; the general-field
background in seams.md; and the FLINT aliasing/cleanup/Dirichlet documentation and Nemo compatibility
items already marked in conventions. Some of these may be settled by sources now on disk, but this
review has not audited every pending citation in every proof file. The SPEC quoted-label audit is complete.

No production C type, Python oracle, golden fixture, specification, plan or convention file was fixed.
No Julia binding was implemented. No real parser fuzz campaign against future C code is possible yet.
The numerical checks do not prove analytic inequalities; the written arguments above are the review
of those proofs. Long-run performance and other processors were not measured.

## Findings against the specification

No counterexample was found to SPEC's repaired finite enclosure, unit-coset, character, root or
analytic bound formulas. The operational refinements of conventions are not yet a consistent
implementation contract. G3 qualifies SPEC 10's identity promise with external context bindings;
G5 makes SPEC 5's "rounded as usual" compatible with the idele invariant; G6 assigns precise statuses
to SPEC's pole requirement. G4 refutes the unconditional C text identity in conventions and PLAN,
not SPEC's statement that decimal text is an enclosure. G10 refutes a conventions acceptance test.
G11 is a lost PLAN amendment. G16 repairs a proof sentence, without changing its modulus formula.
The owned report lists the blockers and majors so the orchestrator can apply edits in its own files.
