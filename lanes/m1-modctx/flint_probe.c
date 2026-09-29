#include <stdio.h>
#include <flint/flint.h>
#include <flint/fmpz.h>

static void show(const char *tag, const fmpz_t x) { printf("%s", tag); fmpz_print(x); printf("\n"); }

int main(void)
{
    fmpz_multi_mod_t P;
    fmpz_multi_CRT_t C;
    fmpz_t moduli[3], vals[3], out[3], a, t;
    int i, ok;

    for (i = 0; i < 3; i++) { fmpz_init(moduli[i]); fmpz_init(vals[i]); fmpz_init(out[i]); }
    fmpz_set_ui(moduli[0], 6);
    fmpz_set_ui(moduli[1], 35);
    fmpz_set_ui(moduli[2], 11);
    fmpz_init(a); fmpz_init(t);
    fmpz_set_str(a, "12345678901234567890123456789", 10);

    fmpz_multi_mod_init(P);
    ok = fmpz_multi_mod_precompute(P, moduli, 3);
    printf("multi_mod precompute ok=%d good=%d\n", ok, P->good);
    fmpz_multi_mod_precomp(out, P, a, 0);
    for (i = 0; i < 3; i++) show("mod sign0 out=", out[i]);
    fmpz_multi_mod_precomp(out, P, a, 1);
    for (i = 0; i < 3; i++) show("mod sign1 out=", out[i]);
    fmpz_neg(t, a);
    fmpz_multi_mod_precomp(out, P, t, 0);
    for (i = 0; i < 3; i++) show("mod neg sign0 out=", out[i]);

    fmpz_multi_CRT_init(C);
    ok = fmpz_multi_CRT_precompute(C, moduli, 3);
    printf("multi_CRT precompute ok=%d good=%d\n", ok, C->good);
    show("final=", C->final_modulus);
    for (i = 0; i < 3; i++) fmpz_mod(vals[i], a, moduli[i]);
    fmpz_multi_CRT_precomp(t, C, vals, 0);
    show("crt sign0 out=", t);
    fmpz_multi_CRT_precomp(t, C, vals, 1);
    show("crt sign1 out=", t);
    {
        fmpz_multi_mod_t P1; fmpz_multi_CRT_t C1; fmpz_t o1;
        fmpz_init(o1);
        fmpz_multi_mod_init(P1);
        ok = fmpz_multi_mod_precompute(P1, moduli, 1);
        printf("k=1 mod precompute ok=%d good=%d\n", ok, P1->good);
        fmpz_multi_mod_precomp(out, P1, a, 0);
        show("k=1 mod out=", out[0]);
        fmpz_multi_CRT_init(C1);
        ok = fmpz_multi_CRT_precompute(C1, moduli, 1);
        printf("k=1 crt precompute ok=%d good=%d\n", ok, C1->good);
        show("k=1 final=", C1->final_modulus);
        fmpz_multi_CRT_precomp(o1, C1, vals, 0);
        show("k=1 crt out=", o1);
        fmpz_multi_mod_clear(P1); fmpz_multi_CRT_clear(C1); fmpz_clear(o1);
    }
    {
        fmpz_multi_mod_t P0; fmpz_multi_CRT_t C0;
        fmpz_multi_mod_init(P0);
        ok = fmpz_multi_mod_precompute(P0, moduli, 0);
        printf("k=0 mod precompute ok=%d good=%d\n", ok, P0->good);
        fmpz_multi_CRT_init(C0);
        ok = fmpz_multi_CRT_precompute(C0, moduli, 0);
        printf("k=0 crt precompute ok=%d good=%d\n", ok, C0->good);
        show("k=0 final=", C0->final_modulus);
        fmpz_multi_mod_clear(P0); fmpz_multi_CRT_clear(C0);
    }
    {
        fmpz_multi_CRT_t C2; fmpz_t m2[2];
        fmpz_init(m2[0]); fmpz_init(m2[1]);
        fmpz_set_ui(m2[0], 4); fmpz_set_ui(m2[1], 6);
        fmpz_multi_CRT_init(C2);
        ok = fmpz_multi_CRT_precompute(C2, m2, 2);
        printf("noncoprime crt precompute ok=%d\n", ok);
        fmpz_multi_CRT_clear(C2); fmpz_clear(m2[0]); fmpz_clear(m2[1]);
    }
    fmpz_multi_mod_clear(P); fmpz_multi_CRT_clear(C);
    for (i = 0; i < 3; i++) { fmpz_clear(moduli[i]); fmpz_clear(vals[i]); fmpz_clear(out[i]); }
    fmpz_clear(a); fmpz_clear(t);
    return 0;
}
