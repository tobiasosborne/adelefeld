/* lanes/q-review2/member_eval.c: exercise set_adele, set_rat, add_rat, get_piece, set, swap on
   LIFT classes and print the stored piece, so that the represented set can be decided in Python.

   Commands on stdin:
     NEW                                        a fresh class, LIFT of (0;0)
     AD <midman> <midexp> <radman> <radexp> <A> <H> <d> <free>
            set_adele(y, a) from a fresh adele a; free = 1 clears a right after the call
     RAT <num> <den>                            set_rat(y, q)
     ADD <num> <den>                            add_rat(y, x, q)
     SELF <set|swap|add>                        the same object as source and destination
     PIECE                                      print the stored piece of y
     IDENT <n>                                  identical(y, x) for n = 0,1
     GET <i>                                    get_piece into a scratch adele, print it
     FORM                                       form and length of y
   Every PIECE line: "P <mid> <mid_den> <rad> <rad_den> <A> <H> <d> <backend> <canon>"
   Every GET line: "G <status> <mid> <mid_den> <rad> <rad_den> <A> <H> <d>" */
#include <adelefeld.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void set_pow2(arf_t r, const char * man, const char * exp)
{
    fmpz_t m, e;
    fmpz_init(m); fmpz_init(e);
    fmpz_set_str(m, man, 10);
    fmpz_set_str(e, exp, 10);
    arf_set_fmpz_2exp(r, m, e);
    fmpz_clear(m); fmpz_clear(e);
}

static void print_dyadic(arf_srcptr v, const char * tag)
{
    fmpz_t man, ex, num, den;
    char * s;
    fmpz_init(man); fmpz_init(ex); fmpz_init(num); fmpz_init(den);
    arf_get_fmpz_2exp(man, ex, v);
    fmpz_set(num, man);
    fmpz_one(den);
    if (fmpz_sgn(ex) >= 0) fmpz_mul_2exp(num, num, fmpz_get_ui(ex));
    else fmpz_mul_2exp(den, den, (-fmpz_get_si(ex)));
    s = fmpz_get_str(NULL, 10, num); printf(" %s", s); flint_free(s);
    s = fmpz_get_str(NULL, 10, den); printf(" %s", s); flint_free(s);
    fmpz_clear(man); fmpz_clear(ex); fmpz_clear(num); fmpz_clear(den);
    (void) tag;
}

static void print_piece(const adf_adele_struct *a)
{
    char * s;
    fmpz_t A, H, d;
    fmpz_init(A); fmpz_init(H); fmpz_init(d);
    adf_fball_get_fmpz3(A, H, d, &a->fin);
    print_dyadic(arb_midref(a->inf), "mid");
    {
        arf_t rr;
        arf_init(rr);
        arf_set_mag(rr, arb_radref(a->inf));
        print_dyadic(rr, "rad");
        arf_clear(rr);
    }
    s = fmpz_get_str(NULL, 10, A); printf(" %s", s); flint_free(s);
    s = fmpz_get_str(NULL, 10, H); printf(" %s", s); flint_free(s);
    s = fmpz_get_str(NULL, 10, d); printf(" %s", s); flint_free(s);
    printf(" %d", (int) a->fin.backend);
    fmpz_clear(A); fmpz_clear(H); fmpz_clear(d);
}

