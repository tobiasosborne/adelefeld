/* tests/test_text_r3r9.c: the two findings of the text review that a unit test did not pin, R3
   and R9 (docs/reviews/m1/text/review.md; the closure judgement in
   docs/reviews/m1/text/closure.md, lanes/m1-closure-text/report.md).

   R3 (MAJOR, settled by decision M1-D6): the default reader is not closed under the printer. The
   reviewer's input is "(9.99e100000 ; 0)" at prec = 128 with the default limits and digits = 1; the
   status of the input is ADF_OK and the old printer returned the text "(1e100001 +/- 1.1e99998 ; 0)",
   which the default reader refuses with ADF_LIMIT (10). The old behaviour is restored in a scratch
   copy by making tx_arb_printable (src/text.c) return 1, which is what the printer did before
   decision M1-D6: it printed whatever the value was.

   With M1-D6 a printer of a value with a real or a complex part returns NULL with *len = 0 when a
   non-zero midpoint or radius has a binary exponent above ADF_PRINT_EXP_MAX in absolute value
   (include/adelefeld/text.h:32-38), and the text of a value that is printed is read back with
   limits that admit it (text.h:39-42, docs/conventions.md 9.6). At the default max_exp10 = 100000
   every printable value prints with a decimal exponent far below the limit, because
   |ARF_EXP| <= 100000 means a magnitude below 2^100001, which is below 10^30104. So at the default
   limits the rule of this file is exact: a value read at the default limits either prints nothing
   or prints text that the default reader reads again, and the re-read real ball contains the
   source. The first test below is that rule, on the reviewer's input and on a table around it; the
   old printer breaks it on the reviewer's input.

   R9 (MINOR, closed by the repair of the fuzz target): the old target took prec from the first
   byte of the text and digits from the last, so an accepted adele or cadele reached only prec in
   {11, 12, 15, 34, 42} and digits in {3, 10, 11, 12, 14} and never prec = 2 or digits = 1
   (tests/fuzz/fuzz_text.c:10-14). The repair gives the target two control bytes and reads every
   accepted text back at prec = 2 and digits = 1 as well. The evidence for that is the committed
   corpus, and the evidence is about the corpus only, as docs/reviews/m1/text/closure.md:35 says
   ("R9's fuzzer change has coverage evidence, not a red assertion"). The last test of this file
   turns that evidence into an assertion: it reads the corpus of tests/fuzz/corpus/text/ as the
   target reads it and requires that an accepted adele or cadele text is reached at prec = 2 and at
   digits = 1. With the old scheme restored in a scratch copy (prec and digits from the bytes of
   the text, and the corpus without the two control bytes) the reached sets are {11, 34, 42, 47, 50,
   51, 52, 53, 54, 55, 56, 57, 62, 93, 101, 104, 116, 119, 125} for prec and {3, 4, 6, 11, 12, 19, 20,
   21, 22, 24, 25, 26, 27, 28} for digits: neither 2 nor 1 is in them, and the test fails.

   The middle tests of the file close the same gap where the target cannot: the values prec = 2 and
   digits = 1 are run here on texts that are not the adele or cadele line of the corpus, namely on
   the bare adf_rat and adf_fball value forms and on texts with leading and trailing whitespace,
   and the contract of conventions 9.6 is asserted for each of them.

   No file of the tree other than this one is read except the corpus, and no file of the tree is
   written. */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>

#include <adelefeld.h>

#include "test_runner.h"

/* The adele and cadele fields are initialised field by field, as tests/test_text_limits.c:44-68
   does; the printers need nothing else. */
static void
adele_init(adf_adele_t x)
{
    arb_init(x->inf);
    adf_fball_init(&x->fin);
}

static void
adele_clear(adf_adele_t x)
{
    arb_clear(x->inf);
    adf_fball_clear(&x->fin);
}

static void
cadele_init(adf_cadele_t z)
{
    acb_init(z->inf);
    adf_fball_init(&z->fin);
}

