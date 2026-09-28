/* tests/test_invariants_lifetime.c: the borrow count of the debug build -DADF_CHECK_INVARIANTS
   (docs/conventions.md 4.6, lifetime contract, lines 323 to 328: "With -DADF_CHECK_INVARIANTS a
   context counts the values that refer to it (atomically) and adf_modctx_free aborts if the count
   is not zero"; include/adelefeld/modctx.h:16-18; finding R7 of docs/reviews/m1/text/review.md,
   reproducer checks/probe.c, mode "lifetime").

   Every scenario runs in a child process (fork). It builds one or two contexts A and B and some
   values, then frees the context that the scenario names, first. What is required of that free
   is stated per scenario: it must abort (SIGABRT, a line on stderr that names adf_modctx_free), or
   it must return, and then the child clears every value and frees the other context, which must
   return too.

   Without -DADF_CHECK_INVARIANTS the program says that it was skipped and counts as passed.
   The test runs from the repository root. */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <adelefeld.h>

#include "test_runner.h"

#ifndef ADF_CHECK_INVARIANTS

ADF_TEST(skipped_without_the_flag)
{
    printf("test_invariants_lifetime: skipped: built without -DADF_CHECK_INVARIANTS\n");
    ADF_CHECK(1);
}

#else

#include <pthread.h>
#include <signal.h>
#include <sys/resource.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif
#include <sys/wait.h>
#include <unistd.h>

typedef struct
{
    int exited, code, signal;
    char err[2048];
} outcome;

typedef void (*scenario)(int which);

static void
run_child(scenario f, int which, outcome * o)
{
    int fd[2];
    pid_t pid;
    int st = 0;
    size_t n = 0;

    memset(o, 0, sizeof *o);
    fflush(stdout);
    if (pipe(fd) != 0)
        abort();
    pid = fork();
    if (pid < 0)
        abort();
    if (pid == 0)
    {
        struct rlimit nocore = {0, 0};

        /* An abort must not write a core file: with systemd-coredump each one costs 0.15 s, and
           RLIMIT_CORE does not stop it. A process that is not dumpable is not dumped. */
        setrlimit(RLIMIT_CORE, &nocore);
#ifdef __linux__
        prctl(PR_SET_DUMPABLE, 0);
#endif
        close(fd[0]);
        dup2(fd[1], 2);
        close(fd[1]);
        f(which);
        _exit(0);
    }
    close(fd[1]);
    for (;;)
    {
        ssize_t r = read(fd[0], o->err + n, sizeof o->err - 1 - n);
        if (r <= 0)
            break;
        n += (size_t) r;
        if (n >= sizeof o->err - 1)
            break;
    }
    o->err[n] = 0;
    close(fd[0]);
    if (waitpid(pid, &st, 0) != pid)
        abort();
    o->exited = WIFEXITED(st) ? 1 : 0;
    o->code = o->exited ? WEXITSTATUS(st) : -1;
    o->signal = WIFSIGNALED(st) ? WTERMSIG(st) : 0;
}

/* expect_abort = 1: the free of the context `which` aborts; 0: it returns and the child ends 0.
   A child that ends by _exit with another code found a wrong intermediate state (the code says
   which check of the scenario failed). */
static void
expect(const char * name, scenario f, int which, int expect_abort)
{
    outcome o;

    run_child(f, which, &o);
    if (expect_abort)
    {
        ADF_CHECK_MSG(o.signal == SIGABRT, "%s, free of context %d: no abort (exited %d code %d signal %d)",
                      name, which, o.exited, o.code, o.signal);
        ADF_CHECK_MSG(strstr(o.err, "adf_modctx_free") != NULL,
                      "%s, free of context %d: stderr does not name adf_modctx_free: [%s]", name, which, o.err);
        ADF_CHECK_MSG(o.err[0] != 0 && strchr(o.err, '\n') == o.err + strlen(o.err) - 1,
                      "%s, free of context %d: stderr is not one line: [%s]", name, which, o.err);
    }
    else
    {
        ADF_CHECK_MSG(o.exited && o.code == 0, "%s, free of context %d: not a normal return (exited %d code %d "
                      "signal %d) [%s]", name, which, o.exited, o.code, o.signal, o.err);
    }
}

