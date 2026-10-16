# numbers.md: every example of docs/GUIDE.md, as command and verbatim output

Written by `python3 lanes/docs-guide/check_examples.py --record lanes/docs-guide/numbers.md` from the
repository root of the worktree of lane docs-guide, on its own build (`make -j2 && make -C tools/adf`;
`sh tests/test_exports.sh` for `build/libadelefeld.so`, which the Julia example loads). Each session
below is the command that reproduces it and the transcript that the guide shows: a line `> ...` is an
input line, the lines after it are what `build/adf` printed for it. `sh lanes/docs-guide/check_examples.sh`
reruns all of them and diffs them against the guide.

## session 1 (GUIDE.md:37)

    printf '%s\n' 'add 1/2 with 1/3' 'type 1/2' | build/adf

```text
> add 1/2 with 1/3
5/6
> type 1/2
rat
```

## session 2 (GUIDE.md:61)

    printf '%s\n' 'idele 22/7' 'abs (3.142857142857142857 +/- 2.4e-19 ; 22/7 * [1]) with real' 'abs (3.142857142857142857 +/- 2.4e-19 ; 22/7 * [1]) with 2' 'abs (3.142857142857142857 +/- 2.4e-19 ; 22/7 * [1]) with 7' 'abs (3.142857142857142857 +/- 2.4e-19 ; 22/7 * [1]) with 11' 'abs (3.142857142857142857 +/- 2.4e-19 ; 22/7 * [1]) with 13' | build/adf

```text
> idele 22/7
(3.142857142857142857 +/- 2.4e-19 ; 22/7 * [1])
> abs (3.142857142857142857 +/- 2.4e-19 ; 22/7 * [1]) with real
3.1428571428571428568 +/- 4.5e-19
> abs (3.142857142857142857 +/- 2.4e-19 ; 22/7 * [1]) with 2
1/2
> abs (3.142857142857142857 +/- 2.4e-19 ; 22/7 * [1]) with 7
7
> abs (3.142857142857142857 +/- 2.4e-19 ; 22/7 * [1]) with 11
1/11
> abs (3.142857142857142857 +/- 2.4e-19 ; 22/7 * [1]) with 13
1
```

## session 3 (GUIDE.md:89)

    printf '%s\n' 'project 22/7 with real 2 7 11' 'project 22/7 with 6' | build/adf

```text
> project 22/7 with real 2 7 11
real: 3.142857142857142857 +/- 2.4e-19; 2: 22/7; 7: 22/7; 11: 22/7
> project 22/7 with 6
error: DOMAIN
```

## session 4 (GUIDE.md:112)

    printf '%s\n' 'show [p=5: -1 + O(5^4)]' 'show [p=5: 1/3 + O(5^6)]' | build/adf

```text
> show [p=5: -1 + O(5^4)]
[p=5: 624 + O(5^4)]
> show [p=5: 1/3 + O(5^6)]
[p=5: 10417 + O(5^6)]
```

## session 5 (GUIDE.md:137)

    printf '%s\n' 'show 146 mod 1000' 'project 146 mod 1000 with 2 3 5 7' | build/adf

```text
> show 146 mod 1000
(* ; 146 mod 1000)
> project 146 mod 1000 with 2 3 5 7
2: 2 + O(2^3); 3: 0 + O(3^0); 5: 21 + O(5^3); 7: 0 + O(7^0)
```

## session 6 (GUIDE.md:168)

    printf '%s\n' 'contains 1146 with 146 mod 1000' 'contains -854 with 146 mod 1000' 'contains 22/7 with 146 mod 1000' | build/adf

```text
> contains 1146 with 146 mod 1000
true
> contains -854 with 146 mod 1000
true
> contains 22/7 with 146 mod 1000
false
```

## session 7 (GUIDE.md:180)

    printf '%s\n' 'show (146.2 +/- 0.5 ; 146 mod 1000)' 'reconstruct (146.2 +/- 0.5 ; 146 mod 1000)' 'reconstruct (3.14159 +/- 0.00001 ; 146 mod 1000)' | build/adf

```text
> show (146.2 +/- 0.5 ; 146 mod 1000)
(146.2 +/- 0.51 ; 146 mod 1000)
> reconstruct (146.2 +/- 0.5 ; 146 mod 1000)
146
> reconstruct (3.14159 +/- 0.00001 ; 146 mod 1000)
error: NO_SOLUTION
```

