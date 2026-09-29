/* linsolve.c: linear systems modulo N (milestone S, S.1, slice 1).

   The contract is the comment block of include/adelefeld/linsolve.h: docs/api-s.md sections 1 and 3
   (decisions S-D6, S-D7, S-D8, S-D9). The statements are those of docs/proofs/solvers.md section 2
   (cited below as solvers.md:<line>); the reference is proto/solvers_checks.py (howell at line 759,
   reduce_by 827, is_howell 841, kernel_cert 972, linsolve_mod 987, linsol_check 1006); the tests
   are tests/test_linsolve.c.

   Everything is computed with fmpz modulo N, for every N >= 1: no factorisation of N, no test of
   primality, no word-size path (solvers.md:497, 947). No routine of FLINT for the Howell form is
   called (S-D7). FLINT routines used: fmpz_xgcd (flint-3.0.1:fmpz.rst:1065: a f + b g = d =
   gcd(f, g)), fmpz_mod (fmpz.rst:880: the remainder is non-negative for a positive modulus),
   fmpz_fdiv_q (fmpz.rst:802), fmpz_divexact and fmpz_divisible (fmpz.rst:852, 868), and fmpz_mat
   for storage only (fmpz_mat.rst:77; a matrix with zero rows or columns is valid, fmpz_mat.rst:356
   to 359). Every loop of this file treats the sizes 0 itself; no empty matrix is passed to a FLINT
   routine other than init, clear and swap. */

#include <flint/fmpz_vec.h>

#include <adelefeld.h>

/* ---- life cycle ---- */

void
adf_linsol_init(adf_linsol_t sol)
{
    sol->kind = ADF_LINSOL_COSET;
    fmpz_init_set_ui(sol->N, 1);
    sol->r = 0;
    sol->c = 0;
    fmpz_mat_init(sol->G, 0, 0);
    fmpz_mat_init(sol->E, 0, 0);
    fmpz_mat_init(sol->V, 0, 0);
    fmpz_mat_init(sol->x0, 0, 1);
    fmpz_mat_init(sol->y, 0, 0);
}

void
adf_linsol_clear(adf_linsol_t sol)
{
    fmpz_clear(sol->N);
    fmpz_mat_clear(sol->G);
    fmpz_mat_clear(sol->E);
    fmpz_mat_clear(sol->V);
    fmpz_mat_clear(sol->x0);
    fmpz_mat_clear(sol->y);
}

/* ---- vectors and matrices modulo N ---- */

/* the index of the first non-zero entry of v[0 .. n-1], or n */
static slong
first_nonzero(const fmpz * v, slong n)
{
    slong j = 0;

    while (j < n && fmpz_is_zero(v + j))
        j++;
    return j;
}

/* v = (s v) mod N, entry by entry */
static void
vec_scalar_mod(fmpz * v, const fmpz_t s, slong n, const fmpz_t N)
{
    slong j;

    for (j = 0; j < n; j++)
    {
        fmpz_mul(v + j, v + j, s);
        fmpz_mod(v + j, v + j, N);
    }
}

/* v = (v - q w) mod N */
static void
vec_submul_mod(fmpz * v, const fmpz * w, const fmpz_t q, slong n, const fmpz_t N)
{
    slong j;

    for (j = 0; j < n; j++)
    {
        fmpz_submul(v + j, q, w + j);
        fmpz_mod(v + j, v + j, N);
    }
}

/* 1 if every entry of M is in [0, N) */
static int
mat_reduced(const fmpz_mat_t M, const fmpz_t N)
{
    slong i, j;

    for (i = 0; i < M->r; i++)
        for (j = 0; j < M->c; j++)
            if (fmpz_sgn(fmpz_mat_entry(M, i, j)) < 0 || fmpz_cmp(fmpz_mat_entry(M, i, j), N) >= 0)
                return 0;
    return 1;
}

