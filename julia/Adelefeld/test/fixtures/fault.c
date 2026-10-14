/* julia/Adelefeld/test/fixtures/fault.c: test fixture libraries for the load gates of the package
   (docs/api-j.md:368-370: "A test library may falsify queries; it is a test fixture, not production
   glue"). Never loaded by the package except when a test sets ADELEFELD_LIB to one of them.

   Each build defines exactly one FAULT_* macro. Every fault except FAULT_MISSING is linked against
   the real build/libadelefeld.so (with -Wl,--no-as-needed: nothing here calls it, and the default
   --as-needed would drop the dependency): the package resolves its symbols with dlsym on the handle
   of the fixture, which searches the fixture first and then its dependencies, so the one function
   defined here replaces the library's and every other symbol is the library's own.
   FAULT_MISSING is linked against nothing and defines only the gate queries before adf_status_count
   in the package's symbol list: it is the library as it was before J-status (docs/api-j.md:70-76).

   The fixture's adf_rat_init counts its calls, so that a test can show that no adf init ran after a
   rejected load (docs/api-j.md:369). It stores 0/1, which is what fmpq_init stores (an fmpz holding a
   small value is that value; the layout of conventions 12.11), so a call would still be harmless. */

#include <stddef.h>

static int fixture_inits = 0;

int fixture_rat_init_calls(void) { return fixture_inits; }

#ifndef FAULT_MISSING
void adf_rat_init(void * x)
{
    long * w = (long *) x;

    fixture_inits++;
    w[0] = 0;
    w[1] = 1;
}
#endif

#if defined(FAULT_SIZE)
size_t adf_sizeof_rat(void) { return 24; }
#elif defined(FAULT_ALIGN)
size_t adf_alignof_rat(void) { return 16; }
#elif defined(FAULT_COUNT)
int adf_status_count(void) { return 12; }
#elif defined(FAULT_VERSION)
int adf_version_check(void) { return 8; }
#elif defined(FAULT_FBALL_SIZE)
size_t adf_sizeof_fball(void) { return 56; }
#elif defined(FAULT_FBALL_ALIGN)
size_t adf_alignof_fball(void) { return 16; }
#elif defined(FAULT_NAME) || defined(FAULT_MISSING)
const char * adf_status_str(int s)
{
    switch (s)
    {
        case 0: return "OK";
        case 1: return "NOT_DETERMINED";
        case 2: return "UNIT_NOT_CERTIFIED";
        case 3: return "NEEDS_SPLIT";
        case 4: return "NOT_UNIQUE";
        case 5: return "NO_SOLUTION";
        case 6: return "NOT_UNIT";
#ifdef FAULT_NAME
        case 7: return "DOMAIN_ERROR";
#else
        case 7: return "DOMAIN";
#endif
        case 8: return "UNSUPPORTED";
        case 9: return "PARSE";
        case 10: return "LIMIT";
        default: return "UNKNOWN";
    }
}
#else
#error "define one FAULT_* macro"
#endif

#ifdef FAULT_MISSING
int adf_version_check(void) { return 0; }
const char * adf_flint_version_compiled(void) { return "3.0.1"; }
size_t adf_sizeof_rat(void) { return 16; }
size_t adf_alignof_rat(void) { return 8; }
size_t adf_sizeof_text_limits(void) { return 32; }
size_t adf_alignof_text_limits(void) { return 8; }
#endif

/* Step J-a.3: compile with FAULT_NAME as well. The layout gate precedes the status-name gate,
   so the extra name fault is unreachable. This keeps the earlier fixture branches unchanged. */
#ifdef FAULT_ADELE_SIZE
size_t adf_sizeof_adele(void) { return 104; }
#endif
#ifdef FAULT_ADELE_ALIGN
size_t adf_alignof_adele(void) { return 16; }
#endif
#ifdef FAULT_CADELE_SIZE
size_t adf_sizeof_cadele(void) { return 152; }
#endif
#ifdef FAULT_CADELE_ALIGN
size_t adf_alignof_cadele(void) { return 16; }
#endif

/* Link FAULT_MISSING against the core for this counting fixture. Its names are correct, and
   the dependency supplies adf_status_count and the later symbols. No production glue.
   Literal defaults: include/adelefeld/text.h:80-84,105-114; fixed eight-byte word layout:89-90. */
#ifdef COUNT_LIMITS
static int fixture_limits = 0;
int fixture_limits_calls(void) { return fixture_limits; }
void adf_text_limits_default(void * lim)
{
    size_t * w = (size_t *) lim;

    fixture_limits++;
    w[0] = 1048576;
    w[1] = 100000;
    w[2] = 100000;
    w[3] = 1048576;
}
#endif