static void
cadele_clear(adf_cadele_t z)
{
    acb_clear(z->inf);
    adf_fball_clear(&z->fin);
}

/* ============================ R3: the printer and the reader ============================ */

/* The rule at the default limits: the printer of a value that the default reader admitted returns
   NULL with *len = 0, or text that the default reader reads again into a value that contains the
   source and has the same finite part. */
static void
check_print_read_back_adele(const char *text, slong prec, slong digits)
{
    adf_adele_t x, y;
    adf_text_limits_t lim;
    size_t len = (size_t) 12345;
    char *t;
    int st;

    adele_init(x);
    adele_init(y);
    adf_text_limits_default(&lim);

    st = adf_adele_set_str(x, text, strlen(text), prec, &lim);
    ADF_CHECK_MSG(st == ADF_OK, "the input \"%s\" at prec = %ld gave the status %d, not OK", text,
                  (long) prec, st);
    t = adf_adele_get_str(&len, x, digits);
    if (t == NULL)
    {
        /* M1-D6: no text means *len = 0 (text.h:32-38). */
        ADF_CHECK_MSG(len == 0, "\"%s\": no text but *len = %lu, not 0", text, (unsigned long) len);
    }
    else
    {
        st = adf_adele_set_str(y, t, len, prec, &lim);
        ADF_CHECK_MSG(st == ADF_OK, "\"%s\" printed as \"%s\" and the default reader gave the status "
                                   "%d, not OK", text, t, st);
        if (st == ADF_OK)
        {
            ADF_CHECK_MSG(arb_contains(y->inf, x->inf), "\"%s\" printed as \"%s\": the re-read real "
                                                       "ball does not contain the source", text, t);
            ADF_CHECK_MSG(adf_fball_identical(&x->fin, &y->fin),
                          "\"%s\" printed as \"%s\": the finite parts differ", text, t);
        }
        adf_str_free(t);
    }
    adele_clear(x);
    adele_clear(y);
}

ADF_TEST(the_reviewers_input_is_not_printed_at_the_default_limits)
{
    /* The input of R3, verbatim. The status is ADF_OK (the review says so), so the value exists
       and the printer is asked about it; with M1-D6 the answer is no text. */
    static const char * const texts[2] = { "(9.99e100000 ; 0)", "(9.99e-100000 ; 0)" };
    size_t i;

    for (i = 0; i < 2; i++)
    {
        adf_adele_t x;
        size_t len = (size_t) 12345;
        char *t;
        int st;

        adele_init(x);
        st = adf_adele_set_str(x, texts[i], strlen(texts[i]), 128, NULL);
        ADF_CHECK_MSG(st == ADF_OK, "\"%s\": the status of the input is %d, not OK", texts[i], st);
        t = adf_adele_get_str(&len, x, 1);
        ADF_CHECK_MSG(t == NULL, "\"%s\" printed \"%s\"; the decimal exponent of that text is above "
                                 "the default max_exp10 = 100000, so the default reader refuses it "
                                 "and there must be no text (decision M1-D6)", texts[i],
                      t == NULL ? "" : t);
        ADF_CHECK_MSG(len == 0, "\"%s\": no text but *len = %lu", texts[i], (unsigned long) len);
        adf_str_free(t);
        adele_clear(x);
    }
}

ADF_TEST(every_value_at_the_default_limits_prints_text_that_reads_back)
{
    /* The rule of text.h:39-42 at the default limits, on the reviewer's input and on values
       around it: the ones the old printer could not print, and the ones it could. The old printer
       fails this test on the first row. */
    static const char * const texts[7] = {
        "(9.99e100000 ; 0)",       /* the input of R3 */
        "(9.99e-100000 ; 0)",      /* the same value with a negative exponent */
        "(1e30100 ; 0)",           /* a large value that is still inside the print bound */
        "(-0.5 +/- 0.25 ; 0)",     /* a ball with a radius */
        "(0.1 ; 1/3 mod 1)",       /* a finite part with a context */
        "(1 ; 0)",                 /* a point */
        "(0 ; 0)",                 /* the zero, whose midpoint and radius are zero */
    };
    size_t i;

    for (i = 0; i < sizeof(texts) / sizeof(texts[0]); i++)
        check_print_read_back_adele(texts[i], 128, 1);
    /* The same rule at another digits value and another prec, where the reader is the one that
       rounds. */
    for (i = 3; i < 7; i++)
    {
        check_print_read_back_adele(texts[i], 2, 1);
        check_print_read_back_adele(texts[i], 64, 20);
    }
}