## session 8 (GUIDE.md:196)

    printf '%s\n' 'recover 146 mod 1000 with 30 with 10' 'recover 146 mod 1000 with 100 with 100' | build/adf

```text
> recover 146 mod 1000 with 30 with 10
22/7
> recover 146 mod 1000 with 100 with 100
error: NOT_UNIQUE
```

## session 9 (GUIDE.md:219)

    printf '%s\n' 'roots 0 -1 1 with 2 with 4' 'roots 0 -1 1 with 3 with 4' | build/adf

```text
> roots 0 -1 1 with 2 with 4
0 mod 2^4; 1 mod 2^4
> roots 0 -1 1 with 3 with 4
0 mod 3^4; 1 mod 3^4
```

## session 10 (GUIDE.md:241)

    printf '%s\n' 'show (0.1 ; 0)' 'show (0.5 ; 0)' 'prec 200' 'show (0.1 ; 0)' | build/adf

```text
> show (0.1 ; 0)
(0.099999999999999999995 +/- 5.9e-21 ; 0)
> show (0.5 ; 0)
(0.5 ; 0)
> prec 200
> show (0.1 ; 0)
(0.1 +/- 1.3e-61 ; 0)
```

## session 11 (GUIDE.md:262)

    printf '%s\n' 'add 3 mod 12 with 5 mod 18' 'mul 3 mod 12 with 5 mod 18' 'mul 12 with 5 mod 18' 'mul 1/3 with 5 mod 18' 'mul 1/2 mod 8 with 2/3 mod 9' 'sub 146 mod 1000 with 146 mod 1000' | build/adf

```text
> add 3 mod 12 with 5 mod 18
(* ; 2 mod 6)
> mul 3 mod 12 with 5 mod 18
(* ; 3 mod 6)
> mul 12 with 5 mod 18
(* ; 60 mod 216)
> mul 1/3 with 5 mod 18
(* ; 5/3 mod 6)
> mul 1/2 mod 8 with 2/3 mod 9
(* ; 0 mod 1/6)
> sub 146 mod 1000 with 146 mod 1000
(* ; 0 mod 1000)
```

## session 12 (GUIDE.md:284)

    printf '%s\n' 'contains 2 mod 4 with 0 mod 2' 'equal 0 mod 2 with 2 mod 4' 'overlaps 0 mod 2 with 1 mod 2' 'compare 3 with 3' 'compare 0 mod 2 with 1 mod 2' 'compare 0 mod 2 with 2 mod 4' | build/adf

```text
> contains 2 mod 4 with 0 mod 2
true
> equal 0 mod 2 with 2 mod 4
false
> overlaps 0 mod 2 with 1 mod 2
false
> compare 3 with 3
equal
> compare 0 mod 2 with 1 mod 2
different
> compare 0 mod 2 with 2 mod 4
undecided
```

## session 13 (GUIDE.md:308)

    printf '%s\n' 'prec 8' 'exp_at 5 with 5' 'exp_at (* ; 5 mod 25) with 5' 'exp_at 5 with real' | build/adf

```text
> prec 8
> exp_at 5 with 5
5: 349831 + O(5^8)
> exp_at (* ; 5 mod 25) with 5
5: 6 + O(5^2)
> exp_at 5 with real
real: 148 +/- 1.1
```

## session 14 (GUIDE.md:341)

    printf '%s\n' 'unitof (1 ; 1 mod 4)' 'sin_at (* ; 0 mod 2) with 2' 'recover 146 mod 1000 with 100 with 100' 'sin_at 2 with 2' 'recover 146 mod 1000 with 1 with 1' 'inv 0' 'prec 100001' 'show 1/' 'neg [p=5: 3]' | build/adf

```text
> unitof (1 ; 1 mod 4)
error: UNIT_NOT_CERTIFIED
> sin_at (* ; 0 mod 2) with 2
error: NOT_DETERMINED
> recover 146 mod 1000 with 100 with 100
error: NOT_UNIQUE
> sin_at 2 with 2
error: DOMAIN
> recover 146 mod 1000 with 1 with 1
error: NO_SOLUTION
> inv 0
error: NOT_UNIT
> prec 100001
error: LIMIT
> show 1/
error: PARSE
> neg [p=5: 3]
error: UNSUPPORTED
```

## session 15 (GUIDE.md:374)

    printf '%s\n' 'prec 8' 'sin_at 2 with 2' 'sin_at (* ; 0 mod 2) with 2' 'sin_at (* ; 0 mod 4) with 2' | build/adf