/* R = M mod N, entry by entry; R is initialised here with the shape of M */
static void
mat_init_mod(fmpz_mat_t R, const fmpz_mat_t M, const fmpz_t N)
{
    slong i, j;

    fmpz_mat_init(R, M->r, M->c);
    for (i = 0; i < M->r; i++)
        for (j = 0; j < M->c; j++)
            fmpz_mod(fmpz_mat_entry(R, i, j), fmpz_mat_entry(M, i, j), N);
}

/* the greedy reduction of Lemma 2.3(2) (solvers.md:556 to 559): for the rows i = start, ..., k - 1 of H
   in this order, if the pivot h_i of row i divides the entry v[j_i] (an integer of [0, N)), v = v -
   (v[j_i]/h_i) H_i mod N. v has H->c entries in [0, N). If coef is not NULL, coef[i] gets q_i (0 for a
   step that is skipped). A zero row of H (no pivot) is skipped; it does not occur for an echelon H. */
static void
greedy_reduce(fmpz * v, fmpz * coef, const fmpz_mat_t H, slong start, const fmpz_t N)
{
    slong i, n = H->c;
    fmpz_t q;

    fmpz_init(q);
    for (i = start; i < H->r; i++)
    {
        const fmpz * row = H->rows[i];
        slong j = first_nonzero(row, n);

        if (coef != NULL)
            fmpz_zero(coef + i);
        if (j == n || fmpz_sgn(row + j) <= 0 || !fmpz_divisible(v + j, row + j))
            continue;
        fmpz_divexact(q, v + j, row + j);
        if (fmpz_is_zero(q))
            continue;
        if (coef != NULL)
            fmpz_set(coef + i, q);
        vec_submul_mod(v, row, q, n, N);
    }
    fmpz_clear(q);
}

/* ---- Algorithm H (solvers.md:620 to 640) ---- */

/* the pending pairs (v, j) of Algorithm H, a stack: the last pair added is processed first, as in
   the reference (proto/solvers_checks.py:763 to 765). The order does not change the output, which is
   the unique Howell form of the span (Proposition 2.4, solvers.md:586; 2.5(2), solvers.md:645). */
typedef struct
{
    fmpz ** v;
    slong * j;
    slong len;
    slong alloc;
} pending_t;

static void
pending_push(pending_t * P, fmpz * v, slong j)
{
    if (P->len == P->alloc)
    {
        P->alloc = P->alloc == 0 ? 16 : 2 * P->alloc;
        P->v = flint_realloc(P->v, (size_t) P->alloc * sizeof(fmpz *));
        P->j = flint_realloc(P->j, (size_t) P->alloc * sizeof(slong));
    }
    P->v[P->len] = v;
    P->j[P->len] = j;
    P->len++;
}

/* H = the Howell form over Z/N (Definition 2.1, solvers.md:504 to 531) of the span of the rows of M, whose
   entries are any integers; H is initialised here, with k rows (the number of pivots) and M->c columns.

   Algorithm H: the input rows become the pending pairs (v mod N, column 0). A pair (v, j) is processed
   by the steps 1 and 2 of solvers.md:628 to 637:
   1. skip the zero entries of v from column j; at the end of v the pair is finished;
   2. a = v[j] in [1, N);
      - the table has no row at column j: g = gcd(a, N) = s a + t N; T[j] = s v mod N (pivot g); the pair
        ((N/g) v mod N, j + 1) is added; the pair is finished;
      - T[j] = w with pivot h, h | a: v = v - (a/h) w mod N, j + 1, step 1;
      - otherwise g = gcd(a, h) = s a + t h; w' = s v + t w, v' = (h/g) v - (a/g) w mod N; T[j] = w'
        (pivot g); the pair ((N/g) w' mod N, j + 1) is added; v = v', j + 1, step 1.
   Then the rows of T in the order of their columns, and for i = 1, ..., k and every l < i:
   H_l = H_l - floor(H[l, j_i]/h_i) H_i mod N (solvers.md:639 to 640). Proposition 2.5 (solvers.md:642 to
   692) proves that this ends and that the output is the Howell form of the span. */