/* ------------------------------------------------------------------ helpers of the scenarios */

static adf_modctx_struct *
new_ctx(ulong q)
{
    adf_modctx_struct * c = NULL;

    if (adf_modctx_new_blocks(&c, &q, 1) != ADF_OK)
        _exit(90);
    return c;
}

/* a local ball of ctx (one block q): the set 1 mod q */
static void
make_local(adf_fball_t x, const adf_modctx_struct * ctx)
{
    fmpz_t A, d;
    fmpz_t K;

    fmpz_init_set_si(A, 1);
    fmpz_init(K);
    fmpz_init_set_si(d, 1);
    adf_modctx_get_modulus(K, ctx);
    if (adf_fball_set_fmpz3(x, A, K, d) != ADF_OK || adf_fball_set_local(x, x, ctx) != ADF_OK)
        _exit(91);
    fmpz_clear(A);
    fmpz_clear(K);
    fmpz_clear(d);
}

static void
make_global(adf_fball_t x)
{
    adf_fball_set_si(x, 3);
}

/* the scenario body of most cases: free ctx[which] first. */
static adf_modctx_struct * ctxs[2];

static void
free_first(int which)
{
    adf_modctx_free(ctxs[which]);
}

static void
free_rest(int which)
{
    adf_modctx_free(ctxs[1 - which]);
}

/* ------------------------------------------------------------------ one borrower of each kind */

static void
scn_fball_local(int which)
{
    adf_fball_t x;

    ctxs[0] = new_ctx(6);
    ctxs[1] = new_ctx(5);
    adf_fball_init(x);
    make_local(x, ctxs[0]);
    (void) which;
    free_first(which);
    adf_fball_clear(x);
    free_rest(which);
}

static void
scn_scaled(int which)
{
    adf_scaled_t x;

    ctxs[0] = new_ctx(6);
    ctxs[1] = new_ctx(5);
    adf_scaled_init(x, ctxs[0]);
    free_first(which);
    adf_scaled_clear(x);
    free_rest(which);
}

static void
scn_adele_fin(int which)
{
    adf_adele_t x;

    ctxs[0] = new_ctx(6);
    ctxs[1] = new_ctx(5);
    adf_adele_init(x);
    make_local(&x->fin, ctxs[0]);
    free_first(which);
    adf_adele_clear(x);
    free_rest(which);
}

static void
scn_cadele_fin(int which)
{
    adf_cadele_t x;

    ctxs[0] = new_ctx(6);
    ctxs[1] = new_ctx(5);
    adf_cadele_init(x);
    make_local(&x->fin, ctxs[0]);
    free_first(which);
    adf_cadele_clear(x);
    free_rest(which);
}

/* a global ball, or an adele with a global finite part, does not borrow */
static void
scn_globals_do_not_borrow(int which)
{
    adf_fball_t x;
    adf_adele_t a;
    adf_cadele_t c;

    ctxs[0] = new_ctx(6);
    ctxs[1] = new_ctx(5);
    adf_fball_init(x);
    adf_adele_init(a);
    adf_cadele_init(c);
    make_global(x);
    free_first(which);
    free_rest(which);
    adf_fball_clear(x);
    adf_adele_clear(a);
    adf_cadele_clear(c);
}

/* the reproducer of finding R7: context (2), a scaled borrower, free */
static void
scn_r7(int which)
{
    adf_scaled_t x;

    (void) which;
    ctxs[0] = new_ctx(2);
    ctxs[1] = new_ctx(3);
    adf_scaled_init(x, ctxs[0]);
    adf_modctx_free(ctxs[0]);
    adf_scaled_clear(x);
}

/* two borrowers of one kind each: the count is a count, not a flag */
static void
scn_two_borrowers(int which)
{
    adf_scaled_t x, y;

    ctxs[0] = new_ctx(6);
    ctxs[1] = new_ctx(5);
    adf_scaled_init(x, ctxs[0]);
    adf_scaled_init(y, ctxs[0]);
    adf_scaled_clear(x);
    free_first(which);          /* y still borrows context 0 */
    adf_scaled_clear(y);
    free_rest(which);
}