ADF_TEST(a_cadele_at_the_exponent_boundary_is_refused_like_an_adele)
{
    /* The review gave the adele case only. The complex part has the same rule (text.h:32-38), and
       it is the real or the imaginary part that decides. */
    static const char * const texts[3] = {
        "((9.99e100000) + (0)*i ; 0)",       /* the real part is over the bound */
        "((0) + (9.99e100000)*i ; 0)",       /* the imaginary part is over the bound */
        "((9.99e100000) + (9.99e100000)*i ; 0)",
    };
    size_t i;

    for (i = 0; i < 3; i++)
    {
        adf_cadele_t z;
        size_t len = (size_t) 12345;
        char *t;
        int st;

        cadele_init(z);
        st = adf_cadele_set_str(z, texts[i], strlen(texts[i]), 128, NULL);
        ADF_CHECK_MSG(st == ADF_OK, "\"%s\": the status of the input is %d, not OK", texts[i], st);
        t = adf_cadele_get_str(&len, z, 1);
        ADF_CHECK_MSG(t == NULL, "\"%s\" printed \"%s\", not NULL", texts[i], t == NULL ? "" : t);
        ADF_CHECK_MSG(len == 0, "\"%s\": no text but *len = %lu", texts[i], (unsigned long) len);
        adf_str_free(t);
        cadele_clear(z);
    }
}

ADF_TEST(a_printed_text_is_read_back_with_limits_that_admit_it)
{
    /* The qualification itself, at a small max_exp10 where the binary bound of M1-D6 is not
       reached: a value read at max_exp10 = 21 prints with a decimal exponent 22, so the reader at
       21 refuses the text and the reader at 22 takes it and encloses the source
       (docs/conventions.md 9.6, text.h:39-42; the case of docs/reviews/m1/text/closure.md:46-50).
       This row is green on the old printer as well; it states the rule, it does not pin the line
       of the repair. */
    static const char * const text = "(9.99e21 ; 0)";
    adf_adele_t x, y;
    adf_text_limits_t lim;
    size_t len = (size_t) 12345;
    char *t;
    int st;

    adele_init(x);
    adele_init(y);
    adf_text_limits_default(&lim);
    lim.max_exp10 = 21;
    st = adf_adele_set_str(x, text, strlen(text), 128, &lim);
    ADF_CHECK_MSG(st == ADF_OK, "the status of the input at max_exp10 = 21 is %d, not OK", st);
    t = adf_adele_get_str(&len, x, 1);
    ADF_CHECK_MSG(t != NULL, "a value inside the print bound was not printed");
    if (t != NULL)
    {
        ADF_CHECK_MSG(adf_adele_set_str(y, t, len, 128, &lim) == ADF_LIMIT,
                      "the reader at max_exp10 = 21 took the text \"%s\"; the decimal exponent of "
                      "that text is 22", t);
        lim.max_exp10 = 22;
        st = adf_adele_set_str(y, t, len, 128, &lim);
        ADF_CHECK_MSG(st == ADF_OK, "the reader at max_exp10 = 22 gave the status %d for \"%s\"",
                      st, t);
        if (st == ADF_OK)
            ADF_CHECK_MSG(arb_contains(y->inf, x->inf), "\"%s\": the re-read ball does not contain "
                                                       "the source", t);
        adf_str_free(t);
    }
    adele_clear(x);
    adele_clear(y);
}