static void
howell_form(fmpz_mat_t H, const fmpz_mat_t M, const fmpz_t N)
{
    slong n = M->c, m = M->r, i, j, l, k;
    fmpz ** T = NULL;
    pending_t P = { NULL, NULL, 0, 0 };
    fmpz_t g, s, t, q, hg, ag, Ng;

    fmpz_init(g);
    fmpz_init(s);
    fmpz_init(t);
    fmpz_init(q);
    fmpz_init(hg);
    fmpz_init(ag);
    fmpz_init(Ng);
    if (n > 0)
        T = flint_calloc((size_t) n, sizeof(fmpz *));
    for (i = m - 1; i >= 0 && n > 0; i--)            /* row 0 is processed first */
    {
        fmpz * v = _fmpz_vec_init(n);

        for (j = 0; j < n; j++)
            fmpz_mod(v + j, fmpz_mat_entry(M, i, j), N);
        pending_push(&P, v, 0);
    }
    while (P.len > 0)
    {
        fmpz * v;

        P.len--;
        v = P.v[P.len];
        j = P.j[P.len];
        for (;;)
        {
            const fmpz * a;

            while (j < n && fmpz_is_zero(v + j))     /* step 1 */
                j++;
            if (j >= n)
            {
                _fmpz_vec_clear(v, n);
                break;
            }
            a = v + j;                               /* step 2: a in [1, N) */
            if (T[j] == NULL)
            {
                fmpz_xgcd(g, s, t, a, N);            /* s a + t N = g = gcd(a, N), 1 <= g < N */
                T[j] = _fmpz_vec_init(n);
                _fmpz_vec_set(T[j], v, n);
                vec_scalar_mod(T[j], s, n, N);       /* T[j] = s v mod N, entry g at column j */
                fmpz_divexact(Ng, N, g);
                vec_scalar_mod(v, Ng, n, N);         /* the pending pair ((N/g) v mod N, j + 1) */
                pending_push(&P, v, j + 1);
                break;
            }
            else
            {
                fmpz * w = T[j];
                const fmpz * h = w + j;

                if (fmpz_divisible(a, h))
                {
                    fmpz_divexact(q, a, h);
                    vec_submul_mod(v, w, q, n, N);   /* v = v - (a/h) w mod N; entry 0 at column j */
                    j++;
                    continue;
                }
                else
                {
                    fmpz * w2 = _fmpz_vec_init(n);
                    fmpz * u = _fmpz_vec_init(n);

                    fmpz_xgcd(g, s, t, a, h);        /* s a + t h = g = gcd(a, h), a proper divisor of h */
                    fmpz_divexact(hg, h, g);
                    fmpz_divexact(ag, a, g);
                    for (l = 0; l < n; l++)
                    {
                        fmpz_mul(w2 + l, s, v + l);  /* w' = s v + t w mod N */
                        fmpz_addmul(w2 + l, t, w + l);
                        fmpz_mod(w2 + l, w2 + l, N);
                        fmpz_mul(u + l, hg, v + l);  /* v' = (h/g) v - (a/g) w mod N */
                        fmpz_submul(u + l, ag, w + l);
                        fmpz_mod(u + l, u + l, N);
                    }
                    _fmpz_vec_clear(T[j], n);
                    T[j] = w2;                       /* pivot g */
                    _fmpz_vec_clear(v, n);
                    v = u;
                    {
                        fmpz * p = _fmpz_vec_init(n);

                        _fmpz_vec_set(p, w2, n);
                        fmpz_divexact(Ng, N, g);
                        vec_scalar_mod(p, Ng, n, N); /* the pending pair ((N/g) w' mod N, j + 1) */
                        pending_push(&P, p, j + 1);
                    }
                    j++;
                    continue;
                }
            }
        }
    }
    /* the rows of T in the order of their columns */
    k = 0;
    for (j = 0; j < n; j++)
        k += T[j] != NULL;
    fmpz_mat_init(H, k, n);
    i = 0;
    for (j = 0; j < n; j++)
        if (T[j] != NULL)
        {
            _fmpz_vec_set(H->rows[i], T[j], n);
            _fmpz_vec_clear(T[j], n);
            i++;
        }
    /* the final reduction: 0 <= H[l, j_i] < h_i for l < i (E3) */
    for (i = 0; i < k; i++)
    {
        slong ji = first_nonzero(H->rows[i], n);
        const fmpz * hi = H->rows[i] + ji;

        for (l = 0; l < i; l++)
        {
            fmpz_fdiv_q(q, H->rows[l] + ji, hi);
            if (!fmpz_is_zero(q))
                vec_submul_mod(H->rows[l], H->rows[i], q, n, N);
        }
    }
    flint_free(T);
    flint_free(P.v);
    flint_free(P.j);
    fmpz_clear(g);
    fmpz_clear(s);
    fmpz_clear(t);
    fmpz_clear(q);
    fmpz_clear(hg);
    fmpz_clear(ag);
    fmpz_clear(Ng);
}