int main(void)
{
    static char line[65536];
    adf_qclass_t y, x;
    adf_qclass_init(y);
    adf_qclass_init(x);
    while (fgets(line, sizeof(line), stdin)) {
        if (strncmp(line, "NEW", 3) == 0) {
            adf_qclass_clear(y);
            adf_qclass_init(y);
            printf("N\n"); fflush(stdout); continue;
        }
        if (strncmp(line, "AD ", 3) == 0) {
            char mm[64], me[64], rm[64], re[64], A[128], H[128], d[128];
            int fr;
            adf_adele_t a;
            if (sscanf(line, "AD %63s %63s %63s %63s %127s %127s %127s %d",
                       mm, me, rm, re, A, H, d, &fr) != 8) { printf("BAD AD\n"); return 2; }
            adf_adele_init(a);
            set_pow2(arb_midref(a->inf), mm, me);
            mag_set_ui_2exp_si(arb_radref(a->inf), strtoull(rm, NULL, 10), strtol(re, NULL, 10));
            {
                fmpz_t t;
                fmpz_init(t);
                fmpz_set_str(t, A, 10); fmpz_set(a->fin.A, t);
                fmpz_set_str(t, H, 10); fmpz_set(a->fin.H, t);
                fmpz_set_str(t, d, 10); fmpz_set(a->fin.d, t);
                fmpz_clear(t);
            }
            adf_qclass_set_adele(y, a);
            if (fr) adf_adele_clear(a);            /* the class must own its copy */
            printf("A\n"); fflush(stdout); continue;
        }
        if (strncmp(line, "RAT ", 4) == 0) {
            char n[4096], dd[4096];
            adf_rat_t q;
            if (sscanf(line, "RAT %4095s %4095s", n, dd) != 2) { printf("BAD RAT\n"); return 2; }
            adf_rat_init(q);
            if (fmpq_set_str(q->q, dd, 10)) { printf("BAD Q\n"); return 2; }
            { fmpq_t tmp; fmpq_init(tmp); fmpq_set_str(tmp, n, 10); fmpq_mul(q->q, tmp, q->q); fmpq_clear(tmp); }
            adf_qclass_set_rat(y, q);
            adf_rat_clear(q);
            printf("R\n"); fflush(stdout); continue;
        }
        if (strncmp(line, "ADD ", 4) == 0) {
            char n[4096], dd[4096];
            adf_rat_t q;
            if (sscanf(line, "ADD %4095s %4095s", n, dd) != 2) { printf("BAD ADD\n"); return 2; }
            adf_rat_init(q);
            if (fmpq_set_str(q->q, dd, 10)) { printf("BAD Q\n"); return 2; }
            { fmpq_t tmp; fmpq_init(tmp); fmpq_set_str(tmp, n, 10); fmpq_mul(q->q, tmp, q->q); fmpq_clear(tmp); }
            adf_qclass_add_rat(y, x, q);
            adf_rat_clear(q);
            printf("D\n"); fflush(stdout); continue;
        }
        if (strncmp(line, "SELF ", 5) == 0) {
            if (strncmp(line + 5, "set", 3) == 0) adf_qclass_set(y, y);
            else if (strncmp(line + 5, "swap", 4) == 0) adf_qclass_swap(y, y);
            else {
                adf_rat_t q;
                adf_rat_init(q);
                adf_qclass_add_rat(y, y, q);
                adf_rat_clear(q);
            }
            printf("S\n"); fflush(stdout); continue;
        }
        if (strncmp(line, "COPY", 4) == 0) {
            adf_qclass_set(x, y);
            printf("C\n"); fflush(stdout); continue;
        }
        if (strncmp(line, "PIECE", 5) == 0) {
            printf("P"); print_piece(&y->piece[0]);
            printf(" %d\n", adf_qclass_is_canonical(y));
            fflush(stdout); continue;
        }
        if (strncmp(line, "IDENT", 5) == 0) {
            printf("I %d %d\n", adf_qclass_identical(y, x), adf_qclass_is_canonical(y));
            fflush(stdout); continue;
        }
        if (strncmp(line, "FORM", 4) == 0) {
            printf("F %d %ld\n", adf_qclass_form(y), (long) adf_qclass_length(y));
            fflush(stdout); continue;
        }
        if (strncmp(line, "GET ", 4) == 0) {
            adf_adele_struct aa[1];
            adf_adele_struct * a = aa;
            long i = strtol(line + 4, NULL, 10);
            int st;
            adf_adele_init(a);
            set_pow2(arb_midref(a->inf), "12345", "0");      /* a recognisable content */
            fmpz_set_si(a->fin.A, 777);
            st = adf_qclass_get_piece(a, y, i);
            printf("G %d", st);
            if (st == ADF_OK) print_piece(a);
            else { printf(" after:"); print_piece(a); }   /* the destination must be untouched */
            printf("\n"); fflush(stdout);
            adf_adele_clear(a);
            continue;
        }
    }
    adf_qclass_clear(y);
    adf_qclass_clear(x);
    return 0;
}
