/* tests/test_dlopen.c: the library is loaded with dlopen and a few functions are called through
   dlsym, without calling any inline function of the headers (work package 1.9 of docs/PLAN.md:
   "a program that loads the library with dlopen and calls a few functions through dlsym without
   the header's inline functions"; conventions 12.1, 12.4, 12.11).

   What the test claims:
   1. build/libadelefeld.so opens with RTLD_NOW | RTLD_LOCAL.  The file is built by this
      test when it is absent or older than the static library build/libadelefeld.a: the test
      runs `sh tests/test_exports.sh` through system(), which is the one place of the tree
      that builds the shared object (the Makefile does not), so `make check` in a fresh tree
      checks something instead of nothing.  If the build fails the test fails;
   2. adf_version_check, adf_flint_version_compiled, adf_rat_init, adf_rat_clear and
      adf_sizeof_rat are found by name in the loaded object, and the code behind each of the
      five pointers comes from that file and not from the static library of this program
      (dladdr);
   3. adf_version_check returns ADF_OK, because the .so was built against the FLINT that is
      installed, and adf_flint_version_compiled returns the version of that FLINT;
   4. adf_sizeof_rat returns the size that the header of the type declares, 16 bytes
      (conventions 5.1, 12.4, CV-40): this is the check a binding makes at load time;
   5. adf_rat_init leaves the exact zero 0/1 (conventions 5.1) and adf_rat_clear releases it.

   The test runs from the repository root, as the Makefile runs it, because the two paths
   above and the script are relative:

       make -j2 check

   Linking. dlopen, dlsym and dladdr are in libc since glibc 2.34, so the link of the Makefile,
   which passes only -lflint -lgmp -lm, is enough on this system (Ubuntu glibc 2.39, checked on
   2026-09-28: `make build/test_dlopen` links without -ldl). An older system needs -ldl, which is
   then given as LDLIBS='-lflint -lgmp -lm -ldl' on the make command line. */

#define _GNU_SOURCE  /* dladdr, to see which file the loaded code comes from */

#include <sys/stat.h>
#include <sys/types.h>

#include <adelefeld.h>

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_runner.h"

#define ADF_SO "build/libadelefeld.so"
#define ADF_LIB "build/libadelefeld.a"
/* The one command of the tree that builds the shared object; it prints its own report. */
#define ADF_BUILD_SO "sh tests/test_exports.sh"

/* The types of the five functions, as they stand in the comment blocks of the headers. */

typedef int (*adf_version_check_fn)(void);
typedef const char *(*adf_flint_version_compiled_fn)(void);
typedef void (*adf_rat_init_fn)(adf_rat_struct *);
typedef void (*adf_rat_clear_fn)(adf_rat_struct *);
typedef size_t (*adf_sizeof_rat_fn)(void);

/* dlsym returns void *, and ISO C has no conversion from an object pointer to a function pointer
   (C11 6.3.2.3p7); POSIX asks for the conversion to work. A union holds the two without a
   conversion that -Wpedantic would report. */

#define ADF_DLSYM(handle, name, type, out)                                          \
    do                                                                              \
    {                                                                               \
        union                                                                       \
        {                                                                           \
            void *obj;                                                              \
            type fn;                                                                 \
        } u;                                                                        \
        u.obj = dlsym((handle), (name));                                             \
        (out) = u.fn;                                                               \
    }                                                                               \
    while (0)

/* dladdr takes the address of the code as an object pointer, the other way round.  The macro
   makes that conversion for a function pointer of any type, without a cast that ISO C has no
   rule for (C11 6.3.2.3p7) and that -Wpedantic would report. */

#define ADF_DLADDR(fn, info, found)                                                   \
    do                                                                                \
    {                                                                                 \
        union                                                                         \
        {                                                                             \
            __typeof__(fn) fn;                                                        \
            void *obj;                                                                \
        } u;                                                                          \
        u.fn = (fn);                                                                  \
        (found) = dladdr(u.obj, (info));                                              \
    }                                                                                 \
    while (0)

