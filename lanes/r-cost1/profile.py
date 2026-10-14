"""Make clock-instrumented copies. Originals are never changed."""
from pathlib import Path
import sys

lane = Path(sys.argv[1] if len(sys.argv) > 1 else "lanes/r-cost1")
lane.mkdir(parents=True, exist_ok=True)
clock = '''
#include <stdio.h>
#include <time.h>
static double cost_now(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC_RAW, &t);
    return t.tv_sec + 1e-9 * t.tv_nsec;
}
'''
s = Path("src/roots.c").read_text()
s = "#define _POSIX_C_SOURCE 200809L\n" + clock + s
s = s.replace("count = real_count(g);", '''double cost_t = cost_now();
        count = real_count(g);
        fprintf(stderr, "section count %.6f\\n", cost_now() - cost_t);''')
s = s.replace("st = adf_roots_real_isolate_counted(cand, &m, g, prec, count);", '''cost_t = cost_now();
        st = adf_roots_real_isolate_counted(cand, &m, g, prec, count);
        fprintf(stderr, "section isolate_refine %.6f\\n", cost_now() - cost_t);''')
# Count and isolation are inside the degree block; finish is outside it.
needle = "st = real_finish(out, g, count, cand, m, prec);"
s = s.replace(needle, '''{
        double cost_t = cost_now();
        ''' + needle + '''
        fprintf(stderr, "section certificate %.6f\\n", cost_now() - cost_t);
    }''')
lane.joinpath("roots_profile.c").write_text(s)
s = Path("src/roots_real.c").read_text()
s = "#define _POSIX_C_SOURCE 200809L\n" + clock + '''
static double cost_exact, cost_secant, cost_sign, cost_shift, cost_isolate;
static long cost_values, cost_qir, cost_signs, cost_fallback;
''' + s
s = s.replace("slong i, d = fmpz_poly_degree(g), t;",
              "double cost_t = cost_now();\n    slong i, d = fmpz_poly_degree(g), t;")
s = s.replace("    fmpz_clear(x);\n}\n\n/* Values", '''    fmpz_clear(x);
    cost_exact += cost_now() - cost_t; cost_values++;
}

/* Values''')
s = s.replace("    arb_t fa, fb, r;", "    double cost_t = cost_now();\n    arb_t fa, fb, r;")
s = s.replace("    return ok;\n}\n\nstatic int\nsign_2exp", '''    cost_secant += cost_now() - cost_t;
    return ok;
}

static int
sign_2exp''')
s = s.replace("    fmpz_t r;\n    arb_t ball;", "    double cost_t = cost_now();\n    fmpz_t r;\n    arb_t ball;")
s = s.replace("        if (s != 0)\n            return s;", '''        if (s != 0)
        {
            cost_sign += cost_now() - cost_t; cost_signs++;
            return s;
        }''')
s = s.replace("    fmpz_clear(r);\n    return s;", '''    fmpz_clear(r);
    cost_sign += cost_now() - cost_t; cost_signs++; cost_fallback++;
    return s;''')
s = s.replace("                if (s != 0)\n                    return s;", '''                if (s != 0)
                {
                    cost_sign += cost_now() - cost_t; cost_signs++;
                    return s;
                }''')
s = s.replace("    slong e = *k - jj;", "    slong e = *k - jj;\n    cost_qir++;")
s = s.replace("    st = refine_local(c, k, point, &f, &df, floor, has_floor, need);",
              '''    cost_shift += cost_now() - cost_t;
    st = refine_local(c, k, point, &f, &df, floor, has_floor, need);''')
s = s.replace("    local_eval f, df;", "    double cost_t = cost_now();\n    local_eval f, df;")
s = s.replace("    *m = 0;\n    if (d < 1", '''    double cost_t = cost_now();
    cost_exact = cost_secant = cost_sign = cost_shift = 0;
    cost_values = cost_qir = cost_signs = cost_fallback = 0;
    *m = 0;
    if (d < 1''')
s = s.replace("        items_sort(&it);", "        cost_isolate = cost_now() - cost_t;\n        items_sort(&it);")
s = s.replace("    items_clear(&it);\n    arf_clear(floor);",
              '''    fprintf(stderr, "detail tree %.6f shift %.6f secant %.6f sign %.6f exact %.6f "
            "values %ld qir %ld signs %ld fallback %ld\\n", cost_isolate, cost_shift, cost_secant,
            cost_sign, cost_exact, cost_values, cost_qir, cost_signs, cost_fallback);
    items_clear(&it);
    arf_clear(floor);''')
lane.joinpath("refine_profile.c").write_text(s)
s = Path("bench/bench_roots_real.c").read_text()
s = s.replace("    adf_rootlist_clear(L);\n    return t1 - t0;", '''    double v0 = now();
    int verified = adf_rootlist_verify_complete(L, g, 0);
    fprintf(stderr, "section verifier %.6f result %d\\n", now() - v0, verified);
    adf_rootlist_clear(L);
    return t1 - t0;''')
lane.joinpath("bench_profile.c").write_text(s)
s = Path("refs/src/flint-src-3.0.1/fmpz_poly/num_real_roots_sturm.c").read_text()
s = "#define _POSIX_C_SOURCE 200809L\n" + clock + s
s = s.replace("    fmpz_t a, b, g, h;", '''    double total_t = cost_now(), rem_time = 0, div_time = 0, t;
    long remainders = 0;
    fmpz_t a, b, g, h;''')
s = s.replace("        _fmpz_poly_pseudo_rem_cohen(A, A, lenA, B, lenB);", '''        t = cost_now();
        _fmpz_poly_pseudo_rem_cohen(A, A, lenA, B, lenB);
        rem_time += cost_now() - t; remainders++;''')
s = s.replace("            _fmpz_vec_scalar_divexact_fmpz(B, B, lenB, b);", '''            t = cost_now();
            _fmpz_vec_scalar_divexact_fmpz(B, B, lenB, b);
            div_time += cost_now() - t;''')
s = s.replace("    _fmpz_vec_clear(W, 2*len - 1);", '''    _fmpz_vec_clear(W, 2*len - 1);
    fprintf(stderr, "sturm total %.6f remainder %.6f division %.6f steps %ld\\n",
            cost_now() - total_t, rem_time, div_time, remainders);''')
lane.joinpath("sturm_profile.c").write_text(s)
