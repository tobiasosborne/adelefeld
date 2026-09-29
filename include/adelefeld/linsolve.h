/* adelefeld/linsolve.h: linear systems modulo N (milestone S, S.1, slice 1).

   Contract: docs/api-s.md sections 1 and 3 (decisions S-D6, S-D7, S-D8, S-D9, docs/SPEC.md 15.3);
   docs/proofs/solvers.md Definition 2.1 (line 504), Lemma 2.3 (line 551), Proposition 2.4
   (line 586), Algorithm H and Proposition 2.5 (line 620), Proposition 2.6 (line 694),
   Proposition 2.7 (line 755), Algorithm L and Proposition 2.8 (line 789); docs/conventions.md
   2.3, 3.2, 4.1, 4.3. Implemented in src/linsolve.c; tests tests/test_linsolve.c; reference
   proto/solvers_checks.py, functions howell, kernel_cert, linsolve_mod, linsol_check.

   The system is A x = b modulo N: A an integer matrix with r >= 0 rows and c >= 0 columns, b an
   integer column with r entries, N >= 1 any integer (composite, with square factors, N = 1, of
   any size). Entries of A and b are any integers; only their residues modulo N matter. N is never
   factored, no primality is tested or assumed, and there is no word-size path. No routine of
   FLINT for the Howell form is called: the engine is Algorithm H of solvers 2.5 (S-D7), one
   computation modulo N (S-D9).

   The type adf_linsol holds the complete answer to one system: the modulus N, the sizes r, c, a
   kind, and the matrices of solvers P2.6 to P2.8, all with entries in [0, N):

     kind   ADF_LINSOL_COSET (0): the solutions are x0 + S(G); ADF_LINSOL_EMPTY (1): there is none
     G      k by c   always      the Howell form of the kernel K = {x : A x = 0}: the canonical
                                 generators (solvers D2.1, P2.4), 0 <= k <= c
     E, V   e by r, e by c       the certificate of completeness: A V_i = E_i (solvers P2.6)
     x0     c by 1   kind COSET  a particular solution; 0 by 0 for kind EMPTY
     y      r by 1   kind EMPTY  y^T A = 0, y^T b != 0 modulo N (solvers P2.7); 0 by 0 for COSET

   A row of a matrix of generators is a vector of (Z/N)^c; S(G) is the set of the combinations of
   the rows of G with coefficients in Z/N. G, E, V are the rows of the Howell form of [A^T | I_c]
   (solvers P2.6(3)), which is unique (P2.4), so they depend on (A mod N, N) only; x0 is
   determined by step 3 and y by step 4 of Algorithm L. Two results of the library on the same
   input are therefore identical entry by entry.

   Predicate (adf_linsol_is_canonical, what can be tested without the system): N >= 1; kind in
   {0, 1}; r, c >= 0; the shapes above; entries in [0, N); (K1), (K4), (K5) of solvers P2.6.
   Init value: kind COSET, N = 1, r = c = 0, G 0 by 0, E 0 by 0, V 0 by 0, x0 0 by 1, y 0 by 0:
   the solution of the empty system modulo 1. */

#ifndef ADELEFELD_LINSOLVE_H
#define ADELEFELD_LINSOLVE_H

#include <flint/fmpz_mat.h>

#include "adelefeld/common.h"
#include "adelefeld/status.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ADF_LINSOL_COSET 0
#define ADF_LINSOL_EMPTY 1

/* S-D8: the bound of r + c. Above it adf_linsolve_mod returns ADF_LIMIT, decided from the sizes
   before any allocation. A size policy, not a measured budget; it does not bound the bit length
   of N or of the entries. */
#define ADF_LINSOLVE_DIM_MAX 4096

/* Layout (64 bit, fixed by this slice and pinned in tests/test_linsolve.c): 192 bytes,
   alignment 8. kind at 0 (then 4 bytes of padding), N at 8, r at 16, c at 24, G at 32, E at 64,
   V at 96, x0 at 128, y at 160; each matrix is an fmpz_mat_struct of 32 bytes (FLINT 3.0.1,
   fmpz_types.h: entries, r, c, rows). The value owns the memory of N and of the five matrices. */
typedef struct
{
    int kind;
    fmpz_t N;
    slong r;
    slong c;
    fmpz_mat_t G;
    fmpz_mat_t E;
    fmpz_mat_t V;
    fmpz_mat_t x0;
    fmpz_mat_t y;
} adf_linsol_struct;

typedef adf_linsol_struct adf_linsol_t[1];
typedef adf_linsol_struct * adf_linsol_ptr;
typedef const adf_linsol_struct * adf_linsol_srcptr;

/* ---- life cycle (conventions 2.3) ---- */

/* adf_linsol_init(sol): the init value above. Allocates nothing beyond what fmpz_mat_init of a
   matrix without rows does. */
void adf_linsol_init(adf_linsol_t sol);

/* adf_linsol_clear(sol): releases the memory; afterwards sol may only be passed to init. */
void adf_linsol_clear(adf_linsol_t sol);

/* adf_linsol_is_canonical(sol): 1 if the predicate above holds, else 0. Never aborts for an
   initialised object whose matrices are valid fmpz_mat values of any shape. Cost: (K5) is at most
   k^2 c multiplications modulo N; the rest is linear in the size of the matrices. */
