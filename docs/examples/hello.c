/* docs/examples/hello.c: one program through the C interface of adelefeld (docs/GUIDE.md, chapter 4).

   It builds the finite ball 146 + 1000 Zhat from its raw triple, makes it the finite part of an
   adele with a real ball, adds a second adele read from text, reconstructs the rational of the sum,
   projects the sum to the prime 5 and takes a square root there on a named branch. Every value is
   printed with the text functions of adelefeld/text.h, every status with adf_status_str, and every
   value and string is freed.

   Compile and run from the repository root (docs/examples/README.md):

       make -j2
       cc -std=c11 -Wall -Wextra -Werror -Iinclude docs/examples/hello.c build/libadelefeld.a \
          -lflint -lgmp -lm -o build/hello
       ./build/hello

   The contracts used, by header: fball.h (adf_fball_set_fmpz3), adele.h (adf_adele_set_arb_fball,
   adf_adele_add), text.h (the parsers adf_x_set_str and printers adf_x_get_str), recon.h
   (adf_adele_reconstruct), lball.h (adf_lball_set_fball), lroot.h (adf_lball_root_count,
   adf_lball_root_seed), place.h (adf_place_prime), status.h (adf_status_str), common.h
   (adf_str_free). */

#include <stdio.h>
#include <string.h>

#include <adelefeld.h>

/* Print a label, then a string from a printer of text.h, then free the string. */
static void
put(const char * label, char * s)
{
    printf("%-26s %s\n", label, s != NULL ? s : "(printer refused the value)");
    adf_str_free(s);
}

int
main(void)
{
    const slong prec = 64; /* bits of the real coordinate */
    const slong digits = 10; /* significant digits of the real-ball printer */
    size_t len;
    int status;

    fmpz_t A, H, d;
    arb_t r;
    adf_fball_t f;
    adf_adele_t x, y, z;
    adf_rat_t q;
    adf_place_t five;
    adf_lball_t z5, w;
    ulong count;
    const char * text = "(-100 +/- 0.001 ; -100)";

    fmpz_init_set_ui(A, 146);
    fmpz_init_set_ui(H, 1000);
    fmpz_init_set_ui(d, 1);
    arb_init(r);
    adf_fball_init(f);
    adf_adele_init(x);
    adf_adele_init(y);
    adf_adele_init(z);
    adf_rat_init(q);
    adf_lball_init(z5);
    adf_lball_init(w);

    /* 1. The finite ball (A + H Zhat)/d = 146 + 1000 Zhat (fball.h, adf_fball_set_fmpz3). */
    status = adf_fball_set_fmpz3(f, A, H, d);
    printf("%-26s %s\n", "set_fmpz3:", adf_status_str(status));
    put("f =", adf_fball_get_str(&len, f));

    /* 2. The adele x = (146.25 +/- 0.25 ; f). The real ball is exact in binary: 585/4 with radius 1/4. */
    arb_set_si(r, 585);
    arb_div_si(r, r, 4, prec);
    arb_add_error_2exp_si(r, -2);
    status = adf_adele_set_arb_fball(x, r, f);
    printf("%-26s %s\n", "set_arb_fball:", adf_status_str(status));
    put("x =", adf_adele_get_str(&len, x, digits));

    /* 3. The adele y read from its value form (text.h, adf_adele_set_str; NULL means default limits). */
    status = adf_adele_set_str(y, text, strlen(text), prec, NULL);
    printf("%-26s %s\n", "adele_set_str:", adf_status_str(status));
    put("y =", adf_adele_get_str(&len, y, digits));

    /* 4. The sum, coordinate by coordinate (adele.h, adf_adele_add; void, no status). */
    adf_adele_add(z, x, y, prec);
    put("z = x + y =", adf_adele_get_str(&len, z, digits));

    /* 5. The rationals inside z (recon.h): 46 + 1000 k in [45.999, 46.501]: only k = 0. */
    status = adf_adele_reconstruct(q, z);
    printf("%-26s %s\n", "reconstruct:", adf_status_str(status));
    if (status == ADF_OK)
        put("q =", adf_rat_get_str(&len, q));

    /* 6. The finite part of z seen at the prime 5 (lball.h): 46 + 125 Z_5. The field fin of
       adf_adele_struct is part of the documented layout (adele.h, conventions 5.5). */
    status = adf_place_prime(&five, 5);
    printf("%-26s %s\n", "place_prime 5:", adf_status_str(status));
    status = adf_lball_set_fball(z5, five, &z->fin);
    printf("%-26s %s\n", "lball_set_fball:", adf_status_str(status));
    put("z at 5 =", adf_lball_get_str(&len, z5));

    /* 7. Square roots at 5 (lroot.h). 46 = 1 mod 5 is a square modulo 5, so there are two branches,
       named by the residue of the root modulo 5: 1 and 4. The precision argument 3 asks for 5^3. */
    status = adf_lball_root_count(&count, z5, 2);
    printf("%-26s %s, %lu branches\n", "root_count:", adf_status_str(status), (unsigned long) count);
    status = adf_lball_root_seed(w, z5, 2, 1, 3);
    printf("%-26s %s\n", "root_seed seed 1:", adf_status_str(status));
    if (status == ADF_OK)
        put("sqrt(z) at 5, branch 1 =", adf_lball_get_str(&len, w));

    /* 8. A refusal: 2 is not a square modulo 5, so no point of 2 + 125 Z_5 has a square root. */
    fmpz_set_ui(A, 2);
    fmpz_set_ui(H, 125);
    adf_fball_set_fmpz3(f, A, H, d);
    adf_lball_set_fball(z5, five, f);
    status = adf_lball_root_seed(w, z5, 2, 1, 3);
    printf("%-26s %s\n", "sqrt(2 + 125 Zhat) at 5:", adf_status_str(status));

    adf_lball_clear(w);
    adf_lball_clear(z5);
    adf_rat_clear(q);
    adf_adele_clear(z);
    adf_adele_clear(y);
    adf_adele_clear(x);
    adf_fball_clear(f);
    arb_clear(r);
    fmpz_clear(d);
    fmpz_clear(H);
    fmpz_clear(A);
    flint_cleanup();
    return 0;
}