```text
> prec 8
> sin_at 2 with 2
error: DOMAIN
> sin_at (* ; 0 mod 2) with 2
error: NOT_DETERMINED
> sin_at (* ; 0 mod 4) with 2
2: 0 + O(2^2)
```

## session 16 (GUIDE.md:395)

    printf '%s\n' 'root_at 3 with 2 with 2 with 1' 'root_at (* ; 1 mod 4) with 2 with 2 with 1' | build/adf

```text
> root_at 3 with 2 with 2 with 1
error: DOMAIN
> root_at (* ; 1 mod 4) with 2 with 2 with 1
error: NOT_DETERMINED
```

## session 17 (GUIDE.md:422)

    printf '%s\n' 'show 6/-4' 'show 0012/18' 'show 7 mod 3' 'type (* ; 1/3 mod 2)' 'show (2.5 +/- 0.125 ; 3/2 * [5 mod 36])' 'type <1.25 +/- 0.25 ; [5 mod 36]>' 'show {p=5: 21 + O(5^3); inf: 0.5; p=2: 1/2}' 'type {inf: 0.5}' 'show [5 mod 6]' 'show ffun(D=1, M=1; (1) + (0)*i)' 'show rfun()' 'rfun rfun()' | build/adf

```text
> show 6/-4
error: PARSE
> show 0012/18
2/3
> show 7 mod 3
(* ; 1 mod 3)
> type (* ; 1/3 mod 2)
fball
> show (2.5 +/- 0.125 ; 3/2 * [5 mod 36])
(2.5 +/- 0.13 ; 3/2 * [5 mod 36])
> type <1.25 +/- 0.25 ; [5 mod 36]>
idclass
> show {p=5: 21 + O(5^3); inf: 0.5; p=2: 1/2}
{inf: 0.5; p=2: 1/2; p=5: 21 + O(5^3)}
> type {inf: 0.5}
sball
> show [5 mod 6]
[2 mod 3]
> show ffun(D=1, M=1; (1) + (0)*i)
ffun(D=1, M=1; (1) + (0)*i)
> show rfun()
error: UNSUPPORTED
> rfun rfun()
rfun()
```

## session 18 (GUIDE.md:461)

    printf '%s\n' 'add 146 mod 1000 with 22/7' 'mul 146 mod 1000 with 7' 'add (1.5 +/- 0.25 ; 1 mod 4) with (2 ; 1/2)' 'mul (3 ; 1 mod 2) with (0.5 ; 3 mod 6)' 'neg (1.5 +/- 0.25 ; 1 mod 4)' 'div (1 ; 1 mod 2) with 2' 'div 1 mod 2 with 146 mod 1000' 'add 146 mod 1000 with (1 ; 0)' | build/adf

```text
> add 146 mod 1000 with 22/7
(* ; 1044/7 mod 1000)
> mul 146 mod 1000 with 7
(* ; 1022 mod 7000)
> add (1.5 +/- 0.25 ; 1 mod 4) with (2 ; 1/2)
(3.5 +/- 0.25 ; 3/2 mod 4)
> mul (3 ; 1 mod 2) with (0.5 ; 3 mod 6)
(1.5 ; 3 mod 6)
> neg (1.5 +/- 0.25 ; 1 mod 4)
(-1.5 +/- 0.25 ; 3 mod 4)
> div (1 ; 1 mod 2) with 2
(0.5 ; 1/2 mod 1)
> div 1 mod 2 with 146 mod 1000
error: DOMAIN
> add 146 mod 1000 with (1 ; 0)
error: DOMAIN
```

## session 19 (GUIDE.md:484)

    printf '%s\n' 'cap 146 mod 1000 with 10' 'cap 146 mod 1000 with 7' | build/adf

```text
> cap 146 mod 1000 with 10
(* ; 6 mod 10)
> cap 146 mod 1000 with 7
(* ; 0 mod 1)
```

## session 20 (GUIDE.md:500)

    printf '%s\n' 'project (3.14159 +/- 0.00001 ; 146 mod 1000) with real 2 3 5' 'project 1/5 mod 25 with 5' 'project 1/3 mod 15625 with 5' 'project 146 mod 1000 with real' 'project 22/7 with 2 2' | build/adf

