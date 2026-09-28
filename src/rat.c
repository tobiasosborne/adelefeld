/* rat.c: adf_rat, the exact global rational (SPEC 4.1, "Exact rationals are a separate type";
   conventions 5.1 for the struct, the predicate and the init value, 2.3 for the life cycle,
   3.2 for the statuses, 4.1 for the aliasing, 4.3 for the state of the outputs after a status,
   4.4 for invalid input, 12.4 for the layout).

   The value is one canonical fmpq: denominator > 0 and gcd(num, den) = 1, the predicate
   fmpq_is_canonical (/usr/include/flint/fmpq.h:117-118). The exact zero is 0/1, the sign is on
   the numerator, and fmpq_init gives 0/1 (fmpq.h:28-32). Every operation of FLINT that is used
   below takes canonical inputs and produces canonical output (refs/src/flint-3.0.1/fmpq.rst:19-26:
   "all functions in the fmpq module assume that inputs are in canonical form, and produce
   outputs in canonical form"), which is exactly the invariant of conventions 5.1; so no
   canonicalisation is needed after the arithmetic and one is needed only where raw data enter.

   Aliasing. conventions 4.1: an output may be any input of the same type, and inputs may alias
   each other. FLINT states for fmpq_add, fmpq_sub, fmpq_mul and fmpq_div (fmpq.h:200, 212, 230,
   248) "Aliasing between any combination of the variables is allowed" (fmpq.rst:409, and
   the same sentence for the underscore versions at :419-420). fmpq_set and fmpq_swap copy and
   exchange whole fields (fmpq.h:78-88), so they alias too. For fmpq_inv the documentation
   (fmpq.rst:475-478) says nothing about aliasing, and conventions 1 says we do not rely on
   wording FLINT does not state, so adf_rat_inv computes 1/x as fmpq_div by the exact 1, whose
   aliasing is documented; the cost is the init and clear of one fmpq, which for the value 1
   allocates nothing.

   Statuses. The only two that a function of this file can return are ADF_DOMAIN (raw data that
   violate the invariant of 5.1, here only a zero denominator; conventions 4.4) and ADF_NOT_UNIT
   (a divisor or an operand of an inversion that is the exact 0, a proved failure; conventions
   3.1, 3.2 row "Division of adf_rat by an adf_rat"). In both cases the check is made before any
   write, so the output is untouched as conventions 4.3 requires. fmpq_div "Division by zero
   results in an error" (fmpq.rst:408-409), which is why the zero divisor is tested here and
   never handed to FLINT. */

#include <adelefeld.h>

/* A note on -DADF_CHECK_INVARIANTS (conventions 4.4, DECISION CV-09: "With
   -DADF_CHECK_INVARIANTS every public function checks adf_x_is_canonical on entry and calls
   flint_abort with a message"). This lane does not implement it, and the reason is in its
   report: it is one mechanism for the whole library, it costs a gcd on every call, the Makefile
   has no target that builds with the flag, and the mutation run of the default build cannot see
   code inside #ifdef. lanes/m1-rat/check_invariants.c and lanes/m1-rat/check_invariants.sh show
   what the check would do and that the precondition violation of 4.4 is, for adf_rat, silent
   today. The owner of the shared build should add the check in one place for every type. */

/* ---- life cycle (conventions 2.3) ---- */

/* adf_rat_init(x): x = 0, stored 0/1 (conventions 5.1; fmpq_init, fmpq.h:28-32). Never fails. */

void
adf_rat_init(adf_rat_t x)
{
    fmpq_init(x->q);
}

/* adf_rat_clear(x): releases the memory of x; afterwards x may only be passed to init
   (conventions 2.3; fmpq_clear, fmpq.h:34-38). */

void
adf_rat_clear(adf_rat_t x)
{
    fmpq_clear(x->q);
}

/* adf_rat_set(y, x): y = x; y may be x (conventions 2.3 and 4.1; fmpq_set, fmpq.h:78-82). */

void
adf_rat_set(adf_rat_t y, const adf_rat_t x)
{
    fmpq_set(y->q, x->q);
}

/* adf_rat_swap(x, y): exchanges the values, O(1) and without allocation (conventions 2.3;
   fmpq_swap, fmpq.h:84-88, which swaps the two fmpz fields). */

void
adf_rat_swap(adf_rat_t x, adf_rat_t y)
{
    fmpq_swap(x->q, y->q);
}

/* adf_rat_is_canonical(x): the predicate of conventions 5.1; never aborts
   (fmpq_is_canonical, fmpq.h:117-118). */