/* The kernel certificate of Proposition 2.6(3) (solvers.md:713 to 716): the Howell form H of
   [A^T | I_c] (c rows, r + c columns; row j is column j of A followed by e_j), split into the rows whose
   pivot column is among the first r, written (E_i | V_i), and the right blocks G_i of the others. A has
   entries in [0, N). E, V, G are initialised here: e by r, e by c, k by c. */
static void
kernel_cert(fmpz_mat_t E, fmpz_mat_t V, fmpz_mat_t G, const fmpz_mat_t A, const fmpz_t N)
{
    slong r = A->r, c = A->c, n = r + c, i, j, e = 0, k = 0;
    fmpz_mat_t M, H;

    fmpz_mat_init(M, c, n);
    for (j = 0; j < c; j++)
    {
        for (i = 0; i < r; i++)
            fmpz_set(fmpz_mat_entry(M, j, i), fmpz_mat_entry(A, i, j));
        fmpz_one(fmpz_mat_entry(M, j, r + j));       /* 1, reduced to 0 by howell_form for N = 1 */
    }
    howell_form(H, M, N);
    for (i = 0; i < H->r; i++)
        e += first_nonzero(H->rows[i], n) < r;
    fmpz_mat_init(E, e, r);
    fmpz_mat_init(V, e, c);
    fmpz_mat_init(G, H->r - e, c);
    e = 0;
    for (i = 0; i < H->r; i++)
    {
        if (first_nonzero(H->rows[i], n) < r)
        {
            _fmpz_vec_set(E->rows[e], H->rows[i], r);
            _fmpz_vec_set(V->rows[e], H->rows[i] + r, c);
            e++;
        }
        else
        {
            _fmpz_vec_set(G->rows[k], H->rows[i] + r, c);
            k++;
        }
    }
    fmpz_mat_clear(M);
    fmpz_mat_clear(H);
}

/* ---- the checker (Propositions 2.6, 2.7, Algorithm L step 5) ---- */

/* The shapes and entries of the predicate (docs/api-s.md section 3): N >= 1; kind in {0, 1}; r, c >= 0;
   G k by c, E e by r, V e by c; x0 c by 1 and y 0 by 0 for kind COSET, x0 0 by 0 and y r by 1 for kind
   EMPTY; every entry in [0, N). */