static void
scn_two_borrowers_then_both_cleared(int which)
{
    adf_scaled_t x, y;
    adf_fball_t f;

    ctxs[0] = new_ctx(6);
    ctxs[1] = new_ctx(5);
    adf_scaled_init(x, ctxs[0]);
    adf_scaled_init(y, ctxs[0]);
    adf_fball_init(f);
    make_local(f, ctxs[0]);
    adf_scaled_clear(x);
    adf_scaled_clear(y);
    adf_fball_clear(f);
    free_first(which);
    free_rest(which);
}

/* ------------------------------------------------------------------ set, swap, moves */

/* y = copy of x (adf_fball_set); x cleared: y still borrows */
static void
scn_fball_set(int which)
{
    adf_fball_t x, y;

    ctxs[0] = new_ctx(6);
    ctxs[1] = new_ctx(5);
    adf_fball_init(x);
    adf_fball_init(y);
    make_local(x, ctxs[0]);
    adf_fball_set(y, x);
    adf_fball_clear(x);
    free_first(which);          /* which = 0: y borrows -> abort */
    adf_fball_clear(y);
    free_rest(which);
}

/* x in A, w in B; adf_fball_set(x, w): x is now a borrower of B and no longer of A */
static void
scn_fball_overwritten_by_other_context(int which)
{
    adf_fball_t x, w;

    ctxs[0] = new_ctx(6);
    ctxs[1] = new_ctx(5);
    adf_fball_init(x);
    adf_fball_init(w);
    make_local(x, ctxs[0]);
    make_local(w, ctxs[1]);
    adf_fball_set(x, w);
    adf_fball_clear(w);
    free_first(which);          /* 0: returns (x left A); 1: aborts (x borrows B) */
    adf_fball_clear(x);
    free_rest(which);
}

/* x local in A; x overwritten by a global value: A is free */
static void
scn_fball_overwritten_by_global(int which)
{
    adf_fball_t x, g;

    ctxs[0] = new_ctx(6);
    ctxs[1] = new_ctx(5);
    adf_fball_init(x);
    adf_fball_init(g);
    make_local(x, ctxs[0]);
    make_global(g);
    adf_fball_set(x, g);
    free_first(which);
    adf_fball_clear(x);
    adf_fball_clear(g);
    free_rest(which);
}

/* swap of a local value of A with a local value of B: the counts do not change */
static void
scn_fball_swap(int which)
{
    adf_fball_t x, y;

    ctxs[0] = new_ctx(6);
    ctxs[1] = new_ctx(5);
    adf_fball_init(x);
    adf_fball_init(y);
    make_local(x, ctxs[0]);
    make_local(y, ctxs[1]);
    adf_fball_swap(x, y);
    adf_fball_clear(y);         /* y holds A's borrower now */
    free_first(which);          /* 0: returns; 1: aborts (x holds B) */
    adf_fball_clear(x);
    free_rest(which);
}

/* swap of a local value with a global one */
static void
scn_fball_swap_with_global(int which)
{
    adf_fball_t x, g;

    ctxs[0] = new_ctx(6);
    ctxs[1] = new_ctx(5);
    adf_fball_init(x);
    adf_fball_init(g);
    make_local(x, ctxs[0]);
    make_global(g);
    adf_fball_swap(x, g);
    /* g is the borrower of A now; x is global */
    free_first(which);          /* 0: aborts */
    adf_fball_clear(g);
    adf_fball_clear(x);
    free_rest(which);
}

/* change of backend: local -> global by adf_fball_set_global; neither context is borrowed then */
static void
scn_fball_backend_change(int which)
{
    adf_fball_t x;

    ctxs[0] = new_ctx(6);
    ctxs[1] = new_ctx(5);
    adf_fball_init(x);
    make_local(x, ctxs[0]);
    adf_fball_set_global(x, x);
    if (adf_fball_is_local(x))
        _exit(93);
    free_first(which);
    adf_fball_clear(x);
    free_rest(which);
}