```text
> project (3.14159 +/- 0.00001 ; 146 mod 1000) with real 2 3 5
real: 3.14159 +/- 1.1e-5; 2: 2 + O(2^3); 3: 0 + O(3^0); 5: 21 + O(5^3)
> project 1/5 mod 25 with 5
5: 1/5 + O(5^2)
> project 1/3 mod 15625 with 5
5: 10417 + O(5^6)
> project 146 mod 1000 with real
error: DOMAIN
> project 22/7 with 2 2
error: DOMAIN
```

## session 21 (GUIDE.md:522)

    printf '%s\n' 'prec 8' 'exp_at 3 with 3' 'log_at 4 with 3' 'log_at -1 with 2' 'cos_at 3 with 3' 'cos_at (* ; 0 mod 4) with 2' 'sin_at 1 with real' 'log_at 2 with 3' | build/adf

```text
> prec 8
> exp_at 3 with 3
3: 958 + O(3^8)
> log_at 4 with 3
3: 1992 + O(3^8)
> log_at -1 with 2
2: 0
> cos_at 3 with 3
3: 5275 + O(3^8)
> cos_at (* ; 0 mod 4) with 2
2: 1 + O(2^3)
> sin_at 1 with real
real: 0.8398 +/- 0.004
> log_at 2 with 3
error: DOMAIN
```

## session 22 (GUIDE.md:549)

    printf '%s\n' 'prec 8' 'roots_at 2 with 7 with 2' 'root_at 2 with 7 with 2 with 3' 'roots_at -1 with 5 with 2' 'roots_at 2 with 5 with 2' 'roots_at 9 with 2 with 2' 'root_at (* ; 46 mod 125) with 5 with 2 with 1' | build/adf

```text
> prec 8
> roots_at 2 with 7 with 2
7 [3]: 1802916 + O(7^8); 7 [4]: 3961885 + O(7^8)
> root_at 2 with 7 with 2 with 3
7 [3]: 1802916 + O(7^8)
> roots_at -1 with 5 with 2
5 [2]: 280182 + O(5^8); 5 [3]: 110443 + O(5^8)
> roots_at 2 with 5 with 2
error: DOMAIN
> roots_at 9 with 2 with 2
2 [+1]: -3; 2 [-1]: 3
> root_at (* ; 46 mod 125) with 5 with 2 with 1
5 [1]: 36 + O(5^3)
```

## session 23 (GUIDE.md:574)

    printf '%s\n' 'powrat_at 9 with 5 with 3/2 with 3' 'powunit_at 6 with 5 with (* ; 2 mod 25)' | build/adf

```text
> powrat_at 9 with 5 with 3/2 with 3
5 [3]: 27
> powunit_at 6 with 5 with (* ; 2 mod 25)
5: 36 + O(5^3)
```

## session 24 (GUIDE.md:588)

    printf '%s\n' 'root 8 with 3' 'root 2 with 2' 'root (4 ; 9/4) with 2 with -1' 'root (4 ; 2 mod 4) with 2' | build/adf

```text
> root 8 with 3
2
> root 2 with 2
error: DOMAIN
> root (4 ; 9/4) with 2 with -1
(-2 ; -3/2)
> root (4 ; 2 mod 4) with 2
error: NOT_DETERMINED
```

## session 25 (GUIDE.md:606)

    printf '%s\n' 'digits 10' 'exp (0.5 ; 0)' 'cos (1 ; 0)' 'exp 1' 'exp (0 ; 0 mod 4)' | build/adf

```text
> digits 10
> exp (0.5 ; 0)
(1.648721271 +/- 3e-10 ; 1)
> cos (1 ; 0)
(0.5403023059 +/- 3.2e-11 ; 1)
> exp 1
error: DOMAIN
> exp (0 ; 0 mod 4)
error: NOT_DETERMINED
```

## session 26 (GUIDE.md:628)

    printf '%s\n' 'roots -2 0 1 with 7 with 5' 'roots 1 0 1 with 5 with 4' 'roots 1 0 1 with 3 with 4' 'digits 10' 'realroots -2 0 1' 'realroots 1 0 1' | build/adf

```text
> roots -2 0 1 with 7 with 5
4567 mod 7^5; 12240 mod 7^5
> roots 1 0 1 with 5 with 4
182 mod 5^4; 443 mod 5^4
> roots 1 0 1 with 3 with 4
none
> digits 10
> realroots -2 0 1
-1.414213562 +/- 3.8e-10; 1.414213562 +/- 3.8e-10
> realroots 1 0 1
none
```