/* ============================ R9: prec = 2 and digits = 1 ============================ */

ADF_TEST(prec_two_and_digits_one_on_the_value_forms_with_a_real_part)
{
    /* The two values the old scheme could not reach, on the adele and cadele value forms. The
       contract is conventions 9.6: the re-read real ball contains the source, the finite part is
       identical, and the status of the re-read is ADF_OK. */
    static const char * const ade[4] = {
        "(0.1 ; 0)", "(0.1 ; 1/3 mod 1)", " (0.1 ; 0) ", "(0.1 ; 0)\n",
    };
    static const char * const cad[2] = { "((0.1) + (0.2)*i ; 0)", " ((0.1) + (0.2)*i ; 0) " };
    size_t i;

    for (i = 0; i < 4; i++)
    {
        adf_adele_t x, y;
        size_t len = (size_t) 12345;
        char *t;
        int st;

        adele_init(x);
        adele_init(y);
        st = adf_adele_set_str(x, ade[i], strlen(ade[i]), 2, NULL);
        ADF_CHECK_MSG(st == ADF_OK, "\"%s\" at prec = 2 gave the status %d, not OK", ade[i], st);
        t = adf_adele_get_str(&len, x, 1);
        ADF_CHECK_MSG(t != NULL, "\"%s\": digits = 1 printed no text", ade[i]);
        if (t != NULL)
        {
            st = adf_adele_set_str(y, t, len, 2, NULL);
            ADF_CHECK_MSG(st == ADF_OK, "\"%s\" printed as \"%s\"; the re-read at prec = 2 gave the "
                                       "status %d, not OK", ade[i], t, st);
            if (st == ADF_OK)
            {
                ADF_CHECK_MSG(arb_contains(y->inf, x->inf), "\"%s\" printed as \"%s\": the re-read "
                                                           "ball does not contain the source", ade[i],
                              t);
                ADF_CHECK_MSG(adf_fball_identical(&x->fin, &y->fin), "\"%s\" printed as \"%s\": the "
                                                                     "finite parts differ", ade[i],
                              t);
            }
            adf_str_free(t);
        }
        adele_clear(x);
        adele_clear(y);
    }

    for (i = 0; i < 2; i++)
    {
        adf_cadele_t z, w;
        size_t len = (size_t) 12345;
        char *t;
        int st;

        cadele_init(z);
        cadele_init(w);
        st = adf_cadele_set_str(z, cad[i], strlen(cad[i]), 2, NULL);
        ADF_CHECK_MSG(st == ADF_OK, "\"%s\" at prec = 2 gave the status %d, not OK", cad[i], st);
        t = adf_cadele_get_str(&len, z, 1);
        ADF_CHECK_MSG(t != NULL, "\"%s\": digits = 1 printed no text", cad[i]);
        if (t != NULL)
        {
            st = adf_cadele_set_str(w, t, len, 2, NULL);
            ADF_CHECK_MSG(st == ADF_OK, "\"%s\" printed as \"%s\"; the re-read at prec = 2 gave the "
                                       "status %d, not OK", cad[i], t, st);
            if (st == ADF_OK)
            {
                ADF_CHECK_MSG(acb_contains(w->inf, z->inf), "\"%s\" printed as \"%s\": the re-read "
                                                           "ball does not contain the source", cad[i],
                              t);
                ADF_CHECK_MSG(adf_fball_identical(&z->fin, &w->fin), "\"%s\" printed as \"%s\": the "
                                                                     "finite parts differ", cad[i],
                              t);
            }
            adf_str_free(t);
        }
        cadele_clear(z);
        cadele_clear(w);
    }
}