/* J-a.4 enum ABI faults. Build with FAULT_NAME; the layout gate fails first. */
#ifdef FAULT_TEXT_KIND_SIZE
size_t adf_sizeof_text_kind(void) { return 8; }
#endif
#ifdef FAULT_TEXT_KIND_ALIGN
size_t adf_alignof_text_kind(void) { return 8; }
#endif

/* J-a.4 counted calls and defensive printer cases, linked with FAULT_MISSING.
   Only this test fixture uses a second handle. FIXTURE_CORE names the real library.
   The production binding still calls its single gated handle, without glue.
   Core dump strings are owned as dump.h:49-52 prescribes. Modes 1-3 falsify that contract
   to test Julia refusal and finally paths without calling FLINT on malformed input. */
#ifdef COUNT_JA4
#include <stdlib.h>
#include <dlfcn.h>

static int fixture_dump_mode = 0;
static int fixture_dumps = 0;
static int fixture_frees = 0;
static int fixture_prints = 0;
static int fixture_contexts = 0;
static int fixture_classifies = 0;

int fixture_dump_calls(void) { return fixture_dumps; }
int fixture_free_calls(void) { return fixture_frees; }
int fixture_print_calls(void) { return fixture_prints; }
int fixture_context_calls(void) { return fixture_contexts; }
int fixture_classify_calls(void) { return fixture_classifies; }
void fixture_set_dump_mode(int mode) { fixture_dump_mode = mode; }

static void * fixture_core_symbol(const char * name)
{
    static void * core = NULL;
    void * sym;

    if (core == NULL)
        core = dlopen(getenv("FIXTURE_CORE"), RTLD_NOW | RTLD_LOCAL);
    if (core == NULL || (sym = dlsym(core, name)) == NULL)
        abort();
    return sym;
}

void adf_str_free(char * s)
{
    void (* real_free)(char *) = fixture_core_symbol("adf_str_free");

    fixture_frees++;
    real_free(s);
}

#define JA4_DUMPER(name) \
char * adf_##name##_dump_str(size_t * len, const void * x) \
{ \
    char * (* real_dump)(size_t *, const void *) = fixture_core_symbol("adf_" #name "_dump_str"); \
    char * s; \
    fixture_dumps++; \
    if (fixture_dump_mode == 1) { *len = 0; return NULL; } \
    s = real_dump(len, x); \
    if (fixture_dump_mode == 2) *len = 0; \
    if (fixture_dump_mode == 3) s[*len] = 'x'; \
    return s; \
}
JA4_DUMPER(rat)
JA4_DUMPER(fball)
JA4_DUMPER(adele)
JA4_DUMPER(cadele)

#define JA4_LOADER(name) \
int adf_##name##_load_str(void * x, const char * s, size_t n, const void * ctx, const void * lim) \
{ \
    int (* real_load)(void *, const char *, size_t, const void *, const void *) = \
        fixture_core_symbol("adf_" #name "_load_str"); \
    if (ctx != NULL) { fixture_contexts++; return 8; } \
    return real_load(x, s, n, ctx, lim); \
}
JA4_LOADER(rat)
JA4_LOADER(fball)
JA4_LOADER(adele)
JA4_LOADER(cadele)

#define JA4_PRINTER(name) \
char * adf_##name##_get_str(size_t * len, const void * x, long digits) \
{ \
    char * (* real_print)(size_t *, const void *, long) = fixture_core_symbol("adf_" #name "_get_str"); \
    fixture_prints++; \
    if (digits < 1 || digits > 1000000) { *len = 0; return NULL; } \
    return real_print(len, x, digits); \
}
JA4_PRINTER(adele)
JA4_PRINTER(cadele)

#define JA4_EXACT_PRINTER(name) \
char * adf_##name##_get_str(size_t * len, const void * x) \
{ \
    char * (* real_print)(size_t *, const void *) = fixture_core_symbol("adf_" #name "_get_str"); \
    fixture_prints++; \
    return real_print(len, x); \
}
JA4_EXACT_PRINTER(rat)
JA4_EXACT_PRINTER(fball)

int adf_text_classify(int * kind, const char * s, size_t n, const void * lim)
{
    int (* real_classify)(int *, const char *, size_t, const void *) =
        fixture_core_symbol("adf_text_classify");

    fixture_classifies++;
    return real_classify(kind, s, n, lim);
}
#endif
