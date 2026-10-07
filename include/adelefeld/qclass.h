/* adelefeld/qclass.h: quotient lifts, storage access and reduction.
   Contract: docs/api-3.md sections 1, 2.1, 2.4; docs/conventions.md 5.10, 4.1-4.4, 12.4.
   LIFT denotes pi(piece[0]); PIECES denotes union_i pi(piece[i]), where pi: A -> A/Q.
   PIECES canonicality uses exact (lower real end, upper real end, H, A) keys of the
   canonical global finite triple, strictly increasing. Each midpoint is in [0,1],
   d = 1 and H >= 0. Real spill is permitted (CV-45). This is storage canonicality.
   The array and its initialized adeles are owned; modulus contexts are borrowed.
   All inputs are initialized and canonical except is_canonical's raw fields.
   Same-type outputs may alias inputs, but no output aliases an input's member.
   Status failures leave every output untouched. INV checks readers on entry;
   init, clear, swap and is_canonical have the usual lifecycle exceptions. */
#ifndef ADELEFELD_QCLASS_H
#define ADELEFELD_QCLASS_H

#include "adelefeld/adele.h"
#include "adelefeld/rat.h"
#include "adelefeld/status.h"

#ifdef __cplusplus
extern "C" {
#endif

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

/* Exact-work bounds of algorithm R (docs/api-3.md 1, N-D21). Check projected
   sizes before exact shifts/products. Lifts and identity copies are unrestricted. */
#define ADF_QCLASS_EXP_MAX ((slong) 1048576)
#define ADF_QCLASS_BITS_MAX ((slong) 2097152)
#ifndef ADF_REAL_PREC_MAX
#define ADF_REAL_PREC_MAX 2097152
#endif

/* Reduce every stored adele by algorithm R (docs/api-3.md 2.2); write PIECES.
   OK: the exact pre-rounding union equals x's represented set; y encloses it by Q1,
   with one rounded ball per constructed piece, sorted and deduplicated.
   LIMIT: prec > ADF_REAL_PREC_MAX, exact-work bounds, piece_limit < 1, or the raw
   construction count > piece_limit, BEFORE rounding/deduplication/allocation.
   No other status. On LIMIT y is untouched. y may equal x; no member aliasing.
   Read all stored spill. Integer/exact radii use P6/P10; fractional A/B uses B
   fibres of radius A (P8). Sources: quotient.md:129, :181, :249; CV-45; Q1/R1.
   Cost: O(K) rational operations, O(K log K) sorting, O(K b) memory; local inputs
   may require CRT. Counts are arbitrary-precision until the allocation preflight. */
int adf_qclass_reduce(adf_qclass_t y, const adf_qclass_t x, slong piece_limit, slong prec);

/* Copy stored entry i to a. OK writes a; DOMAIN for i outside [0,len) leaves it untouched.
   This is a representative set whose image contributes to x, not a section on points.
   No member aliasing; cost one adele copy. This accessor is in the constructor/accessor
   status class, not the quotient reduction row of conventions 3.2. */
int adf_qclass_get_piece(adf_adele_t a, const adf_qclass_t x, slong i);

/* y = x + pi(q) = x exactly, preserving representation and all contexts.
   Never fails; y may equal x. Cost one class copy. Do not call adele_add_rat.
   Source: quotient P10.1:236. q is validated by its ordinary canonical precondition. */
void adf_qclass_add_rat(adf_qclass_t y, const adf_qclass_t x, const adf_rat_t q);

/* Return sizeof and alignment of the struct. Header-inline and exported; O(1), no status. */
ADF_INLINE size_t adf_sizeof_qclass(void) { return sizeof(adf_qclass_struct); }
ADF_INLINE size_t adf_alignof_qclass(void) { return ADF_ALIGNOF(adf_qclass_struct); }

/* Write 0 or 1 to truth on OK: respectively equality, first set inside second,
   or nonempty intersection of the represented subsets of A/Q.
   LIMIT leaves truth untouched when Q2's work_limit or exact-arithmetic bounds fail.
   NOT_DETERMINED is not used for represented sets; no unknown endpoints occur in arb.
   Inputs may alias; truth cannot alias any member. Source: quotient P9:212-226,
   extended by Q2 for spill, containment, overlap, and zero finite radii.
   Cost: K exact pieces; O(K log K) endpoint sorting; (2E+1) K L fiber work,
   O(K^2 L); memory O(K+L) plus exact integers. L is the lcm of positive moduli.
   Exact R is unrounded. Raw K, L, (2E+1) K L must all be <= work_limit before
   bulk allocation, even for identical inputs. E counts exact piece endpoints.
   Canonical initialized inputs are required; INV checks both on entry. */
int adf_qclass_equal_set(int *truth, const adf_qclass_t x, const adf_qclass_t y, slong work_limit);
int adf_qclass_contains(int *truth, const adf_qclass_t x, const adf_qclass_t y, slong work_limit);
int adf_qclass_overlaps(int *truth, const adf_qclass_t x, const adf_qclass_t y, slong work_limit);

/* y encloses {-u : u in x}. Negate each stored real interval and finite ball,
   then apply R and Q1. All radii stay exact in the finite coordinate.
   OK writes y; LIMIT for precision, arithmetic bounds, or construction count.
   y may equal x. Source: Q3; quotient P6/P8 applied to the negated representatives.
   Cost: negation of len entries plus reduction, with its count and sorting.
   The count and the order of the checks are those of adf_qclass_reduce: prec, then
   piece_limit < 1, then len > piece_limit, then the raw construction count (N-D21).
   On LIMIT y is untouched. Canonical initialized input; INV checks it on entry. */
int adf_qclass_neg(adf_qclass_t y, const adf_qclass_t x, slong piece_limit, slong prec);

/* z encloses {u+v : u in x, v in y}, independently varying even if x == y.
   Pair each stored entry of x with each of y. Before rounding, add real endpoints
   exactly and finite balls tightly: (a+b) + gcd(N,M) Zhat. Then R and Q1.
   OK writes z; LIMIT as for reduce, including the raw pair count > piece_limit.
   z may equal either input, including both. Source: Q3 and precision.md P1:27.
   Cost: len(x)*len(y) rational additions/gcds plus reduction and sorting.
   The raw count is taken over all pairs before rounding and deduplication (N-D21).
   On LIMIT z is untouched. Canonical initialized inputs; INV checks both on entry. */
int adf_qclass_add(adf_qclass_t z, const adf_qclass_t x, const adf_qclass_t y,
                  slong piece_limit, slong prec);

#ifdef __cplusplus
}
#endif
#endif /* ADELEFELD_QCLASS_H */
