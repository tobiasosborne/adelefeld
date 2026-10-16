# numbers.md: every number of README.md and docs/assets, with the command that produced it (D-DOC3)

Lane docs-readme, 2026-10-16, worktree `/home/tobias/Projects/adelefeld-wt/docs-readme` at `d9709ef` (the code of
`a9a3bf4`; `git diff --stat a9a3bf4 d9709ef` touches only `.gitignore`, `HANDOFF.md` and a worklog).
Every command runs from the repository root after `make -j2 && make -C tools/adf` (and `sh tests/test_exports.sh`
for the Julia lines). The outputs below are pasted from the runs, not typed.

## 1. The build

    $ env -i HOME=$HOME PATH=$PATH timeout 600 sh -c 'make -j2 && make -C tools/adf' > /dev/null; echo $?
    0

The time on the README ("18 s here on two cores") is a clean build in a copy of `Makefile`, `src`, `include`,
`tests/support`, `tools/adf` in the scratch directory, at load average 1.91:

    $ /usr/bin/time -f 'wall %e s' sh -c 'timeout 600 make -j2 >/dev/null && timeout 600 make -C tools/adf >/dev/null'
    wall 18.31 s

`make -j2` does not build `build/libadelefeld.so` (the static library only, `Makefile:81`); `sh tests/test_exports.sh`
does (17.3 s wall here) and prints, last line:

    test_exports: passed: 625 of 625 declared functions are exported, 0 are not implemented yet, no exported name is undeclared, no variadic function

FLINT and the toolchain:

    $ grep -n 'define FLINT_VERSION' /usr/include/flint/flint.h
    97:#define FLINT_VERSION "3.0.1"
    $ dpkg -l | grep -i flint | awk '{print $2, $3}'
    libflint-dev 3.0.1-3.1build1
    libflint18t64:amd64 3.0.1-3.1build1
    $ lsb_release -ds
    Linux Mint 22.3
    $ julia --version
    julia version 1.12.5

`Makefile:35` names FLINT 3.0.1; `CFLAGS` has `-std=c11` (`Makefile`, the `CFLAGS ?=` line): the badge "C11".

## 2. Every command block of README.md, run as written (`lanes/docs-readme/check_readme_commands.py`)

Each `$ ` line of the README is run in `env -i HOME PATH JULIA_DEPOT_PATH=<new empty dir> sh -c`, under `timeout`,
and its standard output compared with the lines the README shows. The `git clone` line is not run (network); the
build line is run in section 1.


    $ timeout 900 python3 lanes/docs-readme/check_readme_commands.py

    $ echo 'mul (3.0 +/- 0.1 ; 146 mod 1000) with 7' | build/adf
    (21 +/- 0.71 ; 1022 mod 7000)
    [OK]

    $ echo 'project 22/7 with real 2 3 5 7' | build/adf
    real: 3.142857142857142857 +/- 2.4e-19; 2: 22/7; 3: 22/7; 5: 22/7; 7: 22/7
    [OK]

    $ printf 'prec 6\nroots_at -1 with 5 with 2\n' | build/adf
    5 [2]: 14557 + O(5^6); 5 [3]: 1068 + O(5^6)
    [OK]

    $ echo 'recover 146 mod 1000 with 30 with 15' | build/adf
    22/7
    [OK]

    $ printf 'prec 128\ndigits 10\n%s\n' \
        'tate_integral char(q=1, n=1, s=(0) + (0)*i) with ((2) + (0)*i ; 0) with 53' | build/adf
    (0.5235987756 +/- 1.8e-12) + (0 +/- 2.6e-18)*i | halfplane Re(s) > 1 | s=(2) + (0)*i
    [OK]

    $ sh tests/test_exports.sh > /dev/null   # builds build/libadelefeld.so and checks its 625 exports

    [OK]

    $ julia/Adelefeld/bin/adelefeld-julia --project=julia/Adelefeld -e 'using Adelefeld; println(Rat(1)/3)'
    1/3
    [OK]

    $ julia/Adelefeld/bin/adelefeld-julia --project=julia/Adelefeld -e \
        'using Adelefeld; println(readvalue("(3.0 +/- 0.1 ; 146 mod 1000)") * Rat(7))'
    (21 +/- 0.71 ; 1022 mod 7000)
    [OK]

    $ printf 'prec 160\ndigits 10\n%s\n' \
        'tate_fe char(q=1, n=1, s=(0) + (0)*i) with ((0.5) + (0)*i ; 0) with 80' | build/adf
    (-3.976966226 +/- 5e-10) + (0 +/- 2.1e-26)*i | (-3.976966226 +/- 5e-10) + (0 +/- 1.5e-27)*i | overlap=yes | s=(0.5) + (0)*i
    [OK]

    9 commands: 9 agree, 0 skipped, 0 differ