## session 27 (GUIDE.md:651)

    printf '%s\n' 'reconstruct (0.33 +/- 0.01 ; 1/3 mod 1)' 'reconstruct 146 mod 1000 with 0 with 999' 'reconstruct 146 mod 1000 with -1000 with 1000' 'recover 3 mod 7 with 1 with 2' | build/adf

```text
> reconstruct (0.33 +/- 0.01 ; 1/3 mod 1)
1/3
> reconstruct 146 mod 1000 with 0 with 999
146
> reconstruct 146 mod 1000 with -1000 with 1000
error: NOT_UNIQUE
> recover 3 mod 7 with 1 with 2
-1/2
```

## session 28 (GUIDE.md:672)

    printf '%s\n' 'digits 10' 'mul (2.5 +/- 0.125 ; 3/2 * [5 mod 36]) with 22/7' 'inv (2.5 +/- 0.125 ; 3/2 * [5 mod 36])' 'norm (2.5 +/- 0.125 ; 3/2 * [5 mod 36])' 'class (2.5 +/- 0.125 ; 3/2 * [5 mod 36])' 'valuation (2.5 +/- 0.125 ; 3/2 * [5 mod 36]) with 3' 'hull (2.5 +/- 0.125 ; 3/2 * [5 mod 36])' 'pow [5 mod 36] with 2' 'unitof (3 ; 3)' 'norm (3.142857142857142857 +/- 2.4e-19 ; 22/7 * [1])' | build/adf

```text
> digits 10
> mul (2.5 +/- 0.125 ; 3/2 * [5 mod 36]) with 22/7
(7.86 +/- 0.4 ; 33/7 * [5 mod 36])
> inv (2.5 +/- 0.125 ; 3/2 * [5 mod 36])
(0.401 +/- 0.021 ; 2/3 * [29 mod 36])
> norm (2.5 +/- 0.125 ; 3/2 * [5 mod 36])
1.667 +/- 0.084
> class (2.5 +/- 0.125 ; 3/2 * [5 mod 36])
<1.667 +/- 0.084 ; [5 mod 36]>
> valuation (2.5 +/- 0.125 ; 3/2 * [5 mod 36]) with 3
1
> hull (2.5 +/- 0.125 ; 3/2 * [5 mod 36])
(2.5 +/- 0.13 ; 15/2 mod 54)
> pow [5 mod 36] with 2
[25 mod 36]
> unitof (3 ; 3)
(3 ; 3 * [1])
> norm (3.142857142857142857 +/- 2.4e-19 ; 22/7 * [1])
1 +/- 3.3e-19
```

## session 29 (GUIDE.md:699)

    printf '%s\n' 'prec 5' 'Log (1 ; 4 * [1 mod 9])' 'Log_at (1 ; 4 * [1 mod 9]) with 3' 'Log_refine (1 ; 4 * [1 mod 9]) with 3 5' | build/adf

```text
> prec 5
> Log (1 ; 4 * [1 mod 9])
(0 ; 0 mod 4)
> Log_at (1 ; 4 * [1 mod 9]) with 3
3: 3 + O(3^2)
> Log_refine (1 ; 4 * [1 mod 9]) with 3 5
(0 ; 120 mod 180)
```

## session 30 (GUIDE.md:714)

    printf '%s\n' 'legendre 2 with 7' 'legendre 3 with 7' 'jacobi 2 with 15' 'kronecker 3 with 2' 'legendre 2 mod 7 with 7' 'kronecker 1 mod 4 with 2' 'hilbert_at -1 with -1 with real' 'hilbert_at -1 with -1 with 2' 'hilbert_at -1 with -1 with 3' | build/adf

```text
> legendre 2 with 7
1
> legendre 3 with 7
-1
> jacobi 2 with 15
1
> kronecker 3 with 2
-1
> legendre 2 mod 7 with 7
1
> kronecker 1 mod 4 with 2
error: NOT_DETERMINED
> hilbert_at -1 with -1 with real
-1
> hilbert_at -1 with -1 with 2
-1
> hilbert_at -1 with -1 with 3
1
```

## session 31 (GUIDE.md:745)

    printf '%s\n' 'binomtight 0 mod 8 with 4' 'binom 5 mod 8 with 2' 'profpowfine [2 mod 5] with 2 mod 4' 'haar_volume 1/2 mod 8/3' 'cyclo_exp_u <17 ; [2 mod 3]> with 6' | build/adf

