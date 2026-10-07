# Red-green log, lane f4-slice2 (slice 4d, adf_rfun)

All builds in lanes/f4-slice2/build* (make -j2 BUILD=...); every program under timeout.

## A. Vectors

    timeout 300 python3 -B lanes/f4-slice2/gen_vectors.py
    closure 135 records (39 translate, 50 dilate, 11 reflect, 11 conj, 12 add, 12 mul), values 13, texts 164;
    385276 bytes in all

## B/C. Tests first, then code

1. RED (link): tests/test_rfun.c written in full against include/adelefeld/rfun.h before src/rfun.c and the
   text functions existed.
       make -j2 BUILD=lanes/f4-slice2/build lanes/f4-slice2/build/test_rfun  ->  exit 2, 251 undefined
       references (red-a.log). Two compile errors of the test itself were fixed first (fmpz_is_pow_ui does
       not exist; JSONL_STR), and a gcc false -Wstringop-overflow on a flint_malloc array (noinline helpers).
2. GREEN attempt 1 after src/rfun.c and the reader/printer in src/text.c: golden rows passed; texts failed at
   the round trip: the test demanded identical values after reprinting, but a printed radius such as 0.063 is
   not dyadic, so the reread ball differs (conventions 11.3 item 3 asks for containment). Test corrected to
   containment (a test defect, not a code defect).
3. Attempt 2: closure failed on an exactness assertion for translation by 1/3 (8.5 + 8i computed through
   divisions by 3 carries a radius). The assertion was too strong: rounding enters through the division by a
   non-dyadic denominator. Restricted to dyadic q (and, attempt 3, to exactly stored inputs: 6/5 is a ball).
4. Attempt 4: the tightness check (radius below 2^-80 (1 + |v|)) was applied to ball inputs; restricted to
   exact inputs. Then GREEN: rfun: 64874 checks, 0.3 s.
5. Fault "cap of terms off by one" survived (F below): a test gap. RED for the new test was the surviving
   fault itself (the unmutated code passes it); test added: 2^16 + 1 initialized terms refused with LIMIT.
   GREEN: 64876 checks.

## D. Driver and Julia

    expected lines of tests/driver/rfun-{text,algebra,eval}.out written by hand first (digits of 2 pi/3, pi/9,
    exp(-pi/4), exp(-pi) from mpmath and the printer of conventions 9.5) before the first run. No red run of
    the driver was made: the four commands were coded before the fixtures were run. All three fixtures agreed
    on the first run.
    sh tests/test_driver.sh -> 82 cases, 101473 expected lines, all equal.
    julia tests/julia/rfun.jl build/libadelefeld.so -> 9 of 9 passed.

## F. Faults and mutation

    timeout 1500 python3 -B lanes/f4-slice2/plant_faults.py            -> 22 faults, 22 caught (faults.log;
        first run: 20 of 22, one fault did not compile and was rewritten, "cap of terms off by one" survived:
        test gap, see B step 5)
    mutation of src/rfun.c, test_rfun only, SAN + INV, limit 60, seed 404 (mutate.log): stopped at the
        20-minute cap; 13 survivors, 11 of them test gaps. RED for each new test: the survivor mutant itself.
        Tests added (first term invalid, P = [0], cap boundaries 2^16 terms, 2^16 coefficients, 2^20 work for
        product and translation, inputs above the caps from text, INV entry check of each argument alone).
    timeout 1500 python3 -B lanes/f4-slice2/plant_faults.py --survivors -> 11 planted, 11 caught (INV build)
    GREEN: test_rfun 64923 checks (plain, SAN with leaks, clang), 64939 (INV).
