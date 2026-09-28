/* tests/test_dlopen.c: the library is loaded with dlopen and a few functions are called through
   dlsym, without calling any inline function of the headers (work package 1.9 of docs/PLAN.md:
   "a program that loads the library with dlopen and calls a few functions through dlsym without
   the header's inline functions"; conventions 12.1, 12.4, 12.11).

   What the test claims:
   1. build/libadelefeld.so, which tests/test_exports.sh builds, opens with RTLD_NOW | RTLD_LOCAL;
   2. adf_version_check, adf_flint_version_compiled, adf_rat_init, adf_rat_clear and
      adf_sizeof_rat are found by name in the loaded object, and the code behind the pointers
      comes from that file and not from the static library of this program (dladdr);
   3. adf_version_check returns ADF_OK, because the .so was built against the FLINT that is
      installed, and adf_flint_version_compiled returns the version of that FLINT;
   4. adf_sizeof_rat returns the size that the header of the type declares, 16 bytes
      (conventions 5.1, 12.4, CV-40): this is the check a binding makes at load time;
   5. adf_rat_init leaves the exact zero 0/1 (conventions 5.1) and adf_rat_clear releases it.

   If the file is not there, the test prints one line and passes: the Makefile does not build a
   shared object, so `make check` alone must not fail. Run the script first:

       sh tests/test_exports.sh && make -j2 check

   Linking. dlopen, dlsym and dladdr are in libc since glibc 2.34, so the link of the Makefile,
   which passes only -lflint -lgmp -lm, is enough on this system (Ubuntu glibc 2.39, checked on
   2026-09-28: `make build/test_dlopen` links without -ldl). An older system needs -ldl, which is
   then given as LDLIBS='-lflint -lgmp -lm -ldl' on the make command line. */

#define _GNU_SOURCE  /* dladdr, to see which file the loaded code comes from */

#include <adelefeld.h>

#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

#include "test_runner.h"

#define ADF_SO "build/libadelefeld.so"

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

/* dladdr takes the address of the code as an object pointer, the other way round. */

static int
adf_dladdr_of(adf_version_check_fn fn, Dl_info *info)
{
    union
    {
        adf_version_check_fn fn;
        void *obj;
    } u;

    u.fn = fn;
    return dladdr(u.obj, info);
}

/* Is there a shared object to load at all? `make check` does not build one. */

static int
adf_shared_object_exists(void)
{
    FILE *f = fopen(ADF_SO, "r");

    if (f == NULL)
        return 0;
    fclose(f);
    return 1;
}

ADF_TEST(the_shared_object_is_loaded)
{
    void *h;

    if (!adf_shared_object_exists())
    {
        printf("   %s is not there (run sh tests/test_exports.sh first); nothing to load\n",
               ADF_SO);
        return;
    }

    h = dlopen(ADF_SO, RTLD_NOW | RTLD_LOCAL);
    ADF_CHECK_MSG(h != NULL, "dlopen(%s) failed: %s", ADF_SO, dlerror());
    if (h == NULL)
        return;
    ADF_CHECK(dlclose(h) == 0);
}

ADF_TEST(the_four_functions_are_found_by_name_in_the_loaded_object)
{
    void *h;
    adf_version_check_fn version_check = NULL;
    adf_flint_version_compiled_fn flint_version_compiled = NULL;
    adf_rat_init_fn rat_init = NULL;
    adf_rat_clear_fn rat_clear = NULL;
    adf_sizeof_rat_fn sizeof_rat = NULL;
    Dl_info info;
    adf_rat_struct x;

    if (!adf_shared_object_exists())
    {
        printf("   %s is not there; the names cannot be looked up\n", ADF_SO);
        return;
    }

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
       calls of the program and not of the shared object. */
    memset(&info, 0, sizeof(info));
    if (version_check != NULL && adf_dladdr_of(version_check, &info) != 0 && info.dli_fname != NULL)
        ADF_CHECK_MSG(strstr(info.dli_fname, ADF_SO) != NULL,
                      "adf_version_check was found in %s, not in %s", info.dli_fname, ADF_SO);
    else
        printf("   dladdr does not name a file for adf_version_check; the origin of the code is "
               "not checked\n");

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
