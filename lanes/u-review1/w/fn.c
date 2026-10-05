/* item 3: loaded lball / sball values through every public function (INV build aborts on a non-canonical
   input).  stdin lines: "<lball|sball> <text with spaces>".  one fork per line, the function name on crash. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include "adelefeld/dump.h"
#include "adelefeld/text.h"

static volatile const char *cur = "load";
static void onsig(int s)
{
    char b[200]; int n = snprintf(b, sizeof b, "CRASH sig %d in %s\n", s, (const char *) cur);
    (void) !write(1, b, n); _exit(1);
}
#define CALL(name, expr) do { cur = name; (void) (expr); } while (0)

static void battery_lball(const adf_lball_t x, const adf_lball_t y)
{
    adf_lball_t z, w, u; adf_rat_t q; slong m, N; int inf; ulong idx; fmpz_t f; size_t len; char *s;
    adf_lball_init(z); adf_lball_init(w); adf_lball_init(u); adf_rat_init(q); fmpz_init(f);
    CALL("is_canonical", adf_lball_is_canonical(x));
    CALL("set", adf_lball_set(z, x));
    CALL("identical", adf_lball_identical(x, z));
    CALL("is_exact", adf_lball_is_exact(x));
    CALL("contains_zero", adf_lball_contains_zero(x));
    CALL("get_prec", adf_lball_get_prec(&N, x));
    CALL("get_center", adf_lball_get_center(q, x));
    CALL("place", adf_lball_place(x));
    CALL("equal_set", adf_lball_equal_set(x, y)); CALL("overlaps", adf_lball_overlaps(x, y)); CALL("contains", adf_lball_contains(x, y));
    CALL("neg", adf_lball_neg(z, x));
    CALL("add", adf_lball_add(z, x, y)); CALL("add_self", adf_lball_add(z, x, x));
    CALL("sub", adf_lball_sub(z, x, y)); CALL("mul", adf_lball_mul(z, x, y)); CALL("mul_self", adf_lball_mul(z, x, x));
    CALL("inv", adf_lball_inv(z, x)); CALL("div", adf_lball_div(z, x, y));
    CALL("valuation", adf_lball_valuation(&m, &inf, x));
    CALL("abs", adf_lball_abs(q, x));
    CALL("decompose", adf_lball_decompose(&m, z, x));
    CALL("decompose_teich", adf_lball_decompose_teich(&m, w, &idx, u, x, 20));
    CALL("decompose_teich64", adf_lball_decompose_teich(&m, w, &idx, u, x, 3));
    CALL("frac", adf_lball_frac(q, x));
    CALL("unit_mod3", adf_lball_unit_mod(f, x, 3)); CALL("unit_mod0", adf_lball_unit_mod(f, x, 0));
    CALL("pow-3", adf_lball_pow_si(z, x, -3)); CALL("pow0", adf_lball_pow_si(z, x, 0)); CALL("pow5", adf_lball_pow_si(z, x, 5));
    CALL("pow_big", adf_lball_pow_si(z, x, LONG_MAX)); CALL("pow_min", adf_lball_pow_si(z, x, LONG_MIN));
    cur = "get_str"; s = adf_lball_get_str(&len, x); if (s) adf_str_free(s);
    cur = "dump_load";
    s = adf_lball_dump_str(&len, x);
    { adf_text_limits_t l = {1u << 30, 100000, LONG_MAX, 1 << 30}; int st = adf_lball_load_str(z, s, len, NULL, &l);
      if (st != 0 || !adf_lball_identical(x, z)) { printf("ROUNDTRIP FAIL %d %s\n", st, s); } }
    adf_str_free(s);
    adf_lball_clear(z); adf_lball_clear(w); adf_lball_clear(u); adf_rat_clear(q); fmpz_clear(f);
}

static void battery_sball(const adf_sball_t x, const adf_sball_t y, int digits)
{
    adf_sball_t z; adf_place_t where, v; adf_lball_t c; arb_t r; size_t len; char *s; slong i;
    adf_sball_init(z); adf_lball_init(c); arb_init(r);
    CALL("is_canonical", adf_sball_is_canonical(x));
    CALL("set", adf_sball_set(z, x)); CALL("identical", adf_sball_identical(x, z));
    CALL("arch", adf_sball_arch(x)); CALL("num_places", adf_sball_num_places(x));
    for (i = -1; i <= adf_sball_num_places(x); i++) {
        CALL("get_place", adf_sball_get_place(&v, x, i));
        if (i >= 0 && i < adf_sball_num_places(x)) {
            adf_sball_get_place(&v, x, i);
            CALL("has_place", adf_sball_has_place(x, v));
            CALL("get_lball", adf_sball_get_lball(c, x, v));
            CALL("get_arb", adf_sball_get_arb(r, x, v));
        }
    }
    CALL("equal_set", adf_sball_equal_set(x, y)); CALL("overlaps", adf_sball_overlaps(x, y)); CALL("contains", adf_sball_contains(x, y));
    CALL("neg", adf_sball_neg(z, &where, x));
    CALL("add", adf_sball_add(z, &where, x, y, 64)); CALL("add_self", adf_sball_add(z, &where, x, x, 64));
    CALL("sub", adf_sball_sub(z, &where, x, y, 64)); CALL("mul", adf_sball_mul(z, &where, x, x, 30)); CALL("mul2", adf_sball_mul(z, &where, x, y, 200));
    CALL("get_str", 0); s = adf_sball_get_str(&len, x, digits); if (s) adf_str_free(s);
    cur = "dump_load";
    s = adf_sball_dump_str(&len, x);
    { adf_text_limits_t l = {1u << 30, 100000, LONG_MAX, 1 << 30}; int st = adf_sball_load_str(z, s, len, NULL, &l);
      if (st != 0 || !adf_sball_identical(x, z)) printf("ROUNDTRIP FAIL %d %s\n", st, s); }
    adf_str_free(s);
    adf_sball_clear(z); adf_lball_clear(c); arb_clear(r);
}

int main(int argc, char **argv)
{
    char *line = NULL; size_t cap = 0; ssize_t n; adf_text_limits_t lim = {1u << 30, 100000, LONG_MAX, 1 << 30};
    char prev_l[1 << 16] = "adf1 Q lball 5 b 3 0 4", prev_s[1 << 16] = "adf1 Q sball n 0";
    long lines = 0, ok = 0, bad = 0;
    int digits = argc > 1 ? atoi(argv[1]) : 20;
    signal(SIGABRT, onsig); signal(SIGSEGV, onsig); signal(SIGILL, onsig); signal(SIGFPE, onsig); signal(SIGALRM, onsig);
    while ((n = getline(&line, &cap, stdin)) > 0) {
        char *sp; size_t len; pid_t pid;
        if (line[n - 1] == '\n') line[--n] = 0;
        sp = strchr(line, ' ');
        if (!sp) continue;
        *sp++ = 0; len = strlen(sp); lines++;
        fflush(stdout);
        pid = getenv("NOFORK") ? 0 : fork();
        if (pid > 0) { int ws; waitpid(pid, &ws, 0); if (WIFSIGNALED(ws) || WEXITSTATUS(ws)) { bad++; printf("  input: %s %s\n", line, sp); } else ok++;
            if (!strcmp(line, "lball")) strncpy(prev_l, sp, sizeof prev_l - 1); else strncpy(prev_s, sp, sizeof prev_s - 1); continue; }
        alarm(8);
        if (!strcmp(line, "lball")) {
            adf_lball_t x, y; int st;
            adf_lball_init(x); adf_lball_init(y);
            cur = "load";
            st = adf_lball_load_str(x, sp, len, NULL, &lim);
            if (st) { printf("LOADSTATUS %d %s\n", st, sp); if (getenv("NOFORK")) continue; _exit(0); }
            st = adf_lball_load_str(y, prev_l, strlen(prev_l), NULL, &lim);
            battery_lball(x, x);
            if (!st) battery_lball(x, y);
            adf_lball_clear(x); adf_lball_clear(y);
        } else {
            adf_sball_t x, y; int st;
            adf_sball_init(x); adf_sball_init(y);
            st = adf_sball_load_str(x, sp, len, NULL, &lim);
            if (st) { printf("LOADSTATUS %d %s\n", st, sp); if (getenv("NOFORK")) continue; _exit(0); }
            st = adf_sball_load_str(y, prev_s, strlen(prev_s), NULL, &lim);
            battery_sball(x, x, digits);
            if (!st) battery_sball(x, y, digits);
            adf_sball_clear(x); adf_sball_clear(y);
        }
        fflush(stdout); if (!getenv("NOFORK")) _exit(0);
    }
    fprintf(stderr, "lines %ld ok %ld crashed %ld\n", lines, ok, bad);
    free(line); return 0;
}