int
adf_rat_is_canonical(const adf_rat_t x)
{
    return fmpq_is_canonical(x->q);
}

/* adf_rat_identical(x, y): 1 if the numerators and the denominators are equal (the header).
   For this type the representation is canonical, so representation identity and equality of the
   rationals coincide (conventions 2.3 and 5.1); the comparison is nevertheless made on the two
   fields, as fmpq_equal does (fmpq.h:52-56). */

int
adf_rat_identical(const adf_rat_t x, const adf_rat_t y)
{
    return fmpz_equal(fmpq_numref(x->q), fmpq_numref(y->q))
           && fmpz_equal(fmpq_denref(x->q), fmpq_denref(y->q));
}

/* ---- constructors (conventions 3.2 row "Constructors from raw data": OK, DOMAIN) ---- */

/* adf_rat_zero(x): x = 0 (fmpq_zero, fmpq.h:40-45; the exact 0 of conventions 5.1 is 0/1). */

void
adf_rat_zero(adf_rat_t x)
{
    fmpq_zero(x->q);
}

/* adf_rat_one(x): x = 1 (fmpq_one, fmpq.h:47-51). */

void
adf_rat_one(adf_rat_t x)
{
    fmpq_one(x->q);
}

/* adf_rat_set_si(x, n): x = n. fmpq_set_si(res, p, q) "Sets res to the canonical form of the
   fraction p / q" (fmpq.rst:196-198); the canonical form of n/1 is n/1, because gcd(n, 1) = 1,
   so the result is the word n and the header's "Cost: constant" is sound whatever the body of
   _fmpq_set_si does (fmpq.h:123-124). [source pending: the body of _fmpq_set_si, not on disk;
   the FLINT sources under refs/src/ are the documentation only.] */

void
adf_rat_set_si(adf_rat_t x, slong n)
{
    fmpq_set_si(x->q, n, 1);
}

/* adf_rat_set_fmpz(x, n): x = n (fmpq_set_fmpz, fmpq.h:136-139: the numerator is copied and the
   denominator is set to 1). */

void
adf_rat_set_fmpz(adf_rat_t x, const fmpz_t n)
{
    fmpq_set_fmpz(x->q, n);
}

/* adf_rat_set_fmpz2(x, num, den): x = num/den in lowest terms with a positive denominator.
   Status: ADF_OK, x written; ADF_DOMAIN if den = 0, x untouched (conventions 4.4, "Zero
   denominator ... ADF_DOMAIN from constructors"). The check precedes the write, so the output
   is untouched on DOMAIN (conventions 4.3). fmpq_set_fmpz_frac (fmpq.h:142) puts the fraction
   p/q in canonical form, which is one gcd of the sizes of p and q and moves the sign of a
   negative denominator to the numerator. Aliasing: num and den may be the same fmpz; neither is
   of the output type, which conventions 4.1.4 allows without a cast. */

int
adf_rat_set_fmpz2(adf_rat_t x, const fmpz_t num, const fmpz_t den)
{
    if (fmpz_is_zero(den))
        return ADF_DOMAIN;
    fmpq_set_fmpz_frac(x->q, num, den);
    return ADF_OK;
}

/* adf_rat_set_fmpq(x, q): x = q, which may be a non-canonical fmpq. Status: ADF_OK, x written;
   ADF_DOMAIN if the denominator of q is 0, x untouched (conventions 4.4, the same row). The
   copy is a plain field copy (fmpq_set, fmpq.h:78-82) and the reduction is fmpq_canonicalise
   (fmpq.h:115; fmpq.rst:23-26, "the user ... becomes responsible for canonicalising the number
   ... before passing it to any library function"), one gcd of the sizes of the two fields.
   Aliasing: q is an fmpq and x an adf_rat, so conventions 4.1.4 forbids them from aliasing. */

int
adf_rat_set_fmpq(adf_rat_t x, const fmpq_t q)
{
    if (fmpz_is_zero(fmpq_denref(q)))
        return ADF_DOMAIN;
    fmpq_set(x->q, q);
    fmpq_canonicalise(x->q);
    return ADF_OK;
}

/* adf_rat_get_fmpq(q, x): q = x, canonical; q is an fmpq initialised by the caller. The copy
   needs no reduction because x is canonical (conventions 5.1). */

void
adf_rat_get_fmpq(fmpq_t q, const adf_rat_t x)
{
    fmpq_set(q, x->q);
}