/* adf_shared_object_ready(): 1 when build/libadelefeld.so is there and no older than the
   static library, 0 otherwise.  When it is not, `sh tests/test_exports.sh` is run, which
   builds the shared object from src/ and copies it there; the answer is asked again after
   that.  A shared object from an older tree would answer for the old tree, so its age is
   part of the question.  Without this the test would check nothing in a fresh `make check`,
   which is a test that cannot fail (COMMON.md item 3). */
static int
adf_shared_object_ready(void)
{
    struct stat so, lib;

    if (stat(ADF_SO, &so) == 0 && stat(ADF_LIB, &lib) == 0 && so.st_mtime >= lib.st_mtime)
        return 1;
    if (system(ADF_BUILD_SO) != 0)
    {
        printf("   %s: the command failed\n", ADF_BUILD_SO);
        return 0;
    }
    return stat(ADF_SO, &so) == 0;
}

ADF_TEST(the_shared_object_is_loaded)
{
    void *h;

    ADF_CHECK_MSG(adf_shared_object_ready(), "%s is not there and could not be built by"
                  " \"%s\"; run this from the repository root", ADF_SO, ADF_BUILD_SO);
    if (!adf_shared_object_ready())
        return;

    h = dlopen(ADF_SO, RTLD_NOW | RTLD_LOCAL);
    ADF_CHECK_MSG(h != NULL, "dlopen(%s) failed: %s", ADF_SO, dlerror());
    if (h == NULL)
        return;
    ADF_CHECK(dlclose(h) == 0);
}