/* change of backend and of context: local in A -> global -> local in B */
static void
scn_fball_global_to_local(int which)
{
    adf_fball_t x;

    ctxs[0] = new_ctx(6);
    ctxs[1] = new_ctx(5);
    adf_fball_init(x);
    make_local(x, ctxs[0]);
    adf_fball_set_global(x, x);
    make_local(x, ctxs[1]);             /* x borrows B and not A */
    if (!adf_fball_is_local(x))
        _exit(93);
    free_first(which);                  /* 0: returns; 1: aborts */
    adf_fball_clear(x);
    free_rest(which);
}

/* an arithmetic result overwrites a local value: local + global is global, so A is free after it */
static void
scn_fball_arithmetic_overwrite(int which)
{
    adf_fball_t x, g;

    ctxs[0] = new_ctx(6);
    ctxs[1] = new_ctx(5);
    adf_fball_init(x);
    adf_fball_init(g);
    make_local(x, ctxs[0]);
    make_global(g);
    adf_fball_add(x, x, g);             /* the result is global */
    if (adf_fball_is_local(x))
        _exit(94);
    free_first(which);
    adf_fball_clear(x);
    adf_fball_clear(g);
    free_rest(which);
}

/* aliased calls y = x with a local value staying local: one borrower, not two, not zero */
static void
scn_fball_aliased(int which)
{
    adf_fball_t x;

    ctxs[0] = new_ctx(6);
    ctxs[1] = new_ctx(5);
    adf_fball_init(x);
    make_local(x, ctxs[0]);
    adf_fball_neg(x, x);
    adf_fball_add(x, x, x);
    adf_fball_sub(x, x, x);
    adf_fball_set(x, x);
    if (!adf_fball_is_local(x))
        _exit(94);
    free_first(which);          /* 0: aborts if x is still local (it is: add and sub of a value with itself) */
    adf_fball_clear(x);
    free_rest(which);
}

/* aliased calls on a scaled value */
static void
scn_scaled_aliased(int which)
{
    adf_scaled_t x;

    ctxs[0] = new_ctx(6);
    ctxs[1] = new_ctx(5);
    adf_scaled_init(x, ctxs[0]);
    (void) adf_scaled_add(x, x, x);
    (void) adf_scaled_mul(x, x, x);
    (void) adf_scaled_mul_tight(x, x, x);
    adf_scaled_neg(x, x);
    adf_scaled_set(x, x);
    free_first(which);
    adf_scaled_clear(x);
    free_rest(which);
}

/* scaled: set, swap */
static void
scn_scaled_set_swap(int which)
{
    adf_scaled_t x, y, z;

    ctxs[0] = new_ctx(6);
    ctxs[1] = new_ctx(5);
    adf_scaled_init(x, ctxs[0]);
    adf_scaled_init(y, ctxs[1]);
    adf_scaled_init(z, ctxs[0]);
    adf_scaled_set(z, y);               /* z moves from A to B; x still borrows A */
    adf_scaled_swap(x, y);              /* x borrows B, y borrows A */
    adf_scaled_clear(y);                /* the last borrower of A is gone */
    free_first(which);                  /* 0: returns; 1: aborts (x and z borrow B) */
    adf_scaled_clear(x);
    adf_scaled_clear(z);
    free_rest(which);
}

/* scaled: set_context moves the value to another context */
static void
scn_scaled_set_context(int which)
{
    adf_scaled_t x;
    int lost = 0;

    ctxs[0] = new_ctx(6);
    ctxs[1] = new_ctx(12);              /* 6 divides 12: lossless */
    adf_scaled_init(x, ctxs[0]);
    if (adf_scaled_set_context(x, &lost, x, ctxs[1]) != ADF_OK)
        _exit(95);
    free_first(which);                  /* 0: returns; 1: aborts */
    adf_scaled_clear(x);
    free_rest(which);
}

