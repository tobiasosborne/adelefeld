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