int adf_linsol_is_canonical(const adf_linsol_t sol);

/* ---- the solver ---- */

/* adf_linsolve_mod(sol, A, b, N): the set of all x in (Z/N)^c with A x = b modulo N, which is the
   coset x0 + S(G) or empty. A is r by c, b is r by 1; the entries are any integers and are
   reduced modulo N. Algorithm L of solvers 2.8 on Algorithm H of 2.5.

   Statuses, in the order in which they are decided:
     ADF_DOMAIN (edit E-C1): N < 1, or b is not r by 1; sol untouched.
     ADF_LIMIT: r + c > ADF_LINSOLVE_DIM_MAX (S-D8), decided before any allocation; sol untouched.
     ADF_OK: sol written, kind COSET: the solutions are exactly x0 + S(G) (solvers P2.8(3)).
     ADF_NO_SOLUTION: proved; sol written, kind EMPTY, with y (decision S-D6: sol is the report of
       the function and is written on this status too). G, E, V are the kernel certificate of A.
   Never ADF_NOT_DETERMINED, never ADF_NOT_UNIQUE: several solutions are the normal case and are
   described by G. The sizes 0 (r = 0, c = 0), N = 1 and the zero matrix are ordinary inputs
   (solvers P2.8(4)).

   Before the function returns, the result is passed through the conditions of adf_linsol_verify
   (solvers Algorithm L, step 5). For Algorithm H a refusal cannot happen (solvers P2.5, P2.6(3),
   P2.8(1), (2)); if it does, the library has a defect, and the function calls flint_abort with a
   message instead of returning a result that is not proved.

   Aliasing: sol is an output of its own type; A, b and N are inputs and may be the same objects as
   each other where the types allow it (A and b the same matrix when c = 1). The function
   allocates: a table of (r + c)^2 entries, the pending vectors of Algorithm H, and the matrices
   of the result. Cost: solvers P2.5(4), P2.8(5): two runs of Algorithm H on matrices of at most
   r + c columns, O((r + c)^3 log N) multiplications modulo N at most, and the check. */
int adf_linsolve_mod(adf_linsol_t sol, const fmpz_mat_t A, const fmpz_mat_t b, const fmpz_t N);

/* adf_linsol_verify(sol, A, b, N): 1 if sol has the modulus N and the sizes r, c of A, b is r by 1,
   the shapes and entries of sol are those of the predicate, and (K1) to (K5) of solvers P2.6 and
   (K6) A x0 = b modulo N (kind COSET) or (K7) y^T A = 0 and y^T b != 0 modulo N (kind EMPTY) hold;
   else 0. The entries of A and b are reduced modulo N. This verifies a COMPLETE result: the
   conditions imply the whole answer, whatever produced sol (solvers P2.6(1), (2), P2.7(1),
   P2.8(3); decision S-D17). A predicate; never aborts on an initialised sol whose matrices are
   valid fmpz_mat values of any shape. By matrix products and greedy reductions, no elimination.
   Cost: solvers P2.6(4): at most r c (r + c) + k^2 c multiplications modulo N. */
int adf_linsol_verify(const adf_linsol_t sol, const fmpz_mat_t A, const fmpz_mat_t b, const fmpz_t N);

/* ---- accessors ---- */

/* adf_linsol_kind(sol): ADF_LINSOL_COSET or ADF_LINSOL_EMPTY. Constant cost. */
int adf_linsol_kind(const adf_linsol_t sol);

/* adf_linsol_kernel_rows(sol): k, the number of rows of G, 0 <= k <= c (solvers P2.5(3)).
   Constant cost. */
slong adf_linsol_kernel_rows(const adf_linsol_t sol);

/* adf_linsol_get_kernel(G, sol): G = a copy of the Howell form of the kernel, k by c. G must be an
   initialised fmpz_mat of any shape; the function gives it the right shape (fmpz_mat_clear,
   fmpz_mat_init). G may be sol->G itself. Cost: a copy of k c entries. */
void adf_linsol_get_kernel(fmpz_mat_t G, const adf_linsol_t sol);

/* adf_linsol_get_particular(x0, sol): if the kind is COSET, x0 = a copy of the particular solution
   (c by 1, shape set as for get_kernel) and the return value is 1; otherwise 0 and x0 untouched.
   A predicate, not a status. x0 may be sol->x0 itself. */
int adf_linsol_get_particular(fmpz_mat_t x0, const adf_linsol_t sol);

/* adf_linsol_get_dual(y, sol): if the kind is EMPTY, y = a copy of the vector y (r by 1, shape set
   as for get_kernel) and the return value is 1; otherwise 0 and y untouched. A predicate, not a
   status. y may be sol->y itself. */
int adf_linsol_get_dual(fmpz_mat_t y, const adf_linsol_t sol);

/* Layout queries (conventions 12.4, CV-40). Header-inline and exported. */
ADF_INLINE size_t adf_sizeof_linsol(void) { return sizeof(adf_linsol_struct); }
ADF_INLINE size_t adf_alignof_linsol(void) { return ADF_ALIGNOF(adf_linsol_struct); }

#ifdef __cplusplus
}
#endif

#endif /* ADELEFELD_LINSOLVE_H */