/* scaled: conversions from a ball and from a rational, into another context each time */
static void
scn_scaled_conversions(int which)
{
    adf_scaled_t x;
    adf_fball_t f;
    adf_rat_t q;

    ctxs[0] = new_ctx(6);
    ctxs[1] = new_ctx(5);
    adf_scaled_init(x, ctxs[0]);
    adf_fball_init(f);
    adf_rat_init(q);
    adf_rat_set_si(q, 7);
    adf_scaled_set_rat(x, q, ctxs[1]);          /* x moves from A to B */
    free_first(which);                          /* 0: returns */
    adf_scaled_set_fball(x, NULL, f, ctxs[1]);  /* B again */
    adf_scaled_clear(x);
    adf_fball_clear(f);
    adf_rat_clear(q);
    free_rest(which);
}

/* adf_scaled_set_fball of an exact ball and of an inexact ball into another context */
static void
scn_scaled_set_fball_moves(int which)
{
    adf_scaled_t x, y;
    adf_fball_t f, g;
    fmpz_t A, H, d;

    ctxs[0] = new_ctx(6);
    ctxs[1] = new_ctx(5);
    adf_scaled_init(x, ctxs[0]);
    adf_scaled_init(y, ctxs[0]);
    adf_fball_init(f);                  /* exact 0 */
    adf_fball_init(g);
    fmpz_init_set_si(A, 1);
    fmpz_init_set_si(H, 3);
    fmpz_init_set_si(d, 1);
    if (adf_fball_set_fmpz3(g, A, H, d) != ADF_OK)
        _exit(95);
    (void) adf_scaled_set_fball(x, NULL, f, ctxs[1]);
    (void) adf_scaled_set_fball(y, NULL, g, ctxs[1]);
    free_first(which);                  /* 0: returns; 1: aborts */
    adf_scaled_clear(x);
    adf_scaled_clear(y);
    adf_fball_clear(f);
    adf_fball_clear(g);
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
    free_rest(which);
}

/* adele and cadele: set, swap, get_fin, arithmetic */
static void
scn_adele_moves(int which)
{
    adf_adele_t x, y, z;
    adf_fball_t f;

    ctxs[0] = new_ctx(6);
    ctxs[1] = new_ctx(5);
    adf_adele_init(x);
    adf_adele_init(y);
    adf_adele_init(z);
    adf_fball_init(f);
    make_local(&x->fin, ctxs[0]);
    make_local(&y->fin, ctxs[1]);
    adf_adele_swap(x, y);               /* x in B, y in A */
    adf_adele_set(z, y);                /* z in A */
    adf_adele_clear(y);
    adf_adele_get_fin(f, z);            /* f in A */
    adf_adele_neg(z, z);                /* z stays in A */
    adf_adele_clear(z);
    adf_fball_clear(f);                 /* A has no borrower left */
    free_first(which);                  /* 0: returns; 1: aborts (x in B) */
    adf_adele_clear(x);
    free_rest(which);
}

static void
scn_cadele_moves(int which)
{
    adf_cadele_t x, y, z;
    adf_adele_t a;

    ctxs[0] = new_ctx(6);
    ctxs[1] = new_ctx(5);
    adf_cadele_init(x);
    adf_cadele_init(y);
    adf_cadele_init(z);
    adf_adele_init(a);
    make_local(&x->fin, ctxs[0]);
    make_local(&y->fin, ctxs[1]);
    adf_cadele_swap(x, y);
    adf_cadele_set(z, y);               /* z in A */
    adf_cadele_clear(y);
    adf_cadele_neg(z, z);
    adf_cadele_clear(z);
    make_local(&a->fin, ctxs[1]);
    adf_cadele_set_adele(x, a);         /* x in B (again) */
    adf_adele_clear(a);
    free_first(which);                  /* 0: returns; 1: aborts (x in B) */
    adf_cadele_clear(x);
    free_rest(which);
}

