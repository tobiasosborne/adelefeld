/* probe: with the arf and mag exponents (ARF_EXP, MAG_EXP) set to a given value, which exponent
   does arb_get_interval_fmpz_2exp return, and how many bits are the two integers?  The result
   decides the bound of the helper of src/recon.c that turns the interval into exact rationals.
   Only sizes are printed: the integers themselves have 300000 digits.

   Conventions measured: the arf value m*2^e (m odd) is stored with ARF_EXP = e + bits(m)
   (arf.rst:14-20: the internal exponent is the unique e with 0.5 <= |mantissa| < 1), and
   mag_set_ui_2exp_si(r, 1, y) stores 2^y with MAG_EXP = y + 1 (measured: mag(2^0) has
   MAG_EXP = 1, mag(2^-1) has MAG_EXP = 0). So a ball whose ARF_EXP and MAG_EXP are E has the
   midpoint and the radius 2^(E-1). */

#include <stdio.h>
#include <string.h>

#include <flint/arb.h>
#include <flint/arf.h>
#include <flint/mag.h>
#include <flint/fmpz.h>

/* the real ball with midpoint 2^(E-1) (ARF_EXP = E) and radius 2^(R-1) (MAG_EXP = R). */
static void
set_real_exp(arb_t x, slong E, slong R)
{
    fmpz_t one, e;

    fmpz_init_set_ui(one, 1);
    fmpz_init(e);
    fmpz_set_si(e, E - 1);
    arb_set_fmpz_2exp(x, one, e);
    fmpz_set_si(e, R - 1);
    mag_set_ui_2exp_si(arb_radref(x), 1, R - 1);
    fmpz_clear(e);
    fmpz_clear(one);
}

static void
run(const char * label, slong E, slong R)
{
    arb_t x;
    fmpz_t a, b, e;

    arb_init(x);
    set_real_exp(x, E, R);
    printf("%-34s ARF_EXP=%ld MAG_EXP=%ld", label, (long) ARF_EXP(arb_midref(x)),
           (long) MAG_EXP(arb_radref(x)));
    fmpz_init(a); fmpz_init(b); fmpz_init(e);
    arb_get_interval_fmpz_2exp(a, b, e, x);
    printf(" | a_bits=%ld b_bits=%ld exp=", (long) fmpz_bits(a), (long) fmpz_bits(b));
    fmpz_print(e);
    printf("\n");
    fflush(stdout);
    fmpz_clear(a); fmpz_clear(b); fmpz_clear(e);
    arb_clear(x);
}

int
main(void)
{
    slong E = 1048576;

    /* at the bound, one above it and one below it, in both signs */
    run("E=1048576 R=1048576", E, E);
    run("E=1048576 R=0", E, 0);
    run("E=0 R=1048576", 0, E);
    run("E=0 R=-1048576", 0, -E);
    run("E=-1048576 R=0", -E, 0);
    run("E=-1048576 R=-1048576", -E, -E);
    run("E=1048577 R=1048577", E + 1, E + 1);
    run("E=1048577 R=0", E + 1, 0);
    run("E=0 R=1048577", 0, E + 1);
    run("E=0 R=-1048577", 0, -E - 1);
    run("E=-1048577 R=0", -E - 1, 0);
    run("E=-1048577 R=-1048577", -E - 1, -E - 1);
    run("E=1048575 R=1048575", E - 1, E - 1);
    run("E=5 R=-9", 5, -9);
    run("E=-9 R=5", -9, 5);
    return 0;
}
