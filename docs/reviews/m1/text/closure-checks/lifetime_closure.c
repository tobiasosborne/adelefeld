#define _POSIX_C_SOURCE 200809L
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include <flint/arb.h>
#include <adelefeld.h>

static void
exercise(int mode)
{
    adf_modctx_struct *a = NULL, *b = NULL;
    adf_scaled_t s;
    adf_fball_t g, l;
    adf_adele_t x;
    adf_rat_t q;
    ulong six = 6, ten = 10;
    int lost = -1;

    if (mode == 4)
    {
        adf_adele_init(x);
        adf_rat_init(q);
        arb_pos_inf(x->inf);
        adf_adele_reconstruct(q, x);
        _Exit(2);
    }
    if (adf_modctx_new_blocks(&a, &six, 1) != ADF_OK)
        _Exit(3);
    if (mode == 6)
    {
        /* A canonical hand-built value has no counted borrow under proposed M1-D10. */
        fmpq_init(s->s);
        fmpz_init(s->u);
        s->mctx = a;
        s->exact = 1;
        if (!adf_scaled_is_canonical(s))
            _Exit(3);
        adf_modctx_free(a);
        _Exit(0);
    }
    if (mode == 1)
    {
        adf_fball_init(g);
        adf_fball_init(l);
        if (adf_fball_set_str(g, "1 mod 6", 7, NULL) != ADF_OK
            || adf_fball_set_local(l, g, a) != ADF_OK)
            _Exit(3);
        adf_modctx_free(a);
        _Exit(2);
    }
    adf_scaled_init(s, a);
    if (mode == 0)
    {
        adf_modctx_free(a);
        _Exit(2);
    }
    if (mode == 2)
    {
        adf_scaled_clear(s);
        adf_modctx_free(a);
        _Exit(0);
    }
    if (adf_modctx_new_blocks(&b, &ten, 1) != ADF_OK)
        _Exit(3);
    if (mode == 3)
    {
        if (adf_scaled_set_context(s, &lost, s, b) != ADF_OK || lost != 0)
            _Exit(3);
        adf_modctx_free(a);
        adf_modctx_free(b);
        _Exit(2);
    }
    /* This manual change is outside proposed M1-D10. It leaves the borrow on a. */
    s->mctx = b;
    adf_scaled_clear(s);
    adf_modctx_free(a);
    _Exit(2);
}

int
main(void)
{
    int mode, failed = 0;
    for (mode = 0; mode < 7; mode++)
    {
        pid_t child = fork();
        int status = 0, abort_expected = mode != 2 && mode != 6;
        if (child < 0)
            return 2;
        if (child == 0)
        {
            struct rlimit no_core = {0, 0};
            if (freopen("/dev/null", "w", stderr) == NULL)
                _Exit(3);
            setrlimit(RLIMIT_CORE, &no_core);
            exercise(mode);
            _Exit(3);
        }
        if (waitpid(child, &status, 0) != child)
            return 2;
        if (abort_expected ? !(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT)
                           : !(WIFEXITED(status) && WEXITSTATUS(status) == 0))
            failed++;
    }
    printf("cases=7 failed=%d\n", failed);
    return failed != 0;
}
