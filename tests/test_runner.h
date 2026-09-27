/* tests/test_runner.h: a minimal header-only test runner for adelefeld, C11, libc only.

   Use:

       #include "test_runner.h"      <- the last include of the test file

       ADF_TEST(two_plus_three_is_five)
       {
           ADF_CHECK(2 + 3 == 5);
           ADF_CHECK_MSG(1 == 2, "arithmetic is broken: %d != %d", 1, 2);
       }

   The runner supplies main(). A test file that wants its own main defines ADF_TEST_NO_MAIN
   before including this header.

   Registration uses the constructor attribute, so the runner needs GCC or Clang: there is no
   portable C11 way to run code at program start. Each test file is its own executable, so the
   only order ever relied on is the order of the constructors inside one file. Test names must
   be unique within a file.

   Every ADF_CHECK that fails prints one line with the file, the line, the test name, the text of
   the condition and, for ADF_CHECK_MSG, the formatted message. The summary names the number of
   tests, of checks and of failures. The exit status is 1 if any check failed. */

#ifndef ADF_TEST_RUNNER_H
#define ADF_TEST_RUNNER_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#if !defined(__GNUC__) && !defined(__clang__)
#error "the adelefeld test runner needs the constructor attribute: use GCC or Clang"
#endif

/* A helper of the runner may be unused in a given test file; that is not a warning. */

#if defined(__GNUC__) || defined(__clang__)
#define ADF_TEST_UNUSED __attribute__((unused))
#else
#define ADF_TEST_UNUSED
#endif

/* Registration list: one node per ADF_TEST. */

typedef struct adf_test
{
    const char *name;
    const char *file;
    int line;
    void (*fn)(void);
    struct adf_test *next;
} adf_test;

/* State of the run. Zero-initialised, so it is ready before the constructors run. */

static adf_test *adf_test_head ADF_TEST_UNUSED = NULL;
static adf_test *adf_test_tail ADF_TEST_UNUSED = NULL;
static unsigned long adf_test_count ADF_TEST_UNUSED = 0;
static unsigned long adf_test_checks ADF_TEST_UNUSED = 0;
static unsigned long adf_test_failures ADF_TEST_UNUSED = 0;
static unsigned long adf_test_failed_tests ADF_TEST_UNUSED = 0;
static const char *adf_test_current ADF_TEST_UNUSED = "(none)";
static unsigned long adf_test_failures_before ADF_TEST_UNUSED = 0;

/* Append to the list. Called once per test, from a constructor. */

static void ADF_TEST_UNUSED
adf_test_register(adf_test *t)
{
    t->next = NULL;
    if (adf_test_tail == NULL)
        adf_test_head = t;
    else
        adf_test_tail->next = t;
    adf_test_tail = t;
    adf_test_count++;
}

/* The common head of every failure line. Counts the check and prints where it is. */

static void ADF_TEST_UNUSED
adf_test_fail_head(const char *file, int line, const char *cond)
{
    adf_test_failures++;
    printf("FAIL %s:%d: %s: check failed: %s", file, line, adf_test_current, cond);
}

/* A failed check, without a message. */

static void ADF_TEST_UNUSED
adf_test_fail(const char *file, int line, const char *cond)
{
    adf_test_fail_head(file, line, cond);
    printf("\n");
    fflush(stdout);
}

/* A failed check with a printf message. The compiler checks the format against the arguments. */

static void ADF_TEST_UNUSED
adf_test_fail_msg(const char *file, int line, const char *cond, const char *fmt, ...)
    __attribute__((format(printf, 4, 5)));

static void
adf_test_fail_msg(const char *file, int line, const char *cond, const char *fmt, ...)
{
    va_list ap;

    adf_test_fail_head(file, line, cond);
    printf(" | ");
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    printf("\n");
    fflush(stdout);
}

#define ADF_CHECK(cond)                                                     \
    do                                                                      \
    {                                                                       \
        adf_test_checks++;                                                  \
        if (!(cond))                                                        \
            adf_test_fail(__FILE__, __LINE__, #cond);                       \
    }                                                                       \
    while (0)

/* A failed check with a message. The condition is what is named in the report. */

#define ADF_CHECK_MSG(cond, ...)                                            \
    do                                                                      \
    {                                                                       \
        adf_test_checks++;                                                  \
        if (!(cond))                                                        \
            adf_test_fail_msg(__FILE__, __LINE__, #cond, __VA_ARGS__);      \
    }                                                                       \
    while (0)

/* One test. The body follows the macro. */

#define ADF_TEST(name)                                                      \
    static void adf_test_fn_##name(void);                                   \
    static adf_test adf_test_node_##name =                                  \
        {#name, __FILE__, __LINE__, adf_test_fn_##name, NULL};              \
    static void adf_test_ctor_##name(void) __attribute__((constructor));    \
    static void                                                             \
    adf_test_ctor_##name(void)                                              \
    {                                                                       \
        adf_test_register(&adf_test_node_##name);                           \
    }                                                                       \
    static void adf_test_fn_##name(void)

#ifndef ADF_TEST_NO_MAIN

int
main(void)
{
    for (adf_test *t = adf_test_head; t != NULL; t = t->next)
    {
        adf_test_current = t->name;
        adf_test_failures_before = adf_test_failures;
        t->fn();
        if (adf_test_failures == adf_test_failures_before)
            printf("ok   %s\n", t->name);
        else
        {
            adf_test_failed_tests++;
            printf("FAIL %s (%s:%d, %lu failed checks)\n", t->name, t->file, t->line,
                   adf_test_failures - adf_test_failures_before);
        }
    }

    printf("%lu tests, %lu checks, %lu failed checks, %lu failed tests\n", adf_test_count,
           adf_test_checks, adf_test_failures, adf_test_failed_tests);
    fflush(stdout);
    return adf_test_failures == 0 ? 0 : 1;
}

#endif /* ADF_TEST_NO_MAIN */

#endif /* ADF_TEST_RUNNER_H */