```text
> binomtight 0 mod 8 with 4
(* ; 0 mod 2)
> binom 5 mod 8 with 2
(* ; 2 mod 4)
> profpowfine [2 mod 5] with 2 mod 4
[49 mod 120]
> haar_volume 1/2 mod 8/3
3/8
> cyclo_exp_u <17 ; [2 mod 3]> with 6
5
```

## session 32 (GUIDE.md:766)

    printf '%s\n' 'show (0.5 ; 0) + Q' 'qadd_rat (0 ; 1/3) + Q with -7/3' 'qreduce (0 ; 0 mod 1/2) + Q with 2' 'qadd (0.75 +/- 0.25 ; 0 mod 3) + Q with (0.5 +/- 0.5 ; 1 mod 3) + Q with 2' 'qoverlaps (0.75 +/- 0.25 ; 0 mod 3) + Q with (0.25 +/- 0.25 ; 2 mod 3) + Q with 42' | build/adf

```text
> show (0.5 ; 0) + Q
(0.5 ; 0) + Q
> qadd_rat (0 ; 1/3) + Q with -7/3
(0 ; 1/3) + Q
> qreduce (0 ; 0 mod 1/2) + Q with 2
union((0 ; 0 mod 1), (0.5 ; 0 mod 1)) + Q
> qadd (0.75 +/- 0.25 ; 0 mod 3) + Q with (0.5 +/- 0.5 ; 1 mod 3) + Q with 2
union((0.5 +/- 0.51 ; 0 mod 3), (0.75 +/- 0.26 ; 1 mod 3)) + Q
> qoverlaps (0.75 +/- 0.25 ; 0 mod 3) + Q with (0.25 +/- 0.25 ; 2 mod 3) + Q with 42
1
```

## session 33 (GUIDE.md:783)

    printf '%s\n' 'prec 128' 'digits 5' 'psi (0 ; 1/3)' 'psi_at (0 ; 1/3) with 3' 'psi_at (0 ; 1/3) with 5' 'psi_phase (0.5 ; 1/3)' 'psi (0 ; 0 mod 1/2)' 'psi_strict (0 ; 0 mod 1/2)' | build/adf

```text
> prec 128
> digits 5
> psi (0 ; 1/3)
(-0.5) + (0.86603 +/- 4.6e-6)*i
> psi_at (0 ; 1/3) with 3
(-0.5) + (0.86603 +/- 4.6e-6)*i
> psi_at (0 ; 1/3) with 5
(1) + (0)*i
> psi_phase (0.5 ; 1/3)
5/6
> psi (0 ; 0 mod 1/2)
(0 +/- 1.1) + (0)*i
> psi_strict (0 ; 0 mod 1/2)
error: NOT_DETERMINED
```

## session 34 (GUIDE.md:810)

    printf '%s\n' 'prec 128' 'digits 5' 'char char(q=16, n=9, s=(0) + (0)*i)' 'chi char(q=5, n=2, s=(0) + (0)*i) with 2' 'gauss char(q=8, n=7, s=(0) + (0)*i)' 'char_conj char(q=5, n=2, s=(1) + (-2)*i)' 'char_eval char(q=3, n=2, s=(1) + (0)*i) with (-2 ; 1 * [1])' | build/adf

```text
> prec 128
> digits 5
> char char(q=16, n=9, s=(0) + (0)*i)
char(q=8, n=5, s=(0) + (0)*i)
> chi char(q=5, n=2, s=(0) + (0)*i) with 2
(0) + (1)*i
> gauss char(q=8, n=7, s=(0) + (0)*i)
e=1 tau=(0) + (2)*i W=(1) + (0)*i
> char_conj char(q=5, n=2, s=(1) + (-2)*i)
char(q=5, n=3, s=(1) + (2)*i)
> char_eval char(q=3, n=2, s=(1) + (0)*i) with (-2 ; 1 * [1])
(-2) + (0)*i
```

## session 35 (GUIDE.md:835)

    printf '%s\n' 'prec 128' 'digits 10' 'rfun_eval rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)) with 0.5' 'rfun_fourier rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i))' 'rfun_integral rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i))' 'ffun_fourier ffun(D=1, M=2; (1) + (0)*i, (0) + (0)*i)' 'ffun_integral ffun(D=1, M=2; (1) + (0)*i, (0) + (0)*i)' | build/adf