static int
shapes_ok(const adf_linsol_t sol)
{
    slong r = sol->r, c = sol->c;

    if (fmpz_sgn(sol->N) < 1 || r < 0 || c < 0)
        return 0;
    if (sol->G->c != c || sol->E->c != r || sol->V->c != c || sol->E->r != sol->V->r)
        return 0;
    if (sol->kind == ADF_LINSOL_COSET)
    {
        if (sol->x0->r != c || sol->x0->c != 1 || sol->y->r != 0 || sol->y->c != 0)
            return 0;
    }
    else if (sol->kind == ADF_LINSOL_EMPTY)
    {
        if (sol->y->r != r || sol->y->c != 1 || sol->x0->r != 0 || sol->x0->c != 0)
            return 0;
    }
    else
        return 0;
    return mat_reduced(sol->G, sol->N) && mat_reduced(sol->E, sol->N) && mat_reduced(sol->V, sol->N)
           && mat_reduced(sol->x0, sol->N) && mat_reduced(sol->y, sol->N);
}

/* (E1), (E2) of Definition 2.1 (solvers.md:512 to 514), for entries already in [0, N): every row
   non-zero, pivot columns strictly increasing, every pivot a divisor of N with 1 <= pivot < N. */
static int
is_echelon(const fmpz_mat_t H, const fmpz_t N)
{
    slong i, last = -1, n = H->c;

    for (i = 0; i < H->r; i++)
    {
        slong j = first_nonzero(H->rows[i], n);

        if (j == n || j <= last || !fmpz_divisible(N, H->rows[i] + j))
            return 0;
        last = j;
    }
    return 1;
}

/* (K4) (solvers.md:703): (N/p_1) ... (N/p_e) (N/g_1) ... (N/g_k) = N^c as integers. A row without a
   pivot, or a pivot that does not divide N, fails (this matters only if (K1) is not tested first). */
static int
k4_holds(const fmpz_mat_t E, const fmpz_mat_t G, const fmpz_t N, slong c)
{
    fmpz_t prod, t;
    slong i, m;
    int ok = 1;

    fmpz_init_set_ui(prod, 1);
    fmpz_init(t);
    for (m = 0; m < 2 && ok; m++)
    {
        const fmpz_mat_struct * H = m == 0 ? E : G;

        for (i = 0; i < H->r && ok; i++)
        {
            slong j = first_nonzero(H->rows[i], H->c);

            if (j == H->c || !fmpz_divisible(N, H->rows[i] + j))
                ok = 0;
            else
            {
                fmpz_divexact(t, N, H->rows[i] + j);
                fmpz_mul(prod, prod, t);
            }
        }
    }
    if (ok)
    {
        fmpz_pow_ui(t, N, (ulong) c);
        ok = fmpz_equal(prod, t);
    }
    fmpz_clear(prod);
    fmpz_clear(t);
    return ok;
}

/* (K5) (solvers.md:704 to 705): G satisfies (E3), 0 <= G[l, j_i] < g_i for l < i, and for every i the greedy
   reduction of (N/g_i) G_i by the rows after row i ends with zero (Lemma 2.3(1), (2)). */
static int
k5_holds(const fmpz_mat_t G, const fmpz_t N)
{
    slong i, l, n = G->c;
    fmpz * u;
    fmpz_t Ng;
    int ok = 1;

    if (G->r == 0)
        return 1;
    u = _fmpz_vec_init(n);
    fmpz_init(Ng);
    for (i = 0; i < G->r && ok; i++)
    {
        slong j = first_nonzero(G->rows[i], n);

        if (j == n || !fmpz_divisible(N, G->rows[i] + j))
        {
            ok = 0;
            break;
        }
        for (l = 0; l < i && ok; l++)                /* (E3) */
            ok = fmpz_cmp(G->rows[l] + j, G->rows[i] + j) < 0;
        fmpz_divexact(Ng, N, G->rows[i] + j);
        _fmpz_vec_set(u, G->rows[i], n);
        vec_scalar_mod(u, Ng, n, N);
        greedy_reduce(u, NULL, G, i + 1, N);
        ok = ok && _fmpz_vec_is_zero(u, n);
    }
    _fmpz_vec_clear(u, n);
    fmpz_clear(Ng);
    return ok;
}

