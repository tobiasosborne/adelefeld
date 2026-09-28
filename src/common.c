/* common.c: the three functions of include/adelefeld/common.h.

   The header is fixed for this lane (lanes/COMMON-C.md rule 6), so this file implements what the
   three comment blocks of the header say, and nothing else:

     adf_str_free(char *)              conventions 4.2:251, 12.8:1515-1518 (CV-43);
     adf_flint_version_compiled(void)  conventions 12.11:1541-1543 (CV-44);
     adf_version_check(void)           conventions 12.11:1541-1543, 3.1 for the two statuses.

   Ground truth of FLINT. The copy of the FLINT 3.0.1 sources that the repository keeps under
   refs/ (docs/sources.md, the table "flint-3.0.1") holds the documentation files padic.rst,
   acb.rst, arb.rst, fmpq.rst, fmpz_mod.rst, nmod.rst, ulong_extras.rst and acb_dirichlet.rst, but
   not the header src/flint.h, so every line of flint.h below is marked [source pending: FLINT
   3.0.1 src/flint.h under refs/src/flint-3.0.1/] and was read in the installed header
   /usr/include/flint/flint.h of FLINT 3.0.1 (__FLINT_VERSION 3, __FLINT_VERSION_MINOR 0,
   __FLINT_VERSION_PATCHLEVEL 1, /usr/include/flint/flint.h:94-96). The line numbers of that file
   agree with the ones the header of adelefeld cites (flint.h:94-97, 105, 193, 201-204), which is
   why the citations below are usable:
     flint.h:97   #define FLINT_VERSION "3.0.1"      [source pending: as above]
     flint.h:105  FLINT_DLL extern char flint_version[];
     flint.h:204  void flint_free(void * ptr);
   The other citations are to docs/conventions.md of this repository and need no source. */

#include <adelefeld.h>

/* ---- adf_str_free ---- */

/* conventions 4.2:249-252: a string returned by adf_*_get_str or adf_*_dump_str is allocated
   with flint_malloc and the caller frees it with adf_str_free, which calls flint_free
   (flint.h:204); conventions 12.8:1517-1518 says the same, and CV-43 (conventions 13, the table
   at conventions:1600) says the function is exported so that a binding need not find FLINT's
   allocator. The header of this function: "It calls flint_free (flint.h:201-204). s = NULL does
   nothing (as free(NULL))." Nothing is added: the memory of a string of ours is the memory of
   flint_malloc, and flint_free is the matching free. */

void
adf_str_free(char *s)
{
    flint_free(s);
}

/* ---- adf_flint_version_compiled ---- */

/* conventions 12.11:1541-1543: "the library exports const char * adf_flint_version_compiled(void)
   and checks flint_version against it in adf_version_check(void), which returns ADF_OK or
   ADF_UNSUPPORTED" (CV-44, conventions:1601). The string is the FLINT_VERSION of the header this
   file was compiled against (flint.h:97, a string literal, so it is static and the caller must
   not free it, as the header of the function says). Nothing is stored: the value is a constant of
   the translation unit. */

const char *
adf_flint_version_compiled(void)
{
    return FLINT_VERSION;
}

/* ---- adf_version_check ---- */

/* The comparison of two version strings. Only the major and the minor number are read, as
   conventions 12.11:1541-1543 says ("the same major and minor version"); whatever follows the minor
   number is not read. The numbers are compared as decimal strings, so that "3.1" and "3.01" are
   the same number and "3.10" is not the number of "3.1", and no value is computed that could
   overflow.

   A string is a version when it begins with digits, then a '.', then digits. A string that is
   not a version compares unequal to every string, also to itself: nothing then proves that the
   two versions agree, and ADF_OK is a promise that they do. The run-time string of FLINT is
   always of that shape, so the case does not arise for a caller; the test file reaches it
   through the test-only entry at the end of this file. */

/* A run of decimal digits inside a version string. d and n are the run without its leading zeros,
   a run of n characters being d[0], d[1], ..., d[n-1]. end is the number of digits that stand in
   the string, with the leading zeros, so that the character after the run is s[end]. The strip
   below never removes the last digit of a run, so n is 0 exactly when the run is empty, and end
   is 0 exactly when the run is empty. */
typedef struct
{
    const char *d;
    size_t n;
    size_t end;
} adf_ver_run;

/* The run of digits that starts at s. Leading zeros are dropped, except that the run "0" keeps
   one digit, so that "3" and "03" give the same run and "0" is not mistaken for "nothing". */
static adf_ver_run
adf_ver_read(const char *s)
{
    adf_ver_run r;
    size_t k = 0;

    r.d = s;
    r.n = 0;
    while (s[r.n] >= '0' && s[r.n] <= '9')
        r.n++;
    r.end = r.n;
    while (k + 1 < r.n && r.d[k] == '0')
        k++;
    r.d += k;
    r.n -= k;
    return r;
}

/* Do the two runs stand for the same number? The digits are read from the right, which is the
   same as writing the shorter run with '0' in front of it, and that is exact because a run
   without leading zeros is as long as its number needs, except for the single digit "0". Every
   position of the longer run is compared, so the answer is 1 exactly when the two numbers are
   equal; which position differs first does not matter, since one difference is enough. */
static int
adf_ver_run_equal(adf_ver_run a, adf_ver_run b)
{
    size_t i;

    for (i = 1; i <= a.n || i <= b.n; i++)
    {
        char ca = (i <= a.n) ? a.d[a.n - i] : '0';
        char cb = (i <= b.n) ? b.d[b.n - i] : '0';

        if (ca != cb)
            return 0;
    }
    return 1;
}

/* Do the two version strings have the same major and the same minor number? 1 for yes, 0 for no
   and for a string that is not a version. */
static int
adf_version_major_minor_equal(const char *a, const char *b)
{
    adf_ver_run am = adf_ver_read(a);
    adf_ver_run bm = adf_ver_read(b);
    adf_ver_run an, bn;

    if (am.n == 0)
        return 0;
    if (bm.n == 0)
        return 0;
    if (a[am.end] != '.')
        return 0;
    if (b[bm.end] != '.')
        return 0;
    an = adf_ver_read(a + am.end + 1);
    bn = adf_ver_read(b + bm.end + 1);
    if (an.n == 0)
        return 0;
    if (bn.n == 0)
        return 0;
    return adf_ver_run_equal(am, bm) && adf_ver_run_equal(an, bn);
}

/* conventions 12.11:1541-1543: adf_version_check compares flint_version (flint.h:105) with
   adf_flint_version_compiled() and returns ADF_OK (0) when their major and minor versions
   agree, ADF_UNSUPPORTED (8) otherwise (conventions 3.1 for the two values; CV-44). It writes
   nothing, which is the rule of conventions 4.3 for a non-OK status as well: the function has no
   output argument. */

int
adf_version_check(void)
{
    if (adf_version_major_minor_equal(flint_version, adf_flint_version_compiled()))
        return ADF_OK;
    return ADF_UNSUPPORTED;
}

#ifdef ADF_COMMON_TEST
/* The test-only entry, which tests/test_common.c declares and calls. It is not part of the
   library: src/common.c is compiled without ADF_COMMON_TEST, so the symbol exists in no library
   and no binding can reach it. It gives the test access to the static comparison above, whose
   mismatch cases the installed FLINT cannot produce. */

int adf_test_version_major_minor_equal(const char *a, const char *b);

int
adf_test_version_major_minor_equal(const char *a, const char *b)
{
    return adf_version_major_minor_equal(a, b);
}
#endif