/* a value read from a dump with a bound context borrows it */
static void
scn_load(int which)
{
    adf_fball_t x, y;
    char * s;
    size_t n = 0;
    const adf_modctx_struct * binds[1];
    int st;

    ctxs[0] = new_ctx(6);
    ctxs[1] = new_ctx(5);
    adf_fball_init(x);
    adf_fball_init(y);
    make_local(x, ctxs[0]);
    s = adf_fball_dump_str(&n, x);
    binds[0] = ctxs[0];
    st = adf_fball_load_str_binds(y, s, n, binds, 1, NULL);
    if (st != ADF_OK)
        _exit(96);
    adf_str_free(s);
    adf_fball_clear(x);
    free_first(which);                  /* 0: aborts (y) */
    adf_fball_clear(y);
    free_rest(which);
}

static void
scn_load_overwrites(int which)
{
    adf_fball_t x, y;
    char * s;
    size_t n = 0;
    int st;

    ctxs[0] = new_ctx(6);
    ctxs[1] = new_ctx(5);
    adf_fball_init(x);
    adf_fball_init(y);
    make_local(x, ctxs[0]);
    make_local(y, ctxs[1]);
    s = adf_fball_dump_str(&n, x);
    st = adf_fball_load_str(y, s, n, ctxs[0], NULL);        /* y leaves B and enters A */
    if (st != ADF_OK)
        _exit(96);
    adf_str_free(s);
    free_first(1);                      /* B: returns */
    adf_fball_clear(x);
    if (which == 0)
        free_first(0);                  /* A: y borrows it: aborts */
    adf_fball_clear(y);
    free_first(0);
}

static void
scn_load_scaled(int which)
{
    adf_scaled_t x, y;
    char * s;
    size_t n = 0;

    ctxs[0] = new_ctx(6);
    ctxs[1] = new_ctx(5);
    adf_scaled_init(x, ctxs[0]);
    adf_scaled_init(y, ctxs[1]);
    s = adf_scaled_dump_str(&n, x);
    if (adf_scaled_load_str(y, s, n, ctxs[0], NULL) != ADF_OK)
        _exit(97);
    adf_str_free(s);
    adf_scaled_clear(x);
    free_first(which);                  /* 0: aborts (y in A); 1: returns */
    adf_scaled_clear(y);
    free_rest(which);
}

/* the parsers write a global value over a local one */
static void
scn_set_str_overwrites(int which)
{
    adf_fball_t x;
    adf_adele_t a;
    adf_cadele_t c;

    ctxs[0] = new_ctx(6);
    ctxs[1] = new_ctx(5);
    adf_fball_init(x);
    adf_adele_init(a);
    adf_cadele_init(c);
    make_local(x, ctxs[0]);
    make_local(&a->fin, ctxs[1]);
    make_local(&c->fin, ctxs[1]);
    if (adf_fball_set_str(x, "(* ; 1/2)", 9, NULL) != ADF_OK ||
        adf_adele_set_str(a, "(1 ; 1/2)", 9, 64, NULL) != ADF_OK ||
        adf_cadele_set_str(c, "((1) + (2)*i ; 1/2)", 19, 64, NULL) != ADF_OK)
        _exit(98);
    free_first(which);                  /* both return */
    adf_fball_clear(x);
    adf_adele_clear(a);
    adf_cadele_clear(c);
    free_rest(which);
}

/* the constructors that are exempt from the entry check (they write an output and read no value)
   overwrite a local value with a global one: the borrow is given back. g_sel picks the function. */
static int g_sel;

static void
scn_constructors(int which)
{
    adf_fball_t x;
    adf_adele_t a;
    fmpz_t n, H, d;

    ctxs[0] = new_ctx(6);
    ctxs[1] = new_ctx(5);
    adf_fball_init(x);
    adf_adele_init(a);
    fmpz_init_set_si(n, 4);
    fmpz_init_set_si(H, 6);
    fmpz_init_set_si(d, 3);
    make_local(x, ctxs[0]);
    make_local(&a->fin, ctxs[0]);
    switch (g_sel)
    {
        case 0: adf_fball_zero(x); break;
        case 1: adf_fball_one(x); break;
        case 2: adf_fball_set_si(x, 5); break;
        case 3: adf_fball_set_fmpz(x, n); break;
        case 4: (void) adf_fball_set_fmpz3(x, n, H, d); break;
        case 5: adf_adele_set_si(a, 3); break;
        case 6: (void) adf_fball_canonicalise(x); break;      /* a local value: no change, still a borrower */
        default: _exit(89);
    }
    if (g_sel == 6)
        adf_fball_set_global(x, x);     /* now release it */
    adf_fball_clear(x);
    if (g_sel != 5)
        adf_adele_set_si(a, 1);         /* the adele's borrow goes too */
    free_first(which);                  /* the value in question no longer borrows context 0 */
    adf_adele_clear(a);
    fmpz_clear(n);
    fmpz_clear(H);
    fmpz_clear(d);
    free_rest(which);
}