/* 1 if A x = e modulo N, with x a vector of c entries and e a vector of r entries (A reduced) */
static int
matvec_equals(const fmpz_mat_t A, const fmpz * x, const fmpz * e, const fmpz_t N)
{
    slong i, j;
    fmpz_t acc;
    int ok = 1;

    fmpz_init(acc);
    for (i = 0; i < A->r && ok; i++)
    {
        fmpz_zero(acc);
        for (j = 0; j < A->c; j++)
            fmpz_addmul(acc, fmpz_mat_entry(A, i, j), x + j);
        fmpz_mod(acc, acc, N);
        ok = e == NULL ? fmpz_is_zero(acc) : fmpz_equal(acc, e + i);
    }
    fmpz_clear(acc);
    return ok;
}

/* (K2): A V_i = E_i modulo N for every row i (solvers.md:701) */
static int
k2_holds(const fmpz_mat_t A, const fmpz_mat_t E, const fmpz_mat_t V, const fmpz_t N)
{
    slong i;

    for (i = 0; i < V->r; i++)
        if (!matvec_equals(A, V->rows[i], E->rows[i], N))
            return 0;
    return 1;
}

/* (K3): A G_i = 0 modulo N for every row i (solvers.md:702) */
static int
k3_holds(const fmpz_mat_t A, const fmpz_mat_t G, const fmpz_t N)
{
    slong i;

    for (i = 0; i < G->r; i++)
        if (!matvec_equals(A, G->rows[i], NULL, N))
            return 0;
    return 1;
}

/* (K6): A x0 = b modulo N (solvers.md:800); x0 and b are columns */
static int
k6_holds(const fmpz_mat_t A, const fmpz_mat_t x0, const fmpz_mat_t b, const fmpz_t N)
{
    fmpz * x = _fmpz_vec_init(A->c);
    fmpz * e = _fmpz_vec_init(A->r);
    slong i;
    int ok;

    for (i = 0; i < A->c; i++)
        fmpz_set(x + i, fmpz_mat_entry(x0, i, 0));
    for (i = 0; i < A->r; i++)
        fmpz_set(e + i, fmpz_mat_entry(b, i, 0));
    ok = matvec_equals(A, x, e, N);
    _fmpz_vec_clear(x, A->c);
    _fmpz_vec_clear(e, A->r);
    return ok;
}

/* (K7): y^T A = 0 and y^T b != 0 modulo N (solvers.md:800; Proposition 2.7(1), solvers.md:759) */
static int
k7_holds(const fmpz_mat_t A, const fmpz_mat_t y, const fmpz_mat_t b, const fmpz_t N)
{
    slong i, j;
    fmpz_t acc;
    int ok = 1;

    fmpz_init(acc);
    for (j = 0; j < A->c && ok; j++)
    {
        fmpz_zero(acc);
        for (i = 0; i < A->r; i++)
            fmpz_addmul(acc, fmpz_mat_entry(y, i, 0), fmpz_mat_entry(A, i, j));
        ok = fmpz_divisible(acc, N);
    }
    if (ok)
    {
        fmpz_zero(acc);
        for (i = 0; i < A->r; i++)
            fmpz_addmul(acc, fmpz_mat_entry(y, i, 0), fmpz_mat_entry(b, i, 0));
        ok = !fmpz_divisible(acc, N);
    }
    fmpz_clear(acc);
    return ok;
}

int
adf_linsol_is_canonical(const adf_linsol_t sol)
{
    if (!shapes_ok(sol))
        return 0;
    return is_echelon(sol->E, sol->N) && is_echelon(sol->G, sol->N) && k4_holds(sol->E, sol->G, sol->N, sol->c)
           && k5_holds(sol->G, sol->N);
}

/* The checker of Propositions 2.6(1), (2), 2.7(1) and Algorithm L step 5 (solvers.md:699 to 705, 800). The
   conditions imply the answer whatever produced sol (Proposition 2.8(3), solvers.md:806 to 808): with (K1) to
   (K4) the rows of G generate the whole kernel (2.6(1)); with (K5) G is its Howell form (2.6(2)); with (K6)
   the solutions are x0 + S(G); with (K7) there is none. One condition per statement below. */
