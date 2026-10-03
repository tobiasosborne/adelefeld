# progress f-review10
- built lib (build/) and ASan+UBSan+INV lib (asan/); harness w/h.c (AT/REF lines), oracle w/o1.py (own, exact ints)
- step 1: t1.py 1060 cases vs enumeration mod p^H: 0 findings; mutant oracle detected (7 of 120)
- step 1b: t6.py (huge primes) 120, t7.py (K up to 1000) 60: 0 findings
- step 2: t2.py/t2b.py 700 refine cases: 0 findings; boundary of CRT bound checked by hand
- step 3: s3.c 34 status lines, t5.py 2400 real-side outputs: 0 findings
- step 4: ASan+LSan (leak test of LSan itself positive): 604 harness lines + 34 status lines clean
- step 5: ~50 driver lines: nothing wrong
