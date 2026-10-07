/* Temporary weak definitions for independent assertion reds before each implementation. */
#include <adelefeld.h>
__attribute__((weak)) void adf_char_conj(adf_char_t y, const adf_char_t x) { adf_char_set(y, x); }
__attribute__((weak)) int adf_char_load_str(adf_char_t x, const char *s, size_t n,
    const adf_modctx_struct *ctx, const adf_text_limits_t *lim)
{ (void) x; (void) s; (void) n; (void) ctx; (void) lim; return ADF_UNSUPPORTED; }
__attribute__((weak)) int adf_char_load_str_binds(adf_char_t x, const char *s, size_t n,
    const adf_modctx_struct *const *b, size_t nb, const adf_text_limits_t *lim)
{ (void) b; (void) nb; return adf_char_load_str(x, s, n, NULL, lim); }
__attribute__((weak)) char *adf_char_dump_str(size_t *n, const adf_char_t x)
{ (void) x; *n = 0; return NULL; }
__attribute__((weak)) int adf_char_dump_inspect(size_t *nc, adf_ctx_desc_t *d,
    const char *s, size_t n, const adf_text_limits_t *lim)
{ (void) nc; (void) d; (void) s; (void) n; (void) lim; return ADF_UNSUPPORTED; }