int
adf_linsol_verify(const adf_linsol_t sol, const fmpz_mat_t A, const fmpz_mat_t b, const fmpz_t N)
{
    fmpz_mat_t Ar, br;
    int ok = 0;

    if (fmpz_sgn(N) < 1 || !shapes_ok(sol) || !fmpz_equal(sol->N, N) || sol->r != A->r || sol->c != A->c
        || b->r != A->r || b->c != 1)
        return 0;
    mat_init_mod(Ar, A, N);
    mat_init_mod(br, b, N);
    if (!is_echelon(sol->E, N) || !is_echelon(sol->G, N))                                 /* (K1) */
        goto done;
    if (!k2_holds(Ar, sol->E, sol->V, N))                                                 /* (K2) */
        goto done;
    if (!k3_holds(Ar, sol->G, N))                                                         /* (K3) */
        goto done;
    if (!k4_holds(sol->E, sol->G, N, sol->c))                                             /* (K4) */
        goto done;
    if (!k5_holds(sol->G, N))                                                             /* (K5) */
        goto done;
    if (sol->kind == ADF_LINSOL_COSET)
        ok = k6_holds(Ar, sol->x0, br, N);                                                /* (K6) */
    else
        ok = k7_holds(Ar, sol->y, br, N);                                                 /* (K7) */
done:
    fmpz_mat_clear(Ar);
    fmpz_mat_clear(br);
    return ok;
}

/* ---- the solver: Algorithm L (solvers.md:789 to 800) ---- */

/* Algorithm L:
   1. N < 1 or b not r by 1: DOMAIN; r + c > ADF_LINSOLVE_DIM_MAX: LIMIT (S-D8), both before any allocation.
      A and b are reduced modulo N.
   2. The kernel certificate (E, V, G) of Proposition 2.6(3) by Algorithm H.
   3. The greedy reduction of b (a row of r entries) by E, with coefficients q_i. If it ends with zero,
      x0 = q_1 V_1 + ... + q_e V_e mod N, and the answer is the coset x0 + S(G) (Proposition 2.8(1),
      solvers.md:804).
   4. Otherwise the generators Y of the kernel of A^T (Proposition 2.6(3) for A^T), and y the first row of Y
      with y b != 0 modulo N; such a row exists (Proposition 2.8(2), solvers.md:805, by 2.7(3)).
   5. The checker adf_linsol_verify, before the answer is returned. */