ADF_TEST(prec_two_reaches_a_wider_ball_than_a_larger_prec)
{
    /* The value of prec is that the reader rounds to prec bits (text.h:157-163). A test that only
       round-trips one prec cannot see whether prec is used at all, so the two are compared on the
       same text. The midpoint of 0.1 is not dyadic, so the two balls differ. */
    static const char * const text = "(0.1 ; 0)";
    adf_adele_t lo, hi;

    adele_init(lo);
    adele_init(hi);
    ADF_CHECK(adf_adele_set_str(lo, text, strlen(text), 2, NULL) == ADF_OK);
    ADF_CHECK(adf_adele_set_str(hi, text, strlen(text), 64, NULL) == ADF_OK);
    ADF_CHECK_MSG(mag_cmp(arb_radref(lo->inf), arb_radref(hi->inf)) > 0,
                  "the radius at prec = 2 (%s) is not larger than the one at prec = 64 (%s)",
                  mag_dump_str(arb_radref(lo->inf)), mag_dump_str(arb_radref(hi->inf)));
    /* Both contain the decimal 0.1, which the reader promises: the ball at prec = 2 is the
       coarser one and still contains it. The decimal is read as an exact rational with fmpq, and
       the two are compared with arb_contains_fmpq. */
    {
        fmpq_t d;
        fmpq_init(d);
        fmpq_set_str(d, "1/10", 10);
        ADF_CHECK_MSG(arb_contains_fmpq(lo->inf, d), "the ball at prec = 2 does not contain 1/10");
        ADF_CHECK_MSG(arb_contains_fmpq(hi->inf, d), "the ball at prec = 64 does not contain 1/10");
        fmpq_clear(d);
    }
    adele_clear(lo);
    adele_clear(hi);
}

ADF_TEST(the_bare_rat_and_fball_value_forms_round_trip)
{
    /* The value forms of adf_rat and adf_fball have no prec and no digits (text.h:132-147), so the
       values of R9 do not apply to them; they are here because the corpus of the fuzz target is made
       of adele and cadele lines, and these two forms are then never the subject of a round trip in
       a test. conventions 9.6: the re-read value is identical to the first one. */
    static const char * const rats[3] = { "12/35", "0", "1000000/3" };
    static const char * const fballs[3] = { "2 mod 6", "1 mod 6/2", "0 mod 6" };
    size_t i;

    for (i = 0; i < 3; i++)
    {
        adf_rat_t r, s;
        size_t len = (size_t) 12345;
        char *t;
        int st;

        adf_rat_init(r);
        adf_rat_init(s);
        st = adf_rat_set_str(r, rats[i], strlen(rats[i]), NULL);
        ADF_CHECK_MSG(st == ADF_OK, "\"%s\" gave the status %d, not OK", rats[i], st);
        t = adf_rat_get_str(&len, r);
        ADF_CHECK_MSG(t != NULL, "\"%s\": adf_rat_get_str returned NULL, which it never does "
                                 "(text.h:36-38)", rats[i]);
        if (t != NULL)
        {
            ADF_CHECK_MSG(adf_rat_set_str(s, t, len, NULL) == ADF_OK, "\"%s\" printed as \"%s\" and "
                                                                   "was not read back", rats[i], t);
            ADF_CHECK_MSG(adf_rat_identical(r, s), "\"%s\" printed as \"%s\" and read back as a "
                                                    "different value", rats[i], t);
            adf_str_free(t);
        }
        adf_rat_clear(r);
        adf_rat_clear(s);
    }

    for (i = 0; i < 3; i++)
    {
        adf_fball_t f, g;
        size_t len = (size_t) 12345;
        char *t;
        int st;

        adf_fball_init(f);
        adf_fball_init(g);
        st = adf_fball_set_str(f, fballs[i], strlen(fballs[i]), NULL);
        ADF_CHECK_MSG(st == ADF_OK, "\"%s\" gave the status %d, not OK", fballs[i], st);
        t = adf_fball_get_str(&len, f);
        ADF_CHECK_MSG(t != NULL, "\"%s\": adf_fball_get_str returned NULL, which it never does "
                                 "(text.h:36-38)", fballs[i]);
        if (t != NULL)
        {
            ADF_CHECK_MSG(adf_fball_set_str(g, t, len, NULL) == ADF_OK, "\"%s\" printed as \"%s\" "
                                                                     "and was not read back",
                          fballs[i], t);
            ADF_CHECK_MSG(adf_fball_identical(f, g), "\"%s\" printed as \"%s\" and read back as a "
                                                      "different value", fballs[i], t);
            adf_str_free(t);
        }
        adf_fball_clear(f);
        adf_fball_clear(g);
    }
}

