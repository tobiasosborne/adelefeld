/* tests/test_abi.c: layouts of the public structs (lane m1-headers).

   docs/conventions.md 0.4 fixes the field lists and their order (5.1 adf_rat, 5.2 adf_fball, 5.4
   adf_scaled, 5.5 adf_adele and adf_cadele, 7 adf_place_t, 8.4 adf_text_limits_t, 10.2
   adf_ctx_desc_t) and the sizes of the embedded FLINT types on x86-64 Linux with FLINT 3.0.1
   (12.11, table; **[probed]** there). The sizes and offsets below follow from those two by the C
   layout rules on a 64-bit platform with 8-byte pointers, size_t and slong (12.10, CV-42); a
   binding (conventions 12.4, CV-40) allocates values inline with exactly these numbers.

   Static assertions fail the build; the run-time checks compare the header-inline
   adf_sizeof_* / adf_alignof_* functions with sizeof / _Alignof. No library function is called. */

#include <stddef.h>

#include <flint/padic.h>

#include <adelefeld.h>

#include "test_runner.h"

#define ADF_SIZE(T, n) _Static_assert(sizeof(T) == (n), "sizeof(" #T ") != " #n)
#define ADF_ALIGN(T, n) _Static_assert(_Alignof(T) == (n), "_Alignof(" #T ") != " #n)
#define ADF_OFF(T, f, n) _Static_assert(offsetof(T, f) == (n), "offsetof(" #T ", " #f ") != " #n)
/* The type of a field. Padding can hide a narrower type (an int in an 8-byte slot keeps every
   offset), so sizes and offsets alone do not fix the layout a binding reads. The operand of
   _Generic is not evaluated (C11 6.5.1.1p3); an array field is tested after its decay. */
