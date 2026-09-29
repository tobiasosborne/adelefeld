/* Independent probes. Contracts under refs/src/flint-3.0.1/:
   fmpq.rst:562-567; fmpz_mat.rst:1168-1176,1224-1231; nmod_mat.rst:716-723;
   fmpz_poly.rst:1923-1926,3264-3268; arb_fmpz_poly.rst:66-100.
   The finite-span oracle below uses only repeated addition of generators. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <flint/flint.h>
#include <flint/fmpz.h>
#include <flint/fmpq.h>
#include <flint/fmpz_mat.h>
#include <flint/nmod_mat.h>
#include <flint/fmpz_poly.h>
#include <flint/arb_fmpz_poly.h>
#include <flint/acb.h>

static unsigned long state = 294710;
static unsigned long next_word(void)
{
    state = 1664525UL * state + 1013904223UL;
    return state;
}

static int code_row(const ulong *v, int cols, int N)
{
    int x = 0;
    for (int j = cols - 1; j >= 0; --j) x = N*x + (int)v[j];
    return x;
}

static int closure(unsigned char *seen, ulong rows[6][3], int nr, int nc, int N)
{
    int size = 1;
    for (int j = 0; j < nc; ++j) size *= N;
    int *queue = calloc((size_t)size, sizeof(int));
    memset(seen, 0, (size_t)size);
    int count = 1;
    seen[0] = 1;
    for (int head = 0; head < count; ++head)
    {
        int a = queue[head];
        ulong v[3] = {0, 0, 0};
        for (int j = 0; j < nc; ++j) { v[j] = a % N; a /= N; }
        for (int i = 0; i < nr; ++i)
        {
            ulong w[3] = {0, 0, 0};
            for (int j = 0; j < nc; ++j) w[j] = (v[j] + rows[i][j]) % N;
            int b = code_row(w, nc, N);
            if (!seen[b]) { seen[b] = 1; queue[count++] = b; }
        }
    }
    free(queue);
    return count;
}

static int canonical(ulong rows[6][3], int nr, int nc, int N)
{
    int last = -1;
    for (int i = 0; i < nr; ++i)
    {
        int j = 0;
        while (j < nc && !rows[i][j]) ++j;
        if (j == nc || j <= last || N % rows[i][j]) return 0;
        for (int l = 0; l < i; ++l) if (rows[l][j] >= rows[i][j]) return 0;
        last = j;
    }
    return 1;
}

static int matrices(void)
{
    int moduli[] = {1, 4, 6, 8, 9, 12, 16, 36};
    long cases = 0, bad = 0;
    for (int ni = 0; ni < 8; ++ni)
    {
        int N = moduli[ni];
        for (int nc = 0; nc <= 3; ++nc)
        {
            int size = 1;
            for (int j = 0; j < nc; ++j) size *= N;
            for (int rep = 0; rep < 80; ++rep)
            {
                int nr = rep % 4;
                int padded = nr > nc ? nr : nc;
                ulong input[6][3] = {{0}}, output[6][3] = {{0}}, word[6][3] = {{0}};
                fmpz_mat_t A;
                nmod_mat_t B;
                fmpz_t mod;
                fmpz_init_set_ui(mod, N);
                fmpz_mat_init(A, padded, nc);
                nmod_mat_init(B, padded, nc, N);
                for (int i = 0; i < nr; ++i)
                    for (int j = 0; j < nc; ++j)
                    {
                        input[i][j] = (next_word() >> 8) % N;
                        fmpz_set_ui(fmpz_mat_entry(A, i, j), input[i][j]);
                        nmod_mat_entry(B, i, j) = input[i][j];
                    }
                slong ka = fmpz_mat_howell_form_mod(A, mod);
                slong kb = nmod_mat_howell_form(B);
                for (int i = 0; i < padded; ++i)
                    for (int j = 0; j < nc; ++j)
                    {
                        output[i][j] = fmpz_get_ui(fmpz_mat_entry(A, i, j));
                        word[i][j] = nmod_mat_entry(B, i, j);
                    }
                unsigned char *s = calloc((size_t)size, 1), *t = calloc((size_t)size, 1);
                closure(s, input, nr, nc, N);
                closure(t, output, ka, nc, N);
                int ok = !memcmp(s, t, (size_t)size) && canonical(output, ka, nc, N);
                closure(t, word, kb, nc, N);
                ok = ok && !memcmp(s, t, (size_t)size) && canonical(word, kb, nc, N);
                ok = ok && ka == kb && !memcmp(output, word, sizeof(output));
                fmpz_mat_t lattice, hermite;
                fmpz_mat_init(lattice, nr + nc, nc);
                fmpz_mat_init(hermite, nr + nc, nc);
                for (int i = 0; i < nr; ++i)
                    for (int j = 0; j < nc; ++j)
                        fmpz_set_ui(fmpz_mat_entry(lattice, i, j), input[i][j]);
                for (int j = 0; j < nc; ++j)
                    fmpz_set_ui(fmpz_mat_entry(lattice, nr+j, j), N);
                fmpz_mat_hnf(hermite, lattice);
                int kept = 0;
                for (int i = 0; i < nc; ++i)
                {
                    ulong d = fmpz_get_ui(fmpz_mat_entry(hermite, i, i));
                    ok = ok && d > 0 && N % d == 0;
                    for (int j = 0; j < nc; ++j)
                    {
                        ulong h = fmpz_get_ui(fmpz_mat_entry(hermite, i, j));
                        if (d == (ulong)N) ok = ok && h == (ulong)(i == j ? N : 0);
                        else ok = ok && kept < ka && h == output[kept][j];
                    }
                    if (d < (ulong)N) ++kept;
                }
                ok = ok && kept == ka;
                fmpz_mat_clear(lattice); fmpz_mat_clear(hermite);
                bad += !ok;
                ++cases;
                free(s); free(t);
                fmpz_mat_clear(A); nmod_mat_clear(B); fmpz_clear(mod);
            }
        }
    }
    printf("flint_howell_hnf: matrices=%ld, engines=3, differences=%ld\n", cases, bad);
    return bad != 0;
}

static int reconstruction(void)
{
    int bits[] = {63, 64, 65, 127, 128, 129, 255, 1024};
    fmpz_t m, a, n, d, inv, A, B, g;
    fmpq_t q;
    fmpq_init(q);
    fmpz_init(m); fmpz_init(a); fmpz_init(n); fmpz_init(d); fmpz_init(inv);
    fmpz_init(A); fmpz_init(B); fmpz_init(g);
    fmpz_set_ui(A, 7); fmpz_set_ui(B, 9);
    long cases = 0, bad = 0;
    for (int k = 0; k < 8; ++k)
    {
        fmpz_one(m); fmpz_mul_2exp(m, m, bits[k]); fmpz_add_ui(m, m, 15);
        for (int nn = -7; nn <= 7; ++nn)
            for (int dd = 1; dd <= 9; ++dd)
            {
                fmpz_set_si(n, nn); fmpz_set_ui(d, dd);
                fmpz_gcd(g, n, d);
                if (!fmpz_is_one(g) || !fmpz_invmod(inv, d, m)) continue;
                fmpz_mul(a, n, inv); fmpz_mod(a, a, m);
                int ret = fmpq_reconstruct_fmpz_2(q, a, m, A, B);
                ++cases;
                bad += !ret || !fmpz_equal(fmpq_numref(q), n) || !fmpz_equal(fmpq_denref(q), d);
            }
    }
    printf("flint_reconstruct_large: planted=%ld, bit_sizes=8, differences=%ld\n", cases, bad);
    fmpz_clear(m); fmpz_clear(a); fmpz_clear(n); fmpz_clear(d); fmpz_clear(inv);
    fmpz_clear(A); fmpz_clear(B); fmpz_clear(g); fmpq_clear(q);
    return bad != 0;
}

static int roots(void)
{
    fmpz_poly_t f, h, g, linear;
    fmpz_poly_init(f); fmpz_poly_init(h); fmpz_poly_init(g); fmpz_poly_init(linear);
    for (int d = 2; d <= 4; ++d)
    {
        fmpz_poly_zero(f); fmpz_poly_set_coeff_si(f, d, 1);
        printf("FINDING flint_nonsquarefree: f=X^%d, count=%ld, throws=0\n",
               d, fmpz_poly_num_real_roots(f));
    }
    fmpz_poly_set_coeff_si(f, 0, 1); /* not used as a nonsquarefree input to isolation */
    long cases = 0, bad = 0, exact = 0;
    for (int rep = 0; rep < 60; ++rep)
    {
        fmpz_poly_one(f);
        int expected = rep % 6;
        for (int j = 0; j < expected; ++j)
        {
            fmpz_poly_zero(linear);
            fmpz_poly_set_coeff_si(linear, 1, 1024);
            fmpz_poly_set_coeff_si(linear, 0, -(1024 + j));
            fmpz_poly_mul(f, f, linear);
        }
        /* Repeated input roots, removed independently by an exact polynomial gcd. */
        if (rep % 2) fmpz_poly_mul(f, f, f);
        fmpz_poly_derivative(h, f); fmpz_poly_gcd(h, h, f);
        if (!fmpz_poly_divides(g, f, h)) abort();
        if (rep % 3 == 0)
        {
            fmpz_poly_zero(linear);
            fmpz_poly_set_coeff_si(linear, 0, 1); fmpz_poly_set_coeff_si(linear, 2, 1);
            fmpz_poly_mul(g, g, linear);
        }
        slong count = fmpz_poly_num_real_roots(g), deg = fmpz_poly_degree(g);
        acb_ptr z = _acb_vec_init(deg);
        arb_fmpz_poly_complex_roots(z, g, 0, 8 + rep);
        int nreal = 0;
        for (int j = 0; j < deg; ++j)
            if (arb_is_zero(acb_imagref(z+j)))
            {
                ++nreal;
                exact += arb_is_exact(acb_realref(z+j));
                fmpq_t q;
                fmpq_init(q);
                fmpq_set_si(q, 1024+j, 1024);
                bad += !arb_contains_fmpq(acb_realref(z+j), q);
                fmpq_clear(q);
            }
        bad += count != expected || nreal != expected;
        ++cases;
        _acb_vec_clear(z, deg);
    }
    printf("flint_real_planted: polynomials=%ld, exact_balls=%ld, differences=%ld\n", cases, exact, bad);
    fmpz_poly_clear(f); fmpz_poly_clear(h); fmpz_poly_clear(g); fmpz_poly_clear(linear);
    return bad != 0;
}

int main(void)
{
    flint_set_num_threads(1);
    printf("FLINT=%s\n", flint_version);
    int failures = matrices() + reconstruction() + roots();
    printf("total: failures=%d\n", failures);
    flint_cleanup();
    return failures != 0;
}
