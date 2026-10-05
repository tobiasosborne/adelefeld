/* lanes/q-review2/can_eval.c: build adf_qclass values by hand from a text description on stdin
   and print adf_qclass_is_canonical, plus per-piece canonical triples read back from C.

   Input grammar (one value per EVAL ... END):
     EVAL <form> <len> <nullflag> <npieces>
     PIECE <midman> <midexp> <radman> <radexp> <ftype> <A> <H> <d> <ctxidx> <r0> [<r1> ...]
     END
   Output:
     V <verdict> <len> then for i < len: " 1 <A> <H> <d>" if the member adele is canonical,
     else " 0".

   midman "inf" makes the real ball infinite. radman must fit ulong.
   ftype: 0 global, 1 local, 2 global with mctx, 3 backend tag 2, 4 local with mctx NULL,
   5 global with a res array. ctxidx indexes the context table, -1 meaning NULL. */
#include <adelefeld.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { ulong q[4]; slong k; ulong K; const adf_modctx_struct * ctx; } CtxT;

static CtxT cxt[4];

static void cxt_init(void)
{
    slong i, j;
    static const ulong q0[2] = {2, 3}, q1[2] = {5, 7}, q2[3] = {3, 5, 7},
                        q3[2] = {243, 4294967296UL};
    const ulong * qs[4];
    slong ks[4];
    qs[0] = q0; qs[1] = q1; qs[2] = q2; qs[3] = q3;
    ks[0] = 2; ks[1] = 2; ks[2] = 3; ks[3] = 2;
    for (i = 0; i < 4; i++) {
        cxt[i].k = ks[i];
        cxt[i].K = 1;
        for (j = 0; j < ks[i]; j++) { cxt[i].q[j] = qs[i][j]; cxt[i].K *= qs[i][j]; }
        if (adf_modctx_new_blocks((adf_modctx_struct **) &cxt[i].ctx, cxt[i].q, ks[i])
            != ADF_OK) { printf("ctx fail\n"); exit(2); }
    }
}

#define MAXP 16
#define MAXR 4

typedef struct {
    char midman[400010], midexp[64], radman[64], radexp[64];
    int ftype, ctxidx, nres;
    char A[128], H[128], d[128];
    char res[MAXR][32];
} PieceT;

static void set_arb_mid(arb_t a, const char *man, const char *exp)
{
    fmpz_t m, e;
    if (strcmp(man, "inf") == 0) { arf_pos_inf(arb_midref(a)); return; }
    fmpz_init(m); fmpz_init(e);
    if (fmpz_set_str(m, man, 10) || fmpz_set_str(e, exp, 10)) { printf("bad int\n"); exit(2); }
    arf_set_fmpz_2exp(arb_midref(a), m, e);
    fmpz_clear(m); fmpz_clear(e);
}

static void set_arb_rad(arb_t a, const char *man, const char *exp)
{
    mag_set_ui_2exp_si(arb_radref(a), strtoull(man, NULL, 10), strtol(exp, NULL, 10));
}

static void build_piece(adf_adele_struct *a, const PieceT *p)
{
    slong i, k = (p->ctxidx >= 0 && p->ctxidx < 4) ? cxt[p->ctxidx].k : 0;
    set_arb_mid(a->inf, p->midman, p->midexp);
    set_arb_rad(a->inf, p->radman, p->radexp);
    if (fmpz_set_str(a->fin.A, p->A, 10) || fmpz_set_str(a->fin.H, p->H, 10) ||
        fmpz_set_str(a->fin.d, p->d, 10)) { printf("bad int\n"); exit(2); }
    switch (p->ftype) {
    case 0: a->fin.backend = ADF_GLOBAL; a->fin.mctx = NULL; break;
    case 1: a->fin.backend = ADF_LOCAL; a->fin.mctx = (k > 0) ? cxt[p->ctxidx].ctx : NULL; break;
    case 2: a->fin.backend = ADF_GLOBAL; a->fin.mctx = (k > 0) ? cxt[p->ctxidx].ctx : NULL; break;
    case 3: a->fin.backend = 2; a->fin.mctx = NULL; break;
    case 4: a->fin.backend = ADF_LOCAL; a->fin.mctx = NULL; break;
    default: a->fin.backend = ADF_GLOBAL; a->fin.mctx = NULL; break;
    }
    if (p->ftype == 1 || p->ftype == 2 || p->ftype == 5) {
        a->fin.res = flint_calloc((size_t) (k > 0 ? k : 1), sizeof(ulong));
        for (i = 0; i < k && i < p->nres && i < MAXR; i++) a->fin.res[i] = strtoull(p->res[i], NULL, 10);
    }
}

static void print_fmpz(const fmpz_t v)
{
    char *s = fmpz_get_str(NULL, 10, v);
    printf(" %s", s);
    flint_free(s);
}

int main(void)
{
    static char line[4000000];
    PieceT pieces[MAXP];
    adf_qclass_t x;
    int form = 0, nullflag = 0, npieces = 0, slot = 0, verdict, i, done = 0;
    slong len = 1, j;
    adf_adele_struct * keep;

    cxt_init();
    adf_qclass_init(x);
    keep = NULL;
    while (fgets(line, sizeof(line), stdin)) {
        if (strncmp(line, "PIECE", 5) == 0) {
            PieceT * p = &pieces[slot];
            if (slot >= MAXP) { printf("too many pieces\n"); return 2; }
            memset(p, 0, sizeof(*p));
            p->nres = MAXR;
            sscanf(line, "PIECE %400009s %63s %63s %63s %d %127s %127s %127s %d %31s %31s %31s %31s",
                   p->midman, p->midexp, p->radman, p->radexp, &p->ftype, p->A, p->H, p->d,
                   &p->ctxidx, p->res[0], p->res[1], p->res[2], p->res[3]);
            slot++;
            continue;
        }
        if (strncmp(line, "EVAL", 4) == 0) {
            form = 0; len = 1; nullflag = 0; npieces = 0;
            sscanf(line, "EVAL %d %ld %d %d", &form, &len, &nullflag, &npieces);
            adf_qclass_clear(x);
            x->form = form;
            x->len = len;
            x->piece = flint_calloc((size_t) (len > 0 ? len : 1), sizeof(*x->piece));
            for (i = 0; i < len && i < MAXP; i++) adf_adele_init(x->piece + i);
            slot = 0;
            done = 0;
            continue;
        }
        if (strncmp(line, "END", 3) == 0) {
            if (done) continue;
            done = 1;
            for (j = 0; j < slot && j < len && j < MAXP; j++) build_piece(x->piece + j, &pieces[j]);
            if (nullflag) { keep = x->piece; x->piece = NULL; }
            verdict = adf_qclass_is_canonical(x);
            if (nullflag) { x->piece = keep; keep = NULL; }
            printf("V %d %ld", verdict, len);
            for (j = 0; j < len && j < MAXP; j++) {
                if (adf_adele_is_canonical(x->piece + j)) {
                    fmpz_t A, H, d;
                    fmpz_init(A); fmpz_init(H); fmpz_init(d);
                    adf_fball_get_fmpz3(A, H, d, &x->piece[j].fin);
                    printf(" 1");
                    print_fmpz(A); print_fmpz(H); print_fmpz(d);
                    fmpz_clear(A); fmpz_clear(H); fmpz_clear(d);
                }
                else printf(" 0");
            }
            printf("\n");
            fflush(stdout);
            continue;
        }
    }
    flint_free(keep);
    adf_qclass_clear(x);
    for (i = 0; i < 4; i++) adf_modctx_free((adf_modctx_struct *) cxt[i].ctx);
    return 0;
}