ADF_TEST(the_committed_corpus_reaches_prec_two_and_digits_one)
{
    /* The coverage evidence of R9 as an assertion. The rule is the one of the fuzz target
       (tests/fuzz/fuzz_text.c:287-288): byte 0 chooses prec = 2 + byte mod 190, byte 1 chooses
       digits = 1 + byte mod 30, and the rest of the file is the text. The limits are those of the
       target: the defaults of conventions 8.4 with max_exp10 = 5000. A file whose text is accepted
       as an adele or a cadele is a case of the round trip at that prec and that digits; the sets
       of prec and of digits reached over the whole corpus must contain 2 and 1, the two values the
       old scheme could not reach. */
    static const char * const dir = "tests/fuzz/corpus/text";
    int precs[192];
    int digs[31];
    int old_precs[192];
    int old_digs[31];
    int precs_ade[192];
    int precs_cad[192];
    long files = 0, accepted = 0, old_accepted = 0;
    DIR *d;
    struct dirent *e;

    memset(precs, 0, sizeof(precs));
    memset(digs, 0, sizeof(digs));
    memset(old_precs, 0, sizeof(old_precs));
    memset(precs_ade, 0, sizeof(precs_ade));
    memset(precs_cad, 0, sizeof(precs_cad));
    memset(old_digs, 0, sizeof(old_digs));
    d = opendir(dir);
    ADF_CHECK_MSG(d != NULL, "the corpus directory %s cannot be read; the test runs from the "
                             "repository root", dir);
    if (d == NULL)
        return;

    while ((e = readdir(d)) != NULL)
    {
        char path[1024];
        unsigned char buf[8192];
        adf_text_limits_t lim;
        adf_text_kind kind;
        slong prec, digits;
        size_t n;
        int cst;
        FILE *f;

        if (e->d_name[0] == '.')
            continue;
        if (snprintf(path, sizeof(path), "%s/%s", dir, e->d_name) >= (int) sizeof(path))
            continue;
        f = fopen(path, "rb");
        if (f == NULL)
            continue;
        n = fread(buf, 1, sizeof(buf), f);
        fclose(f);
        files++;
        if (n < 3)
            continue;
        prec = 2 + (slong) (buf[0] % 190);
        digits = 1 + (slong) (buf[1] % 30);
        adf_text_limits_default(&lim);
        lim.max_exp10 = 5000;
        cst = adf_text_classify(&kind, (const char *) buf + 2, n - 2, &lim);
        if (cst == ADF_OK && (kind == ADF_TEXT_ADELE || kind == ADF_TEXT_CADELE))
        {
            accepted++;
            precs[prec] = 1;
            digs[digits] = 1;
            if (kind == ADF_TEXT_ADELE)
                precs_ade[prec] = 1;
            else
                precs_cad[prec] = 1;
        }
        /* The same corpus as the old target of commit 1b2331e saw it: the text without the two
           control bytes, prec from its first byte and digits from its last
           (docs/reviews/m1/text/review.md, R9). The two sets below are what that scheme reaches. */
        cst = adf_text_classify(&kind, (const char *) buf + 2, n - 2, &lim);
        if (cst == ADF_OK && (kind == ADF_TEXT_ADELE || kind == ADF_TEXT_CADELE))
        {
            slong op = 2 + (slong) (buf[2] % 190);
            slong od = 1 + (slong) (buf[n - 1] % 30);

            old_accepted++;
            old_precs[op] = 1;
            old_digs[od] = 1;
        }
    }
    closedir(d);

    ADF_CHECK_MSG(files > 100, "%ld files in %s, more than a hundred are expected", files, dir);
    ADF_CHECK_MSG(accepted > 10, "%ld accepted adele or cadele texts among %ld files", accepted,
                  files);
    /* digits = 1 is one of the two values of finding R9. With the old scheme (prec and digits from
       the bytes of the text) the only digits value reached by an accepted adele or cadele text of
       this corpus is 12, because every corpus text is a line that begins with '(' and ends with
       ')'. */
    ADF_CHECK_MSG(digs[1] == 1, "no accepted adele or cadele text of the corpus is reached at "
                                "digits = 1; that is the coverage gap of finding R9");
    /* What the old scheme reached on the same texts, measured here: neither prec = 2 nor
       digits = 1, which is the finding of R9 (docs/reviews/m1/text/review.md:149-159). Every
       corpus text of this corpus is a line, so its first byte is '(' and its last is ')'. */
    {
        slong i;
        long op = 0, od = 0;

        for (i = 2; i <= 191; i++)
            op += old_precs[i];
        for (i = 1; i <= 30; i++)
            od += old_digs[i];
        ADF_CHECK_MSG(old_accepted > 10, "the old reading of the corpus accepts %ld adele or cadele "
                                        "texts, not more than ten", old_accepted);
        ADF_CHECK_MSG(old_precs[2] == 0, "the old scheme of finding R9 reaches prec = 2 on this "
                                         "corpus, so the finding no longer describes it");
        ADF_CHECK_MSG(old_digs[1] == 0, "the old scheme of finding R9 reaches digits = 1 on this "
                                        "corpus, so the finding no longer describes it");
        printf("   the old scheme on the same texts: %ld accepted, %ld prec values, %ld digits "
               "values\n", old_accepted, op, od);
    }
    /* The rule of the target is a bijection onto its ranges, so every prec and every digits is
       reachable from some byte pair; the corpus must reach a large part of both ranges. The old
       scheme reaches the single prec 42 and the single digits 12 on this corpus. */
    {
        long p = 0, g = 0, at_prec_2 = 0;
        slong i;

        for (i = 2; i <= 191; i++)
            p += precs[i];
        for (i = 1; i <= 30; i++)
            g += digs[i];
        ADF_CHECK_MSG(p > 50, "only %ld of the 190 prec values of the range 2..191 are reached by "
                              "the accepted adele and cadele texts of the corpus", p);
        ADF_CHECK_MSG(g > 20, "only %ld of the 30 digits values are reached by the accepted adele "
                              "and cadele texts of the corpus", g);
        /* prec = 2 is the other value of R9 (adf-zbl). The corpus holds an accepted adele text and
           an accepted cadele text with the control byte 0 (prec = 2), and the same two with the
           control byte 189 (prec = 191, the largest of the range); the round trip at those values
           is exercised by the fuzz target on every run. The value of prec is used by the adele
           and cadele round trips only, so the two kinds are asserted apart. */
        ADF_CHECK_MSG(precs_ade[2] == 1, "no accepted adele text of the corpus is reached at "
                                         "prec = 2 (finding R9, adf-zbl)");
        ADF_CHECK_MSG(precs_cad[2] == 1, "no accepted cadele text of the corpus is reached at "
                                         "prec = 2 (finding R9, adf-zbl)");
        ADF_CHECK_MSG(precs_ade[191] == 1, "no accepted adele text of the corpus is reached at "
                                           "prec = 191, the largest prec of the range");
        ADF_CHECK_MSG(precs_cad[191] == 1, "no accepted cadele text of the corpus is reached at "
                                           "prec = 191, the largest prec of the range");
        at_prec_2 = precs[2];
        printf("   the corpus of the text target: %ld files, %ld accepted adele or cadele texts, "
               "%ld of 190 prec values, %ld of 30 digits values, prec = 2 reached: %ld\n", files,
               accepted, p, g, at_prec_2);
    }
}