```text
> prec 128
> digits 10
> rfun_eval rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)) with 0.5
(0.4559381278 +/- 3.5e-11) + (0)*i
> rfun_fourier rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i))
rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i))
> rfun_integral rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i))
(1) + (0)*i
> ffun_fourier ffun(D=1, M=2; (1) + (0)*i, (0) + (0)*i)
ffun(D=2, M=1; (0.5) + (0)*i, (0.5) + (0)*i)
> ffun_integral ffun(D=1, M=2; (1) + (0)*i, (0) + (0)*i)
(0.5) + (0)*i
```

## session 36 (GUIDE.md:855)

    printf '%s\n' 'prec 144' 'digits 10' 'tensor_eval rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)) with ffun(D=1, M=1; (1) + (0)*i) with (0.5 ; 0 mod 2)' 'poisson rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)) with ffun(D=1, M=1; (1) + (0)*i) with 20' | build/adf

```text
> prec 144
> digits 10
> tensor_eval rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)) with ffun(D=1, M=1; (1) + (0)*i) with (0.5 ; 0 mod 2)
(0.4559381278 +/- 3.5e-11) + (0)*i
> poisson rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)) with ffun(D=1, M=1; (1) + (0)*i) with 20
(1.086434811 +/- 2.2e-10) + (0 +/- 1.1e-12)*i | (1.086434811 +/- 2.2e-10) + (0 +/- 1.1e-12)*i | NL=2 NR=2
```

## session 37 (GUIDE.md:880)

    printf '%s\n' 'prec 128' 'digits 10' 'tate_vector char(q=1, n=1, s=(0) + (0)*i)' 'tate_integral char(q=1, n=1, s=(0) + (0)*i) with ((2) + (0)*i ; 0) with 53' 'tate_integral char(q=1, n=1, s=(0) + (0)*i) with ((1) + (0)*i ; 0) with 53' 'tate_integral char(q=1, n=1, s=(0) + (0)*i) with ((1 +/- 0.5) + (0)*i ; 0) with 53' | build/adf

```text
> prec 128
> digits 10
> tate_vector char(q=1, n=1, s=(0) + (0)*i)
rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)) | ffun(D=1, M=1; (1) + (0)*i)
> tate_integral char(q=1, n=1, s=(0) + (0)*i) with ((2) + (0)*i ; 0) with 53
(0.5235987756 +/- 1.8e-12) + (0 +/- 2.6e-18)*i | halfplane Re(s) > 1 | s=(2) + (0)*i
> tate_integral char(q=1, n=1, s=(0) + (0)*i) with ((1) + (0)*i ; 0) with 53
error: DOMAIN
> tate_integral char(q=1, n=1, s=(0) + (0)*i) with ((1 +/- 0.5) + (0)*i ; 0) with 53
error: NOT_DETERMINED
```

## session 38 (GUIDE.md:901)

    printf '%s\n' 'prec 160' 'digits 10' 'tate_continue char(q=1, n=1, s=(0) + (0)*i) with ((-1) + (0)*i ; 0) with 80' 'tate_completed char(q=1, n=1, s=(0) + (0)*i) with ((0.5) + (0)*i ; 0) with 80' 'tate_fe char(q=1, n=1, s=(0) + (0)*i) with ((0.5) + (0)*i ; 0) with 80' | build/adf

```text
> prec 160
> digits 10
> tate_continue char(q=1, n=1, s=(0) + (0)*i) with ((-1) + (0)*i ; 0) with 80
(0.5235987756 +/- 1.8e-12) + (0 +/- 2.3e-26)*i | continuation | s=(-1) + (0)*i
> tate_completed char(q=1, n=1, s=(0) + (0)*i) with ((0.5) + (0)*i ; 0) with 80
(-3.976966226 +/- 5e-10) + (0 +/- 2.1e-26)*i | completed | s=(0.5) + (0)*i
> tate_fe char(q=1, n=1, s=(0) + (0)*i) with ((0.5) + (0)*i ; 0) with 80
(-3.976966226 +/- 5e-10) + (0 +/- 2.1e-26)*i | (-3.976966226 +/- 5e-10) + (0 +/- 1.5e-27)*i | overlap=yes | s=(0.5) + (0)*i
```

## session 39 (GUIDE.md:919)

    printf '%s\n' 'prec 128' 'digits 5' 'local_zeta_factor_at ((2) + (0)*i ; 0) with 2' 'local_zeta_factor_at ((2) + (0)*i ; 0) with real' 'tate_local ((2) + (0)*i ; 0) with 2 with ((1) + (0)*i ; 0) with char(q=1, n=1, s=(0) + (0)*i)' 'tate_gamma ((2) + (0)*i ; 0) with 2 with ((1) + (0)*i ; 0) with char(q=1, n=1, s=(0) + (0)*i)' | build/adf

