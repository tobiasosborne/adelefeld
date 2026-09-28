/* lanes/m1-dump/stub_dump.c: the stub of src/dump.c used for the red runs of the lane
   (lanes/m1-dump/redgreen.log): every loader and inspector returns ADF_PARSE, every dumper "x".
   Copy it over src/dump.c to repeat a red run; copy the implementation back afterwards. */
#include <string.h>
#include "adelefeld/dump.h"

static char * stub_str(size_t * len) { char * s = flint_malloc(2); s[0] = 'x'; s[1] = 0; *len = 1; return s; }
#define STUB_TYPE(T, t_t) \
int adf_##T##_load_str(t_t x, const char * s, size_t len, const adf_modctx_struct * ctx, const adf_text_limits_t * lim) \
{ (void) x; (void) s; (void) len; (void) ctx; (void) lim; return ADF_PARSE; } \
int adf_##T##_load_str_binds(t_t x, const char * s, size_t len, const adf_modctx_struct * const * b, size_t nb, const adf_text_limits_t * lim) \
{ (void) x; (void) s; (void) len; (void) b; (void) nb; (void) lim; return ADF_PARSE; } \
char * adf_##T##_dump_str(size_t * len, const t_t x) { (void) x; return stub_str(len); } \
int adf_##T##_dump_inspect(size_t * nctx, adf_ctx_desc_t * d, const char * s, size_t len, const adf_text_limits_t * lim) \
{ (void) nctx; (void) d; (void) s; (void) len; (void) lim; return ADF_PARSE; }
STUB_TYPE(rat, adf_rat_t)
STUB_TYPE(fball, adf_fball_t)
STUB_TYPE(scaled, adf_scaled_t)
STUB_TYPE(adele, adf_adele_t)
STUB_TYPE(cadele, adf_cadele_t)
char * adf_scaled_get_str(size_t * len, const adf_scaled_t x) { (void) x; return stub_str(len); }
int adf_dump_ctx_occurrence(adf_ctx_desc_t * d, const char * s, size_t len, size_t occurrence,
                            const adf_text_limits_t * lim);
int adf_dump_ctx_occurrence(adf_ctx_desc_t * d, const char * s, size_t len, size_t occurrence,
                            const adf_text_limits_t * lim)
{ (void) d; (void) s; (void) len; (void) occurrence; (void) lim; return ADF_PARSE; }