The Julia precompilation on the first call in a fresh depot (the README's "a second or two"), from the first run
of the lane: `1148.8 ms  ✓ Adelefeld (serial)`.

## 3. The terminal session of docs/assets/terminal.svg (`sh lanes/docs-readme/terminal_session.sh`)


    $ echo 'add (0 ; 0) with 22/7' | build/adf
    (3.142857142857142857 +/- 2.4e-19 ; 22/7)
    $ echo 'project 22/7 with real 2 3 5 7' | build/adf
    real: 3.142857142857142857 +/- 2.4e-19; 2: 22/7; 3: 22/7; 5: 22/7; 7: 22/7
    $ printf 'prec 6\nroots_at -1 with 5 with 2\n' | build/adf
    5 [2]: 14557 + O(5^6); 5 [3]: 1068 + O(5^6)
    $ echo 'roots_at -1 with 3 with 2' | build/adf
    error: DOMAIN
    $ echo 'mul (3.0 +/- 0.1 ; 146 mod 1000) with 7' | build/adf
    (21 +/- 0.71 ; 1022 mod 7000)
    $ echo 'sin_at (* ; 0 mod 2) with 2' | build/adf
    error: NOT_DETERMINED
    $ echo 'recover 146 mod 1000 with 30 with 15' | build/adf
    22/7


`lanes/docs-readme/make_assets.py` runs the same seven commands and refuses to write the SVG if an output differs
from its list EXPECT (the strings of this section). Independent checks of the outputs, plain Python integers:

    $ python3 -c "m=5**6
    for a in (14557,1068): print(a, (a*a+1)%m, a%5)
    print((146*7-22)%1000==0, any((x*x+1)%3==0 for x in range(3)))"
    14557 0 2
    1068 0 3
    True False

So `14557^2 = 1068^2 = -1 mod 5^6 = 15625`, the branch labels `[2]`, `[3]` are the residues modulo 5, `7 * 146 = 1022
= 22 mod 1000`, and `-1` is not a square modulo 3 (the reason of `DOMAIN` at 3). The real radius `0.71` printed for
`7 * (3.0 +/- 0.1)` contains the exact `0.7` (outward rounding of the printer, conventions 9.5).

## 4. The places picture docs/assets/places.svg (`sh lanes/docs-readme/places_digits.sh`)

Levels 1 to 4 at p = 2, 3, 5, then the real coordinate at `digits 7`, then the valuations at 2, 3, 5, 7, 11:


    2: 0 + O(2^1)
    2: 2 + O(2^2)
    2: 2 + O(2^3)
    2: 10 + O(2^4)
    3: 1 + O(3^1)
    3: 7 + O(3^2)
    3: 7 + O(3^3)
    3: 61 + O(3^4)
    5: 1 + O(5^1)
    5: 21 + O(5^2)
    5: 21 + O(5^3)
    5: 271 + O(5^4)
    real: 3.142857 +/- 1.5e-7
    1
    0
    0
    -1
    1


Digits (low first, read off the residues by the generator, which asserts `sum d_k p^k = c`): `10 = 0 + 1*2 + 0*4 +
1*8`; `61 = 1 + 2*3 + 0*9 + 2*27`; `271 = 1 + 4*5 + 0*25 + 2*125`. The ruler places `22/7 = 3.142857142857143`
(`python3 -c 'print(22/7)'`). "twelve nodes" = 3 primes x 4 levels. The valuation at 11 is 1 because `22 = 2 * 11`.

## 5. The status table of README.md


    $ echo 'sin_at (* ; 0 mod 2) with 2' | build/adf
    error: NOT_DETERMINED
    $ echo 'roots_at -1 with 3 with 2' | build/adf
    error: DOMAIN
    $ echo 'roots -2 0 1 with 7 with 3000000' | build/adf
    error: LIMIT
    $ echo 'roots 2 -1 at 5 prec 8' | build/adf
    error: PARSE


The statuses and their count: `include/adelefeld/status.h` lines 17 to 28 define 11 codes (`sed -n 17,28p
include/adelefeld/status.h | grep -c define` gives 11), `OK` and ten others. The four classes and their shapes:
`docs/ux/README.md` lines 58 to 60. The reason of `NOT_DETERMINED`: the 2-adic sine converges on `4 Z_2`
(`tools/adf/README.md` line 469, "c=2 at 2"), and `2 Z_2` holds 0 and 2; the rule "a ball that meets the domain
and its complement is NOT_DETERMINED" is line 474. The `LIMIT` example is the manual's own, line 177.

## 6. Tate's thesis lines (README "Five minutes" and the details)

The two commands are run in section 2 (`tate_integral`, `tate_fe`). Their values against mpmath:

    $ python3 -c "import mpmath as m; m.mp.dps=20; print(m.pi/6); print(m.pi**(-0.25)*m.gamma(0.25)*m.zeta(0.5))"
    0.52359877559829887308
    -3.9769662255065128793

`pi^(-1) Gamma(1) zeta(2) = pi/6 = 0.5235987756` is inside `0.5235987756 +/- 1.8e-12`; `Lambda(1/2) = pi^(-1/4)
Gamma(1/4) zeta(1/2) = -3.976966226` is inside both sides of `tate_fe`. The formula: `docs/SPEC.md` lines 476 to 477,
quoted there from `refs/src/tate-warwick/tatesthesis_notes.txt` lines 510 to 512 (read in this lane: `pi^(-s/2)
Gamma(s/2)` times the Euler product); `I_chi(s) = pi^-z Gamma(z) L(s, chi)`: `tools/adf/README.md` line 970.

## 7. The badges and counts

    $ ls tests/test_*.c | wc -l
    107

The Julia count 5369 and "5 of 5 green" are NOT computed in this lane (the brief forbids running the suites): they
are quoted from `HANDOFF.md` line 22 ("ALL FIVE SUITES GREEN on `a9a3bf4` at 01:53 to 02:03 on 2026-10-16 ... 107
programs; ... Julia with `Pkg.test` 5369"). The badge text names the commit; the README's build notes say where the
counts come from. Licence: `LICENSE` line 1, "GNU AFFERO GENERAL PUBLIC LICENSE", version 3 (line 2).

## 8. Other numbers of README.md and where they come from

| Number | Source |
|---|---|
| `3.14159 ± 0.00001` | the example of `docs/SPEC.md` line 65 (illustrative, not a computation) |
| `146 + 2^3 Z_2`, `146 + 5^3 Z_5` | `1000 = 2^3 * 5^3`; SPEC lines 74 to 75 and 85 |
| `|n| <= 30`, `0 < d <= 15` | the operands 30 and 15 of the `recover` command (the bounds of `docs/ux/brief-1-adelica-notebook.md` line 42) |
| `22/7` valuation -1 at 7 | section 4, `valuation ... with 7` |
| the `1/5` quotation | `docs/SPEC.md` line 535 |
| `tools/adf/README.md` line 260 | `grep -n 'centre> + O' tools/adf/README.md` |
| `Makefile` lines 35 to 41 | the comment on FLINT and MPFR |
| `tools/adf/adf.c:4455` (report) | "the whole script is read first" |
| milestone states and dates | `HANDOFF.md` lines 80 (milestone 5), 147 to 149 (3 and 4), 305 (1F), 719 to 720 (S, 2), 949 (1), 1308 (0), 11 and 56 (J-b step 1 of 4), 85 and 121 (J-a); `docs/reviews/m3/review-m3-wp31-wp32.md` (WP 3.1, 3.2 reviewed); `docs/PLAN.md` row 1.6 (R) |
| 625 exports | section 1 |
| 79 command names | each found as a string literal in `tools/adf/adf.c` (a grep loop in this lane: no name missing) |