```text
> prec 128
> digits 5
> local_zeta_factor_at ((2) + (0)*i ; 0) with 2
(1.3333 +/- 3.4e-5) + (0)*i
> local_zeta_factor_at ((2) + (0)*i ; 0) with real
(0.31831 +/- 1.2e-7) + (0)*i
> tate_local ((2) + (0)*i ; 0) with 2 with ((1) + (0)*i ; 0) with char(q=1, n=1, s=(0) + (0)*i)
continuation S=((2) + (0)*i ; 0): (1.3333 +/- 3.4e-5) + (0)*i
> tate_gamma ((2) + (0)*i ; 0) with 2 with ((1) + (0)*i ; 0) with char(q=1, n=1, s=(0) + (0)*i)
continuation S=((2) + (0)*i ; 0): (-0.75 +/- 3e-39) + (0)*i
```

## session 40 (GUIDE.md:942)

    printf '%s\n' 'dump 22/7' 'dump 146 mod 1000' 'dump (0.1 ; 146 mod 1000)' 'load adf1 Q adele 1 3333333333333333 -41 ccccccd -5f g 92 3e8 1' 'dump [p=5: 21 + O(5^3)]' 'dump ffun(D=1, M=2; (1) + (0)*i, (0) + (0)*i)' | build/adf

```text
> dump 22/7
adf1 Q rat 16 7
> dump 146 mod 1000
adf1 Q fball g 92 3e8 1
> dump (0.1 ; 146 mod 1000)
adf1 Q adele 1 3333333333333333 -41 ccccccd -5f g 92 3e8 1
> load adf1 Q adele 1 3333333333333333 -41 ccccccd -5f g 92 3e8 1
(0.099999999999999999995 +/- 5.9e-21 ; 146 mod 1000)
> dump [p=5: 21 + O(5^3)]
adf1 Q lball 5 b 15 0 3
> dump ffun(D=1, M=2; (1) + (0)*i, (0) + (0)*i)
adf1 Q ffun 1 2 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
```

## hello.c

    cc -std=c11 -Wall -Wextra -Werror -Iinclude docs/examples/hello.c build/libadelefeld.a -lflint -lgmp -lm -o build/hello && ./build/hello

```text
set_fmpz3:                 OK
f =                        (* ; 146 mod 1000)
set_arb_fball:             OK
x =                        (146.25 +/- 0.25 ; 146 mod 1000)
adele_set_str:             OK
y =                        (-100 +/- 0.0011 ; -100)
z = x + y =                (46.25 +/- 0.26 ; 46 mod 1000)
reconstruct:               OK
q =                        46
place_prime 5:             OK
lball_set_fball:           OK
z at 5 =                   [p=5: 46 + O(5^3)]
root_count:                OK, 2 branches
root_seed seed 1:          OK
sqrt(z) at 5, branch 1 =   [p=5: 36 + O(5^3)]
sqrt(2 + 125 Zhat) at 5:   DOMAIN
```

## julia

    julia/Adelefeld/bin/adelefeld-julia --startup-file=no --project=julia/Adelefeld <the program of the guide, section 5>

```text
(* ; 146 mod 1000)
(* ; 1044/7 mod 1000)
(* ; 1022 mod 7000)
```

## The numbers derived in the prose

    python3 lanes/docs-guide/arith_checks.py

```text
(22/7) * (1/2) * 7 * (1/11): 1
624 in base 5: 4444
10417 in base 5: 313132
3 * 10417: 31251 = 2 * 5^6 + 1
1/3 mod 5^10 in base 5: 3131313132
146 mod 8, 146 mod 125: 2, 21
7 * 146: 1022
(22/7 - 146) / 1000: -1/7
x^2 = x modulo 30: [0, 1, 6, 10, 15, 16, 21, 25]
gcd(1000, 10), gcd(1000, 7): 10, 1
36^2: 1296 = 46 + 10 * 125
1/2 mod 7, -1/2 mod 7: 4, 3
1 / (8/3): 3/8
pi/6: 0.5235987755982988
1/pi: 0.3183098861837907
(1 - 2^-2)^-1: 4/3
0x3333333333333333 * 2^-65: 0.09999999999999999999457898913757247782996
0.1 - that midpoint: 5.4210108624275225e-21
```