#define ADF_FTYPE(T, f, FT) \
    _Static_assert(_Generic(((T *) 0)->f, FT: 1, default: 0), "type of " #T "." #f " is not " #FT)

/* conventions 12.10: 64-bit words, pointers and lengths. */
ADF_SIZE(ulong, 8);
ADF_SIZE(slong, 8);
ADF_SIZE(size_t, 8);
ADF_SIZE(void *, 8);
ADF_SIZE(int, 4);

/* conventions 12.11, table of FLINT layouts. */
ADF_SIZE(fmpz, 8);
ADF_SIZE(fmpq, 16);
ADF_SIZE(mag_struct, 16);
ADF_SIZE(arf_struct, 32);
ADF_SIZE(arb_struct, 48);
ADF_SIZE(acb_struct, 96);
ADF_SIZE(padic_struct, 24);

/* conventions 5.1: { fmpq_t q; } */
ADF_SIZE(adf_rat_struct, 16);
ADF_ALIGN(adf_rat_struct, 8);
ADF_OFF(adf_rat_struct, q, 0);
ADF_FTYPE(adf_rat_struct, q, fmpq *);
ADF_SIZE(adf_rat_t, 16);

/* docs/api-s.md section 1 (slice s3-slice1): { fmpz_t c, m; } and { fmpz_t Rp, Tp, R, T; int kind; } */
ADF_SIZE(adf_resid_struct, 16);
ADF_ALIGN(adf_resid_struct, 8);
ADF_OFF(adf_resid_struct, c, 0);
ADF_OFF(adf_resid_struct, m, 8);
ADF_FTYPE(adf_resid_struct, c, fmpz *);
ADF_FTYPE(adf_resid_struct, m, fmpz *);
ADF_SIZE(adf_recon_cert_struct, 40);
ADF_ALIGN(adf_recon_cert_struct, 8);
ADF_OFF(adf_recon_cert_struct, Rp, 0);
ADF_OFF(adf_recon_cert_struct, Tp, 8);
ADF_OFF(adf_recon_cert_struct, R, 16);
ADF_OFF(adf_recon_cert_struct, T, 24);
ADF_OFF(adf_recon_cert_struct, kind, 32);
ADF_FTYPE(adf_recon_cert_struct, Rp, fmpz *);
ADF_FTYPE(adf_recon_cert_struct, Tp, fmpz *);
ADF_FTYPE(adf_recon_cert_struct, R, fmpz *);
ADF_FTYPE(adf_recon_cert_struct, T, fmpz *);
ADF_FTYPE(adf_recon_cert_struct, kind, int);

/* conventions 5.2: { fmpz_t A, H, d; int backend; const adf_modctx_struct * mctx; ulong * res; } */
ADF_SIZE(adf_fball_struct, 48);
ADF_ALIGN(adf_fball_struct, 8);
ADF_OFF(adf_fball_struct, A, 0);
ADF_OFF(adf_fball_struct, H, 8);
ADF_OFF(adf_fball_struct, d, 16);
ADF_OFF(adf_fball_struct, backend, 24);
ADF_OFF(adf_fball_struct, mctx, 32);
ADF_OFF(adf_fball_struct, res, 40);
ADF_FTYPE(adf_fball_struct, A, fmpz *);
ADF_FTYPE(adf_fball_struct, H, fmpz *);
ADF_FTYPE(adf_fball_struct, d, fmpz *);
ADF_FTYPE(adf_fball_struct, backend, int);
ADF_FTYPE(adf_fball_struct, mctx, const adf_modctx_struct *);
ADF_FTYPE(adf_fball_struct, res, ulong *);

/* conventions 5.4: { fmpq_t s; fmpz_t u; const adf_modctx_struct * mctx; int exact; } */
ADF_SIZE(adf_scaled_struct, 40);
ADF_ALIGN(adf_scaled_struct, 8);
ADF_OFF(adf_scaled_struct, s, 0);
ADF_OFF(adf_scaled_struct, u, 16);
ADF_OFF(adf_scaled_struct, mctx, 24);
ADF_OFF(adf_scaled_struct, exact, 32);
ADF_FTYPE(adf_scaled_struct, s, fmpq *);
ADF_FTYPE(adf_scaled_struct, u, fmpz *);
ADF_FTYPE(adf_scaled_struct, mctx, const adf_modctx_struct *);
ADF_FTYPE(adf_scaled_struct, exact, int);

/* conventions 5.5: { arb_t inf; adf_fball_struct fin; } and { acb_t inf; adf_fball_struct fin; } */
ADF_SIZE(adf_adele_struct, 96);
ADF_ALIGN(adf_adele_struct, 8);
ADF_OFF(adf_adele_struct, inf, 0);
ADF_OFF(adf_adele_struct, fin, 48);
ADF_SIZE(adf_cadele_struct, 144);
ADF_ALIGN(adf_cadele_struct, 8);
ADF_OFF(adf_cadele_struct, inf, 0);
ADF_OFF(adf_cadele_struct, fin, 96);
ADF_FTYPE(adf_adele_struct, inf, arb_struct *);
ADF_FTYPE(adf_adele_struct, fin, adf_fball_struct);
ADF_FTYPE(adf_cadele_struct, inf, acb_struct *);
ADF_FTYPE(adf_cadele_struct, fin, adf_fball_struct);

/* conventions 7, 12.5: { ulong opaque; }, 8 bytes, passed by value */
ADF_SIZE(adf_place_t, 8);
ADF_ALIGN(adf_place_t, 8);
ADF_FTYPE(adf_place_t, opaque, ulong);

/* conventions 8.4: { size_t max_len; slong max_exp10; slong max_prec; slong max_items; } */
ADF_SIZE(adf_text_limits_t, 32);
ADF_OFF(adf_text_limits_t, max_len, 0);
ADF_OFF(adf_text_limits_t, max_exp10, 8);
ADF_OFF(adf_text_limits_t, max_prec, 16);
ADF_OFF(adf_text_limits_t, max_items, 24);
ADF_FTYPE(adf_text_limits_t, max_len, size_t);
ADF_FTYPE(adf_text_limits_t, max_exp10, slong);
ADF_FTYPE(adf_text_limits_t, max_prec, slong);
ADF_FTYPE(adf_text_limits_t, max_items, slong);

/* conventions 10.2: { fmpz_t K; slong k; ulong * q; } */
ADF_SIZE(adf_ctx_desc_t, 24);
ADF_OFF(adf_ctx_desc_t, K, 0);
ADF_OFF(adf_ctx_desc_t, k, 8);
ADF_OFF(adf_ctx_desc_t, q, 16);
ADF_FTYPE(adf_ctx_desc_t, K, fmpz *);
ADF_FTYPE(adf_ctx_desc_t, k, slong);
ADF_FTYPE(adf_ctx_desc_t, q, ulong *);

/* conventions 9.7, 12.3: the text kind is written through a pointer; a binding reads it as an int. */
ADF_SIZE(adf_text_kind, 4);

ADF_TEST(sizeof_functions_agree_with_the_layouts)
{
    ADF_CHECK(adf_sizeof_rat() == 16 && adf_alignof_rat() == 8);
    ADF_CHECK(adf_sizeof_fball() == 48 && adf_alignof_fball() == 8);
    ADF_CHECK(adf_sizeof_scaled() == 40 && adf_alignof_scaled() == 8);
    ADF_CHECK(adf_sizeof_adele() == 96 && adf_alignof_adele() == 8);
    ADF_CHECK(adf_sizeof_cadele() == 144 && adf_alignof_cadele() == 8);
    ADF_CHECK(adf_sizeof_place() == 8 && adf_alignof_place() == 8);
    ADF_CHECK(adf_sizeof_text_limits() == 32 && adf_alignof_text_limits() == 8);
    ADF_CHECK(adf_sizeof_ctx_desc() == 24 && adf_alignof_ctx_desc() == 8);
    ADF_CHECK(adf_sizeof_resid() == 16 && adf_alignof_resid() == 8);
    ADF_CHECK(adf_sizeof_recon_cert() == 40 && adf_alignof_recon_cert() == 8);
}

/* The _t types are arrays of one struct (conventions 1, 2.1): an array parameter decays to a
   pointer, so a const adf_x_t argument is a const pointer to the struct. */
ADF_TEST(array_of_one_types)
{
    ADF_CHECK(sizeof(adf_fball_t) == sizeof(adf_fball_struct));
    ADF_CHECK(sizeof(adf_adele_t) == sizeof(adf_adele_struct));
    ADF_CHECK(sizeof(adf_cadele_t) == sizeof(adf_cadele_struct));
    ADF_CHECK(sizeof(adf_scaled_t) == sizeof(adf_scaled_struct));
    ADF_CHECK(sizeof(adf_rat_t) == sizeof(adf_rat_struct));
    ADF_CHECK(sizeof(adf_resid_t) == sizeof(adf_resid_struct));
    ADF_CHECK(sizeof(adf_recon_cert_t) == sizeof(adf_recon_cert_struct));
}

/* Slice 2 of milestone S (lane s3-slice2): the signatures of the functions of resid.h that a binding calls, and the
   status codes that adf_resid_reconstruct now returns (the Julia test tests/julia/resid.jl reads them as numbers). */
ADF_TEST(resid_function_signatures_and_status_codes)
{
    int (*rec)(adf_rat_t, adf_recon_cert_t, const adf_resid_t, const fmpz_t, const fmpz_t, slong) = NULL;
    int (*chk)(const adf_recon_cert_t, const adf_resid_t, const fmpz_t) = NULL;

    /* The assignments are inside sizeof: the compiler checks the types and no symbol of the library is
       referenced, because this file is also built without the library (lanes/m1-headers/check_headers.sh). */
    ADF_CHECK(sizeof(rec = adf_resid_reconstruct) == sizeof(rec));
    ADF_CHECK(sizeof(chk = adf_recon_cert_check) == sizeof(chk));
    ADF_CHECK(ADF_OK == 0 && ADF_NOT_DETERMINED == 1 && ADF_NOT_UNIQUE == 4 && ADF_NO_SOLUTION == 5);
    ADF_CHECK(sizeof(slong) == 8);              /* the limit is a C long on this platform: a Julia Clong */
}