/* ------------------------------------------------------------------ threads */

typedef struct
{
    const adf_modctx_struct * ctx;
    long n;
} targ;

static void *
thread_scaled(void * v)
{
    targ * t = (targ *) v;
    long i;

    for (i = 0; i < t->n; i++)
    {
        adf_scaled_t x;
        adf_scaled_init(x, t->ctx);
        adf_scaled_clear(x);
    }
    return NULL;
}

static void *
thread_fball(void * v)
{
    targ * t = (targ *) v;
    adf_fball_t x, y;
    long i;

    adf_fball_init(x);
    make_local(x, t->ctx);              /* one borrower that lives as long as the thread */
    for (i = 0; i < t->n; i++)
    {
        adf_fball_init(y);
        adf_fball_set(y, x);
        adf_fball_clear(y);
    }
    adf_fball_clear(x);
    return NULL;
}

/* which = 0: the threads end, then the free must return; which = 1: the main thread still holds
   one borrower, the free must abort (an exact count, not a flag and not lost updates). */
static void
scn_threads(int which)
{
    pthread_t th[4];
    targ a;
    adf_scaled_t held;

    ctxs[0] = new_ctx(6);
    ctxs[1] = new_ctx(5);
    a.ctx = ctxs[0];
    a.n = 100000;
    adf_scaled_init(held, ctxs[0]);
    if (pthread_create(&th[0], NULL, thread_scaled, &a) != 0 ||
        pthread_create(&th[1], NULL, thread_scaled, &a) != 0 ||
        pthread_create(&th[2], NULL, thread_fball, &a) != 0 ||
        pthread_create(&th[3], NULL, thread_fball, &a) != 0)
        _exit(99);
    pthread_join(th[0], NULL);
    pthread_join(th[1], NULL);
    pthread_join(th[2], NULL);
    pthread_join(th[3], NULL);
    if (which == 0)
        adf_scaled_clear(held);
    adf_modctx_free(ctxs[0]);           /* which = 1: held borrows it: aborts */
    adf_modctx_free(ctxs[1]);
}

/* ------------------------------------------------------------------ the tests */

ADF_TEST(free_with_one_live_borrower_of_each_kind_aborts)
{
    expect("fball local", scn_fball_local, 0, 1);
    expect("scaled", scn_scaled, 0, 1);
    expect("adele fin local", scn_adele_fin, 0, 1);
    expect("cadele fin local", scn_cadele_fin, 0, 1);
    expect("finding R7 (scaled, context 2)", scn_r7, 0, 1);
}

ADF_TEST(free_returns_after_every_borrower_is_cleared)
{
    /* free the context that had the borrower, after the clear: which = 1 frees the other context
       first (no borrower at all), then the first one after the clear */
    expect("fball local", scn_fball_local, 1, 0);
    expect("scaled", scn_scaled, 1, 0);
    expect("adele fin local", scn_adele_fin, 1, 0);
    expect("cadele fin local", scn_cadele_fin, 1, 0);
    expect("global values do not borrow (context 0)", scn_globals_do_not_borrow, 0, 0);
    expect("global values do not borrow (context 1)", scn_globals_do_not_borrow, 1, 0);
    expect("two borrowers cleared", scn_two_borrowers_then_both_cleared, 0, 0);
    expect("two borrowers cleared", scn_two_borrowers_then_both_cleared, 1, 0);
}

ADF_TEST(the_count_is_a_count)
{
    expect("two borrowers, one cleared", scn_two_borrowers, 0, 1);
    expect("two borrowers, one cleared", scn_two_borrowers, 1, 0);
}

