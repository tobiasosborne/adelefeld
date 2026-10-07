/* Count direct temporary init/clear pairs in a scratch compilation of psi.c.
   This catches the skipped-array-clear mutant without LeakSanitizer. It is not
   a general allocation or leak checker. The production source is not edited. */
#include <adelefeld.h>
#include <stdio.h>
#include <stdlib.h>

static long qlive, arflive;
static void traced_qinit(fmpq_t q) { fmpq_init(q); qlive++; }
static void traced_qclear(fmpq_t q) { fmpq_clear(q); qlive--; }
static void traced_arfinit(arf_t x) { arf_init(x); arflive++; }
static void traced_arfclear(arf_t x) { arf_clear(x); arflive--; }
#define fmpq_init traced_qinit
#define fmpq_clear traced_qclear
#define arf_init traced_arfinit
#define arf_clear traced_arfclear
#ifndef ADF_PSI_SOURCE
#define ADF_PSI_SOURCE "../../src/psi.c"
#endif
#include ADF_PSI_SOURCE
#undef fmpq_init
#undef fmpq_clear
#undef arf_init
#undef arf_clear

int main(void)
{
    adf_adele_t x; adf_rat_t a; acb_t z;
    adf_adele_init(x); adf_rat_init(a); acb_init(z);
    fmpq_one(a->q);
    fmpz_mul_2exp(fmpq_denref(a->q), fmpq_denref(a->q), 2000);
    fmpz_add_ui(fmpq_denref(a->q), fmpq_denref(a->q), 1);
    adf_fball_set_rat(&x->fin, a);
    if (adf_adele_psi_tate(z, x, 128) != ADF_OK || qlive != 0 || arflive != 0) {
        fprintf(stderr, "test_lifecycle.c: qlive=%ld arflive=%ld\n", qlive, arflive);
        return 1;
    }
    adf_adele_clear(x); adf_rat_clear(a); acb_clear(z); flint_cleanup();
    printf("psi lifecycle: 1 call, 0 unmatched q/arf temporaries\n"); return 0;
}
