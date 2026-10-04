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