ADF_TEST(the_five_functions_are_found_by_name_in_the_loaded_object)
{
    void *h;
    adf_version_check_fn version_check = NULL;
    adf_flint_version_compiled_fn flint_version_compiled = NULL;
    adf_rat_init_fn rat_init = NULL;
    adf_rat_clear_fn rat_clear = NULL;
    adf_sizeof_rat_fn sizeof_rat = NULL;
    Dl_info info;
    adf_rat_struct x;
    int found;

    ADF_CHECK_MSG(adf_shared_object_ready(), "%s is not there and could not be built by"
                  " \"%s\"; run this from the repository root", ADF_SO, ADF_BUILD_SO);
    if (!adf_shared_object_ready())
        return;

    h = dlopen(ADF_SO, RTLD_NOW | RTLD_LOCAL);
    ADF_CHECK(h != NULL);
    if (h == NULL)
        return;

    ADF_DLSYM(h, "adf_version_check", adf_version_check_fn, version_check);
    ADF_CHECK_MSG(version_check != NULL, "dlsym(adf_version_check): %s", dlerror());
    ADF_DLSYM(h, "adf_rat_init", adf_rat_init_fn, rat_init);
    ADF_CHECK_MSG(rat_init != NULL, "dlsym(adf_rat_init): %s", dlerror());
    ADF_DLSYM(h, "adf_rat_clear", adf_rat_clear_fn, rat_clear);
    ADF_CHECK_MSG(rat_clear != NULL, "dlsym(adf_rat_clear): %s", dlerror());
    ADF_DLSYM(h, "adf_sizeof_rat", adf_sizeof_rat_fn, sizeof_rat);
    ADF_CHECK_MSG(sizeof_rat != NULL, "dlsym(adf_sizeof_rat): %s", dlerror());
    /* The version string of the FLINT the .so was compiled against; a binding reads it. */
    ADF_DLSYM(h, "adf_flint_version_compiled", adf_flint_version_compiled_fn,
              flint_version_compiled);
    ADF_CHECK_MSG(flint_version_compiled != NULL, "dlsym(adf_flint_version_compiled): %s",
                  dlerror());

    /* 2. The code behind the pointers is the code of the loaded file. This program links the
       static library, which holds the same names, so without this check the calls below could be
       calls of the program and not of the shared object.  All five are asked, not one. */
    if (version_check != NULL)
    {
        memset(&info, 0, sizeof(info));
        ADF_DLADDR(version_check, &info, found);
        ADF_CHECK_MSG(found != 0, "dladdr says nothing about adf_version_check");
        ADF_CHECK_MSG(found == 0 || info.dli_fname == NULL
                      || strstr(info.dli_fname, ADF_SO) != NULL,
                      "adf_version_check was found in %s, not in %s",
                      info.dli_fname ? info.dli_fname : "(no file)", ADF_SO);
    }
    if (flint_version_compiled != NULL)
    {
        memset(&info, 0, sizeof(info));
        ADF_DLADDR(flint_version_compiled, &info, found);
        ADF_CHECK_MSG(found != 0, "dladdr says nothing about adf_flint_version_compiled");
        ADF_CHECK_MSG(found == 0 || info.dli_fname == NULL
                      || strstr(info.dli_fname, ADF_SO) != NULL,
                      "adf_flint_version_compiled was found in %s, not in %s",
                      info.dli_fname ? info.dli_fname : "(no file)", ADF_SO);
    }
    if (rat_init != NULL)
    {
        memset(&info, 0, sizeof(info));
        ADF_DLADDR(rat_init, &info, found);
        ADF_CHECK_MSG(found != 0, "dladdr says nothing about adf_rat_init");
        ADF_CHECK_MSG(found == 0 || info.dli_fname == NULL
                      || strstr(info.dli_fname, ADF_SO) != NULL,
                      "adf_rat_init was found in %s, not in %s",
                      info.dli_fname ? info.dli_fname : "(no file)", ADF_SO);
    }
    if (rat_clear != NULL)
    {
        memset(&info, 0, sizeof(info));
        ADF_DLADDR(rat_clear, &info, found);
        ADF_CHECK_MSG(found != 0, "dladdr says nothing about adf_rat_clear");
        ADF_CHECK_MSG(found == 0 || info.dli_fname == NULL
                      || strstr(info.dli_fname, ADF_SO) != NULL,
                      "adf_rat_clear was found in %s, not in %s",
                      info.dli_fname ? info.dli_fname : "(no file)", ADF_SO);
    }
    if (sizeof_rat != NULL)
    {
        memset(&info, 0, sizeof(info));
        ADF_DLADDR(sizeof_rat, &info, found);
        ADF_CHECK_MSG(found != 0, "dladdr says nothing about adf_sizeof_rat");
        ADF_CHECK_MSG(found == 0 || info.dli_fname == NULL
                      || strstr(info.dli_fname, ADF_SO) != NULL,
                      "adf_sizeof_rat was found in %s, not in %s",
                      info.dli_fname ? info.dli_fname : "(no file)", ADF_SO);
    }

    /* 3. The version check of the loaded library. */
    if (version_check != NULL)
    {
        ADF_CHECK_MSG(version_check() == ADF_OK, "the loaded library says %d for the version "
                                                   "check, and was built against FLINT %s",
                      version_check(),
                      flint_version_compiled != NULL ? flint_version_compiled() : "?");
    }
    if (flint_version_compiled != NULL)
    {
        ADF_CHECK(strcmp(flint_version_compiled(), FLINT_VERSION) == 0);
        ADF_CHECK(strncmp(flint_version_compiled(), "3.0", 3) == 0);
    }

    /* 4. The layout of adf_rat as a binding sees it (conventions 12.4). */
    if (sizeof_rat != NULL)
    {
        ADF_CHECK_MSG(sizeof_rat() == sizeof(adf_rat_struct),
                      "adf_sizeof_rat says %lu, the header of the type gives %lu",
                      (unsigned long) sizeof_rat(), (unsigned long) sizeof(adf_rat_struct));
        ADF_CHECK_MSG(sizeof_rat() == 16, "adf_rat_struct is one fmpq, 16 bytes on 64 bits "
                                          "(conventions 5.1); the loaded library says %lu",
                      (unsigned long) sizeof_rat());
    }

    /* 5. A value of the loaded library: the exact zero 0/1, and its clear. */
    if (rat_init != NULL && rat_clear != NULL)
    {
        rat_init(&x);
        ADF_CHECK_MSG(fmpq_is_zero(x.q), "adf_rat_init did not leave the exact zero, the "
                                         "numerator is %ld",
                      (long) fmpq_numref(x.q)[0]);
        ADF_CHECK_MSG(fmpq_denref(x.q)[0] == 1, "adf_rat_init did not leave the zero as 0/1, the "
                                             "denominator is %ld", (long) fmpq_denref(x.q)[0]);
        ADF_CHECK(fmpq_is_canonical(x.q));
        rat_clear(&x);
    }

    ADF_CHECK(dlclose(h) == 0);
}