/* ---- predicates (conventions 2.3: predicates return int 0 or 1) ---- */

/* adf_rat_is_zero(x): 1 if x = 0. For a canonical fmpq the value is 0 exactly when the
   numerator is 0 (fmpq_is_zero, fmpq.h:63-66; the denominator is 1 then, conventions 5.1). */

int
adf_rat_is_zero(const adf_rat_t x)
{
    return fmpq_is_zero(x->q);
}

/* adf_rat_equal(x, y): 1 if x = y as rationals. Both are canonical, so the two are equal as
   rationals exactly when their two fields are equal (fmpq.h:52-56; the uniqueness of the
   canonical form, which is Lemma 5.2 of conventions 5 for fball and the same argument for
   adf_rat: two canonical pairs num/den with den > 0 and gcd = 1 that describe the same
   rational are equal). */

int
adf_rat_equal(const adf_rat_t x, const adf_rat_t y)
{
    return fmpq_equal(x->q, y->q);
}

/* adf_rat_sgn(x): -1, 0 or 1 (fmpq_sgn, fmpq.h:58-61, which is the sign of the numerator; the
   sign of a canonical fmpq is the sign of the value). */

int
adf_rat_sgn(const adf_rat_t x)
{
    return fmpq_sgn(x->q);
}

/* ---- arithmetic: exact (conventions 3.2 row 1: void) ---- */

/* adf_rat_add(z, x, y): z = x + y, exact and canonical (fmpq_add, fmpq.h:200; fmpq.rst:400-409).
   Aliasing: any (fmpq.rst:409). */

void
adf_rat_add(adf_rat_t z, const adf_rat_t x, const adf_rat_t y)
{
    fmpq_add(z->q, x->q, y->q);
}

/* adf_rat_sub(z, x, y): z = x - y (fmpq_sub, fmpq.h:212; fmpq.rst:400-409). */

void
adf_rat_sub(adf_rat_t z, const adf_rat_t x, const adf_rat_t y)
{
    fmpq_sub(z->q, x->q, y->q);
}

/* adf_rat_mul(z, x, y): z = x * y (fmpq_mul, fmpq.h:230; fmpq.rst:400-409). */

void
adf_rat_mul(adf_rat_t z, const adf_rat_t x, const adf_rat_t y)
{
    fmpq_mul(z->q, x->q, y->q);
}

/* adf_rat_neg(y, x): y = -x. The denominator is positive, so negating the numerator keeps the
   form canonical (fmpq_neg, fmpq.h:90-94). Aliasing: y may be x. */

void
adf_rat_neg(adf_rat_t y, const adf_rat_t x)
{
    fmpq_neg(y->q, x->q);
}

/* adf_rat_div(z, x, y): z = x / y.
   Status: ADF_OK, z written; ADF_NOT_UNIT if y = 0 (conventions 3.2, the division row, and 3.1,
   "proved: the value is not invertible"), z untouched (conventions 4.3). The zero divisor is
   tested before the call because fmpq_div, which would otherwise abort (fmpq.rst:408-409:
   "Division by zero results in an error"), and because the output must not be written on a
   status other than ADF_OK. Aliasing: any (fmpq.rst:409). */

int
adf_rat_div(adf_rat_t z, const adf_rat_t x, const adf_rat_t y)
{
    if (fmpq_is_zero(y->q))
        return ADF_NOT_UNIT;
    fmpq_div(z->q, x->q, y->q);
    return ADF_OK;
}

/* adf_rat_inv(y, x): y = 1/x.
   Status: ADF_OK, y written; ADF_NOT_UNIT if x = 0, y untouched (conventions 3.2, the division
   row, read with x in the place of the divisor). 1/x is computed as the quotient of the exact
   1 by x with fmpq_div, whose aliasing is documented (fmpq.rst:409), rather than with
   fmpq_inv (fmpq.h:245), whose documentation (fmpq.rst:475-478) says nothing about aliasing and
   conventions 1 tells us not to rely on wording FLINT does not state. The exact 1 is a local,
   so nothing is shared between calls and there is no global state (conventions 4.5).
   Aliasing: any, also on the refused path, where the check precedes the write. */

int
adf_rat_inv(adf_rat_t y, const adf_rat_t x)
{
    fmpq_t one;

    if (fmpq_is_zero(x->q))
        return ADF_NOT_UNIT;
    fmpq_init(one);
    fmpq_one(one);
    fmpq_div(y->q, one, x->q);
    fmpq_clear(one);
    return ADF_OK;
}