ADF_TEST(free_of_null_does_nothing)
{
    adf_modctx_free(NULL);
    ADF_CHECK(1);
}

ADF_TEST(set_moves_the_borrow)
{
    expect("fball set", scn_fball_set, 0, 1);
    expect("fball set", scn_fball_set, 1, 0);
    expect("scaled set and swap", scn_scaled_set_swap, 0, 0);
    expect("scaled set and swap", scn_scaled_set_swap, 1, 1);
}

ADF_TEST(swap_keeps_the_counts)
{
    expect("fball swap", scn_fball_swap, 0, 0);
    expect("fball swap", scn_fball_swap, 1, 1);
    expect("fball swap with a global value", scn_fball_swap_with_global, 0, 1);
    expect("fball swap with a global value", scn_fball_swap_with_global, 1, 0);
}

ADF_TEST(a_value_overwritten_by_a_value_of_another_context_leaves_the_first_context)
{
    expect("fball overwritten by another context", scn_fball_overwritten_by_other_context, 0, 0);
    expect("fball overwritten by another context", scn_fball_overwritten_by_other_context, 1, 1);
    expect("fball overwritten by a global value", scn_fball_overwritten_by_global, 0, 0);
    expect("fball overwritten by an arithmetic result", scn_fball_arithmetic_overwrite, 0, 0);
    expect("scaled conversions", scn_scaled_conversions, 0, 0);
    expect("scaled conversions", scn_scaled_conversions, 1, 1);
    expect("scaled set_fball into another context", scn_scaled_set_fball_moves, 0, 0);
    expect("scaled set_fball into another context", scn_scaled_set_fball_moves, 1, 1);
}

ADF_TEST(a_change_of_context_and_a_change_of_backend_leave_the_counts_right)
{
    expect("scaled set_context", scn_scaled_set_context, 0, 0);
    expect("scaled set_context", scn_scaled_set_context, 1, 1);
    expect("fball backend change", scn_fball_backend_change, 0, 0);
    expect("fball backend change", scn_fball_backend_change, 1, 0);
    expect("fball global to local in another context", scn_fball_global_to_local, 0, 0);
    expect("fball global to local in another context", scn_fball_global_to_local, 1, 1);
}

ADF_TEST(aliased_calls_keep_one_borrow)
{
    expect("fball aliased", scn_fball_aliased, 0, 1);
    expect("fball aliased", scn_fball_aliased, 1, 0);
    expect("scaled aliased", scn_scaled_aliased, 0, 1);
    expect("scaled aliased", scn_scaled_aliased, 1, 0);
}

ADF_TEST(adele_and_cadele_moves)
{
    expect("adele moves", scn_adele_moves, 0, 0);
    expect("adele moves", scn_adele_moves, 1, 1);
    expect("cadele moves", scn_cadele_moves, 0, 0);
    expect("cadele moves", scn_cadele_moves, 1, 1);
}

ADF_TEST(loaders_and_parsers)
{
    expect("load with a binding", scn_load, 0, 1);
    expect("load with a binding", scn_load, 1, 0);
    expect("load overwrites a local value", scn_load_overwrites, 0, 1);
    expect("load overwrites a local value", scn_load_overwrites, 1, 0);
    expect("load of a scaled value", scn_load_scaled, 0, 1);
    expect("load of a scaled value", scn_load_scaled, 1, 0);
    expect("parsers overwrite local values", scn_set_str_overwrites, 0, 0);
    expect("parsers overwrite local values", scn_set_str_overwrites, 1, 0);
}

ADF_TEST(constructors_give_the_borrow_back)
{
    int i;

    for (i = 0; i < 7; i++)
    {
        g_sel = i;
        expect("constructors", scn_constructors, 0, 0);
        expect("constructors", scn_constructors, 1, 0);
    }
}

ADF_TEST(two_threads_init_and_clear_100000_borrowers_then_free_returns)
{
    expect("threads", scn_threads, 0, 0);
    expect("threads, one borrower held", scn_threads, 1, 1);
}

#endif