int
adf_linsolve_mod(adf_linsol_t sol, const fmpz_mat_t A, const fmpz_mat_t b, const fmpz_t N)
{
    slong r = A->r, c = A->c, i, j;
    fmpz_mat_t Ar, br, At, E2, V2, Y;
    adf_linsol_t res;
    adf_linsol_struct t;
    fmpz * v;
    fmpz * q;
    int status, found = 0;

    /* step 1 */
    if (fmpz_sgn(N) < 1 || b->r != r || b->c != 1)
        return ADF_DOMAIN;
    if (r > ADF_LINSOLVE_DIM_MAX || c > ADF_LINSOLVE_DIM_MAX - r)
        return ADF_LIMIT;
    mat_init_mod(Ar, A, N);
    mat_init_mod(br, b, N);

    /* step 2 */
    adf_linsol_init(res);
    fmpz_set(res->N, N);
    res->r = r;
    res->c = c;
    fmpz_mat_clear(res->E);
    fmpz_mat_clear(res->V);
    fmpz_mat_clear(res->G);
    fmpz_mat_clear(res->x0);
    kernel_cert(res->E, res->V, res->G, Ar, N);

    /* step 3 */
    v = _fmpz_vec_init(r);
    q = _fmpz_vec_init(res->E->r);
    for (i = 0; i < r; i++)
        fmpz_set(v + i, fmpz_mat_entry(br, i, 0));
    greedy_reduce(v, q, res->E, 0, N);
    if (_fmpz_vec_is_zero(v, r))
    {
        fmpz_mat_init(res->x0, c, 1);
        for (j = 0; j < c; j++)
        {
            fmpz * x = fmpz_mat_entry(res->x0, j, 0);

            for (i = 0; i < res->E->r; i++)
                fmpz_addmul(x, q + i, fmpz_mat_entry(res->V, i, j));
            fmpz_mod(x, x, N);
        }
        res->kind = ADF_LINSOL_COSET;
        status = ADF_OK;
        found = 1;
    }
    else
    {
        /* step 4 */
        fmpz_mat_init(At, c, r);
        for (i = 0; i < r; i++)
            for (j = 0; j < c; j++)
                fmpz_set(fmpz_mat_entry(At, j, i), fmpz_mat_entry(Ar, i, j));
        kernel_cert(E2, V2, Y, At, N);
        fmpz_mat_init(res->x0, 0, 0);
        fmpz_mat_clear(res->y);
        fmpz_mat_init(res->y, r, 1);
        for (i = 0; i < Y->r && !found; i++)
        {
            fmpz_t acc;

            fmpz_init(acc);
            for (j = 0; j < r; j++)
                fmpz_addmul(acc, fmpz_mat_entry(Y, i, j), fmpz_mat_entry(br, j, 0));
            if (!fmpz_divisible(acc, N))
            {
                for (j = 0; j < r; j++)
                    fmpz_set(fmpz_mat_entry(res->y, j, 0), fmpz_mat_entry(Y, i, j));
                found = 1;
            }
            fmpz_clear(acc);
        }
        res->kind = ADF_LINSOL_EMPTY;
        status = ADF_NO_SOLUTION;
        fmpz_mat_clear(At);
        fmpz_mat_clear(E2);
        fmpz_mat_clear(V2);
        fmpz_mat_clear(Y);
    }
    _fmpz_vec_clear(v, r);
    _fmpz_vec_clear(q, res->E->r);

    /* step 5: the checker; for Algorithm H a refusal is a defect of the library (see the header) */
    if (!found || !adf_linsol_verify(res, A, b, N))
    {
        flint_printf("adf_linsolve_mod: the result of Algorithm H fails the checker (a defect of adelefeld)\n");
        flint_abort();
    }
    t = *sol;
    *sol = *res;
    *res = t;
    adf_linsol_clear(res);
    fmpz_mat_clear(Ar);
    fmpz_mat_clear(br);
    return status;
}

/* ---- accessors ---- */

int
adf_linsol_kind(const adf_linsol_t sol)
{
    return sol->kind;
}

slong
adf_linsol_kernel_rows(const adf_linsol_t sol)
{
    return sol->G->r;
}

/* d = a copy of s with the shape of s, through a temporary, so that d may be s itself */
static void
mat_get(fmpz_mat_t d, const fmpz_mat_t s)
{
    fmpz_mat_t t;
    slong i;

    fmpz_mat_init(t, s->r, s->c);
    for (i = 0; i < s->r; i++)
        _fmpz_vec_set(t->rows[i], s->rows[i], s->c);
    fmpz_mat_swap(d, t);
    fmpz_mat_clear(t);
}

void
adf_linsol_get_kernel(fmpz_mat_t G, const adf_linsol_t sol)
{
    mat_get(G, sol->G);
}

int
adf_linsol_get_particular(fmpz_mat_t x0, const adf_linsol_t sol)
{
    if (sol->kind != ADF_LINSOL_COSET)
        return 0;
    mat_get(x0, sol->x0);
    return 1;
}

int
adf_linsol_get_dual(fmpz_mat_t y, const adf_linsol_t sol)
{
    if (sol->kind != ADF_LINSOL_EMPTY)
        return 0;
    mat_get(y, sol->y);
    return 1;
}
