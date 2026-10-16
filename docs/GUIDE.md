# A guide to adelefeld

This guide is for a reader with a first degree in mathematics or physics who has never met an adele. It takes
about an afternoon. It starts with the absolute values of the rational numbers and ends with a Tate integral,
the integral over the adeles that gives the Riemann zeta function. Every example was run on the library, and
its output is pasted here unchanged.

The chapters:

1. [Places and the adeles in one afternoon](#1-places-and-the-adeles-in-one-afternoon)
2. [Enclosures and statuses](#2-enclosures-and-statuses)
3. [The driver, by task](#3-the-driver-by-task)
4. [The C interface in one program](#4-the-c-interface-in-one-program)
5. [The Julia package in one page](#5-the-julia-package-in-one-page)
6. [The four faces](#6-the-four-faces)
7. [Reading the contracts](#7-reading-the-contracts)
8. [Glossary](#8-glossary)

## 0. Before you start

You need a C compiler, GNU make and FLINT 3.0.1 (the library refuses another FLINT at compile time,
`include/adelefeld.h`). From the root of the repository:

    make -j2                 # the library, build/libadelefeld.a
    make -C tools/adf        # the command-line driver, build/adf

The driver `build/adf` reads one command per line and prints one line per command (`tools/adf/README.md:15-26`,
`:381-388`). It is the quickest way to ask the library a question. A command has an operation word and one or
more operands, separated by the word ` with ` (`tools/adf/README.md:45-50`):

    printf '%s\n' 'add 1/2 with 1/3' | build/adf

prints `5/6`. In this guide a session is shown as a block. A line that begins with `> ` is a line you type; the
lines after it are what the driver printed for it. To run a session, give the driver the typed lines without the
`> `, on standard input or in a file (`build/adf file.txt`).

```adf
> add 1/2 with 1/3
5/6
> type 1/2
rat
```

`type` names the kind of a value (`tools/adf/README.md:57`); `rat` is the exact rational. The script
`lanes/docs-guide/check_examples.sh` reruns every session of this guide and compares it with the text.

## 1. Places and the adeles in one afternoon

### 1.1 Two ways for a number to be small

The usual absolute value says that `1/1000` is small. There is a second family of absolute values on the
rationals, one for each prime `p`. Write a non-zero rational as `a = p^m b/c` with `b` and `c` not divisible by
`p`; then `|a|_p = p^(-m)`, and `|0|_p = 0` (`refs/src/evertse-padic/evertsepadicnotes.txt:22-24`). So a number is
`p`-adically small when it is divisible by a high power of `p`. For `22/7 = 2 * 11 / 7`:

    |22/7|_2 = 1/2,   |22/7|_7 = 7,   |22/7|_11 = 1/11,   |22/7|_p = 1 for every other prime p.

The driver computes these with `abs` (`tools/adf/README.md:85`, `:367`). Its operand is an *idele*, the type of
section 3.9; here it is only the carrier of the rational 22/7, made by the command `idele` (`:81`).

```adf
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

Multiply the five answers: `(22/7) * (1/2) * 7 * (1/11) * 1 = 1`. This is the *product formula*: for every non-zero
rational, the product of all its absolute values is 1 (`refs/src/evertse-padic/evertsepadicnotes.txt:44-46`). The
real answer is printed as an interval, `3.1428... +/- 4.5e-19`, because a real number in the library is a ball;
chapter 2 says why.

### 1.2 Places

Ostrowski's theorem: every non-trivial absolute value on the rationals is equivalent to the usual one or to a
`p`-adic one for a prime `p` (`refs/src/evertse-padic/evertsepadicnotes.txt:81-83`). So the absolute values of `Q`,
up to equivalence, are indexed by the primes and one more index, written `inf` or `real`. These indices are the
*places* of `Q`. The driver names a place by a prime in decimal or by the word `real`
(`tools/adf/README.md:239-242`). A place that is not one is refused:

```adf
> project 22/7 with real 2 7 11
real: 3.142857142857142857 +/- 2.4e-19; 2: 22/7; 7: 22/7; 11: 22/7
> project 22/7 with 6
error: DOMAIN
```

`project` (`tools/adf/README.md:72`, `:222-271`) shows a value at a list of places. At each prime the rational
`22/7` is itself; at `real` it is a ball around it. 6 is not a prime: `error: DOMAIN`.

### 1.3 The p-adic integers as digits

Completing `Q` for `|.|_p` gives the field `Q_p` of `p`-adic numbers; its elements with `|x|_p <= 1` form the ring
`Z_p` of `p`-adic integers (`refs/src/evertse-padic/evertsepadicnotes.txt:273`). Every element of `Z_p` is a unique
series `a_0 + a_1 p + a_2 p^2 + ...` with digits `a_k` in `{0, ..., p-1}`
(`refs/src/evertse-padic/evertsepadicnotes.txt:332-336`; `refs/src/tate-kudla/kudla-1.txt:77-89`). Read the digits
from the right, as for an integer that never ends on the left. Knowing the first `k` digits is knowing `x` modulo
`p^k`; the library writes this `c + O(p^k)`.

`-1` in `Z_5` has all digits 4, since `1 + 444...4` (in base 5) carries forever. `1/3` lies in `Z_5`, because 3 is
not divisible by 5. A *local ball* `[p=5: c + O(5^k)]` is the set `c + 5^k Z_5`, one place only (conventions 9.2,
`lball_v`):

```adf
> show [p=5: -1 + O(5^4)]
[p=5: 624 + O(5^4)]
> show [p=5: 1/3 + O(5^6)]
[p=5: 10417 + O(5^6)]
```

The driver prints the centre `c` as the representative in `[0, 5^k)`. In base 5, `624` is `4444` and `10417` is
`313132`: the four last digits of `-1`, and the six last digits of `1/3 = ...3131313132`. (The base-5 digits were
computed in Python for this guide, `lanes/docs-guide/numbers.md`; the driver prints no digits, see "Library
demands" in the lane report.) The digits of `1/3` repeat, as the decimal digits of a rational do; check: `3 *
10417 = 31251 = 2 * 5^6 + 1`, so `3 * 10417` is 1 modulo `5^6`.

### 1.4 Zhat: all primes at once

An integer known modulo 1000 is known by its last three decimal digits: `x = 146 mod 1000` says `x` ends in `146`.
By the Chinese remainder theorem, `1000 = 8 * 125`, and this is the same as knowing `x mod 8` and `x mod 125`, a
condition at the prime 2 and one at the prime 5. It says nothing at the other primes.

A *profinite integer* is an integer known modulo every `N`, consistently; the set of them is `Zhat`, the
projective limit of the rings `Z/NZ` (`docs/SPEC.md` section 2; `refs/src/hertogh-thesis/thesis.txt:231-233`). It
is the product of the rings `Z_p` over all primes. A *finite ball* `a + N Zhat` is the set of profinite numbers
that are `a` modulo `N`; the driver writes it `a mod N`. Seen at the primes, it is one `p`-adic ball at each prime
that divides `N`, and all of `Z_p` at every other prime (`docs/SPEC.md` section 2):

```adf
> show 146 mod 1000
(* ; 146 mod 1000)
> project 146 mod 1000 with 2 3 5 7
2: 2 + O(2^3); 3: 0 + O(3^0); 5: 21 + O(5^3); 7: 0 + O(7^0)
```

`2 + O(2^3)` is `146 mod 8`, `21 + O(5^3)` is `146 mod 125`, and `0 + O(3^0)` is the whole of `Z_3`: no condition.
The canonical text of a finite ball is `(* ; a mod N)`; the `*` stands for "no real coordinate" (conventions 9.4).

### 1.5 The adeles

An *adele* of `Q` is a family `(x_real, x_2, x_3, x_5, ...)`, a real number and one `p`-adic number at every
prime, such that `x_p` lies in `Z_p` for all but finitely many `p`. The set of them is the *restricted product*
`A = R x prod' Q_p` (`refs/src/tate-kudla/kudla-1.txt:93-96`, with the footnote at `:109`; the general form,
`refs/src/milne-cft/CFT.txt:9370-9376`). A rational `q` is the adele `(q, q, q, ...)`, the same number at every
place; this embeds `Q` in `A` as a discrete subring (`refs/src/tate-kudla/kudla-1.txt:114-115`).

The library stores an adele to finite precision as a pair

    ( real ball ; finite ball )

with the finite ball `a + N Zhat` covering all primes at once (`docs/SPEC.md` section 2). Two pictures help.

- **A thread.** A rational is one number at every place. `22/7` passes through `3.142857...` on the real line
  and through the point `22/7` of each `Z_p` and `Q_p`. It is exact: `project 22/7` printed `22/7` at every
  prime above.
- **A tube.** A finite ball `146 + 1000 Zhat` is a set: a small ball at 2 and at 5, and everything at every
  other prime. It contains many threads, for example the integers `146 + 1000 k`. It does not contain `22/7`,
  although `7 * 146 = 1022` is 22 modulo 1000: at the prime 7, `22/7` is not in `Z_7`.

```adf
> contains 1146 with 146 mod 1000
true
> contains -854 with 146 mod 1000
true
> contains 22/7 with 146 mod 1000
false
```

An adele with a real ball as well is a narrower tube. If the real ball is shorter than 1000, it holds at most one
rational, and `reconstruct` finds it (`tools/adf/README.md:65`, `:91-95`; `docs/SPEC.md` 9.2):

```adf
> show (146.2 +/- 0.5 ; 146 mod 1000)
(146.2 +/- 0.51 ; 146 mod 1000)
> reconstruct (146.2 +/- 0.5 ; 146 mod 1000)
146
> reconstruct (3.14159 +/- 0.00001 ; 146 mod 1000)
error: NO_SOLUTION
```

The second tube holds no rational at all: `error: NO_SOLUTION`, a proved answer. That is the discreteness of `Q`
in `A` at work (`docs/SPEC.md` section 2, fact 2; `docs/proofs/quotient.md` Proposition 11).

Where does `22/7` come from, then? From a weaker question. "Which fraction `n/d` with `|n| <= 30` and
`0 < d <= 10` satisfies `n = 146 d` modulo 1000?" uses the residue modulo 1000 only and asks nothing at 7. That is
`recover` (`tools/adf/README.md:71`, `:184-198`; `docs/SPEC.md` 9.2, second item):

```adf
> recover 146 mod 1000 with 30 with 10
22/7
> recover 146 mod 1000 with 100 with 100
error: NOT_UNIQUE
```

With the larger box two fractions qualify, and the answer is `error: NOT_UNIQUE`.

### 1.6 Three facts to keep in view

`docs/SPEC.md` section 2 lists them; each has a proof in `docs/proofs/`.

1. `A` has zero divisors. In `Z_p` the equation `x^2 = x` has only the solutions 0 and 1; in `Zhat` the choice
   is free at every prime. Modulo 30 there are `2^3 = 8` solutions, one for each choice at 2, 3 and 5.
2. `Q` is discrete in `A`, and the quotient `A/Q` is compact (`docs/proofs/quotient.md` Corollary 4). Section 3.12
   computes in `A/Q`.
3. The invertible adeles, the ideles, need their own type with their own precision (section 3.9). No finite ball
   of positive radius is invertible: at a prime outside its modulus it contains 0 (`docs/SPEC.md` 4.5).

The solver `roots` (section 3.7) shows the first fact at one prime at a time: `x^2 - x` has the coefficients
`0 -1 1`.

```adf
> roots 0 -1 1 with 2 with 4
0 mod 2^4; 1 mod 2^4
> roots 0 -1 1 with 3 with 4
0 mod 3^4; 1 mod 3^4
```

## 2. Enclosures and statuses

### 2.1 A ball is a set that contains the answer

A real number to finite precision is stored as a ball `[m +/- r]`, the interval from `m - r` to `m + r`. An
operation on balls gives a ball that contains the result of the exact operation for every choice of points in
the input balls (`refs/src/flint-3.0.1/arb.rst:6-12`; the library's real part is FLINT's `arb`). This is an
*enclosure*: the true value is certainly inside, and the radius says how well it is known. The same holds for the
finite part: `a + N Zhat` contains the true finite part.

The decimal `0.1` has no finite binary expansion, so a real ball read from `0.1` at 64 bits is a small interval
around it. Its printed radius is rounded up, so the printed interval contains the stored one (conventions 9.5).
`prec` sets the bits of the real coordinate and `digits` the printed significant digits
(`tools/adf/README.md:88-89`).

```adf
> show (0.1 ; 0)
(0.099999999999999999995 +/- 5.9e-21 ; 0)
> show (0.5 ; 0)
(0.5 ; 0)
> prec 200
> show (0.1 ; 0)
(0.1 +/- 1.3e-61 ; 0)
```

`0.5` is exact in binary and prints with no radius.

### 2.2 The rules for finite balls

With `gcd` of rationals meaning the positive generator of the group they generate (`docs/SPEC.md` 4.3):

    (a + N Zhat) + (b + M Zhat)  =  (a + b) + gcd(N, M) Zhat
    (a + N Zhat) * (b + M Zhat)  is inside  a b + gcd(a M, b N, N M) Zhat

Both rules are tight: no smaller ball contains every result (`docs/proofs/precision.md`). The examples of SPEC 4.3:

```adf
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

Multiplying by the exact 12 gains precision at 2 and 3; dividing by 3 loses one digit at 3; a radius may be a
fraction. The last line is a warning: the two operands are read as independent points of the same ball, so
`x - x` is a ball around 0, not 0 (SPEC 4.3).

Radii are ordered by divisibility, not by size: `R Zhat` lies inside `S Zhat` exactly when `R/S` is an integer
(SPEC section 2). The three set predicates and the comparison of points (`tools/adf/README.md:61-64`, `:109-114`):

```adf
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

`compare` answers `undecided` when the two balls meet: the two unknown points may or may not be equal.

### 2.3 Precision at a prime: O(p^k)

At a prime, the precision is the exponent `k` of `p^k Z_p`. `c + O(5^8)` says: the first 8 base-5 digits are
`c`, the rest are unknown. At the real place `prec` counts bits of the real ball; at a prime it is the requested
absolute `p`-adic precision of a result (`tools/adf/README.md:249-253`). An input ball keeps its own precision
when that is smaller.

```adf
> prec 8
> exp_at 5 with 5
5: 349831 + O(5^8)
> exp_at (* ; 5 mod 25) with 5
5: 6 + O(5^2)
> exp_at 5 with real
real: 148 +/- 1.1
```

`exp(5)` in `Q_5` is a 5-adic number; asked to 8 digits, the answer is `349831 + O(5^8)`. The ball
`5 + 25 Zhat` gives only 2 digits, whatever `prec` is. At `real` the same `prec 8` means 8 bits, so `e^5` is
printed as `148 +/- 1.1`.

### 2.4 Statuses

A function either returns a value that satisfies its contract, or a *status* that says why not. The driver
prints the status as `error: NAME` (`tools/adf/README.md:381-384`). The status codes and their meaning are in
`docs/conventions.md` 3.1. They fall into four classes (`docs/ux/README.md`, "One status language for all
four"):

| Class | Statuses | What it means for you |
|---|---|---|
| open | `NOT_DETERMINED`, `UNIT_NOT_CERTIFIED`, `NEEDS_SPLIT`, `NOT_UNIQUE` | undecided; more data may decide it |
| proved no | `DOMAIN`, `NO_SOLUTION`, `NOT_UNIT` | proved: no point of the input has an answer |
| budget | `LIMIT` | a size bound was reached |
| not mathematics | `PARSE`, `UNSUPPORTED` | not a sentence of the grammar; not in version 1 |

For an open status, more precision, a smaller ball, a split of the input or a smaller search box may give an
answer. For a proved status, nothing will.

One line for each status that the driver can produce:

```adf
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

In order: a finite ball cannot certify a unit (SPEC 4.5); the ball `2 Z_2` meets the domain of `sin` at 2 and its
complement; two fractions in the box; 2 is outside the domain of `sin` at 2; no fraction in the box; 0 has no
inverse; a precision above the bound `ADF_PRINT_EXP_MAX = 100000` (`tools/adf/README.md:422-435`); a fraction
with no denominator; the driver has no negation of a local ball (`:282-289`). `NEEDS_SPLIT` belongs to the
quotient functions of the library (`docs/conventions.md` 3.1, and the row "Quotient by Q" of 3.2); none of the 86
driver fixtures shows it.

### 2.5 NOT_DETERMINED is not DOMAIN

The series of `sin` at the prime 2 converges exactly on `4 Z_2`, the 2-adic integers divisible by 4
(`docs/SPEC.md` 9.3.2, `c = 2` at 2; `docs/proofs/functions.md` Proposition 6). Three inputs:

```adf
> prec 8
> sin_at 2 with 2
error: DOMAIN
> sin_at (* ; 0 mod 2) with 2
error: NOT_DETERMINED
> sin_at (* ; 0 mod 4) with 2
2: 0 + O(2^2)
```

- `2` lies outside `4 Z_2`: every point of the input is outside the domain, so the answer is the proved
  `DOMAIN`.
- `0 mod 2` is the ball `2 Z_2`. It contains 4, which is in the domain, and 2, which is not. The library does not
  pick one: `NOT_DETERMINED`. A smaller ball can change the answer.
- `0 mod 4` is the ball `4 Z_2`, inside the domain: an answer, `0 + O(2^2)`.

The rule is general: a proved status (`DOMAIN`, `NO_SOLUTION`, `NOT_UNIT`) is never returned because an enclosure
merely failed to prove the opposite (`docs/conventions.md` 3.1, last paragraph). The same split shows in square
roots at 2: the unit 3 is not a square in `Q_2`, while the ball `1 + 4 Z_2` holds squares (1, 9) and non-squares
(5):

```adf
> root_at 3 with 2 with 2 with 1
error: DOMAIN
> root_at (* ; 1 mod 4) with 2 with 2 with 1
error: NOT_DETERMINED
```

When a function looks at several places, its status is the largest of theirs in the order of conventions 3.1, and
the place it reports is the first place with that status (`docs/conventions.md` 3.3). The driver prints the status
but not the place (`tools/adf/README.md:253`).

## 3. The driver, by task

This chapter is a tour of the driver by what you want to do. Each section names the lines of
`tools/adf/README.md` (the whole manual) where its commands are specified. Each session starts with the defaults,
`prec 64` and `digits 20`, unless it sets them. The 86 fixtures `tests/driver/*.cmd`, with the expected lines in
`tests/driver/*.out`, are the full catalogue.

### 3.1 Reading and printing values

`show` parses a value and prints its canonical text; `print` is the same command; `type` names the kind
(`tools/adf/README.md:56-57`, `:740`). The driver reads the kinds of the value form (`docs/conventions.md` 9.2):
the exact rational, the finite ball, the adele, the complex adele, the unit coset, the idele, the idele class, the
local ball, the partial ball, the class modulo `Q`, the finite test function and the character
(`tools/adf/README.md:275-289`, `:867`, `:826`). A real test function is printed by its own command `rfun`
(`:849`); `show` refuses it.

```adf
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

`6/-4` is not a sentence (a denominator has no sign, conventions 9.1); `0012/18` is read and reduced. A centre is
reduced into `[0, N)`. The partial ball `{...}` lists its places in the canonical order, the real place first
(conventions 7). `[5 mod 6]` is the set of units congruent to 5 modulo 6, which is the same set as `[2 mod 3]`
(`docs/SPEC.md` R6).

### 3.2 Arithmetic

`add`, `sub`, `mul`, `neg` and `div` (`tools/adf/README.md:58-60`). Operands of different kinds are combined only
where the specification defines it (`:322-337`): an exact rational with a finite ball, an adele or a complex adele;
a finite ball with a finite ball; an adele with an adele; an adele with a complex adele. `div` divides by an exact
rational only, because no finite ball can be certified invertible (`:335-337`).

```adf
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

The last line is refused: a finite ball has no real coordinate, so its sum with an adele is not defined in
SPEC 4.1. `cap` replaces the radius `R` of a result by `gcd(R, C)`, a coarser ball, to keep numbers small
(`tools/adf/README.md:66`; `docs/SPEC.md` 4.4 item 3):

```adf
> cap 146 mod 1000 with 10
(* ; 6 mod 10)
> cap 146 mod 1000 with 7
(* ; 0 mod 1)
```

`gcd(1000, 10) = 10` keeps the last decimal digit; `gcd(1000, 7) = 1` keeps nothing, and the ball is all of
`Zhat`.

### 3.3 One place at a time: local and partial balls

`project X with PLACES` gives the partial ball of `X` over a list of places (`tools/adf/README.md:72`,
`:222-271`). Its line is a text of the driver, `place: value; place: value`, not the value form `{...}` of
section 3.1 (`:263-267`).

```adf
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

A centre of negative valuation prints as a rational (`1/5 + O(5^2)`). A finite ball has no real coordinate, and a
place named twice is refused (`:242-243`).

### 3.4 Functions at one place

`exp_at`, `log_at`, `sin_at`, `cos_at`, `sinh_at`, `cosh_at` evaluate a function at one place
(`tools/adf/README.md:73-74`, `:222-224`, `:463-478`). At a prime the series of `exp`, `sin`, `sinh` converge on
`p Z_p` (on `4 Z_2` at 2), and `log` on `1 + p Z_p` (`docs/SPEC.md` 9.3.2).

```adf
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

`log(-1) = 0` exactly at 2: the series is not injective on `1 + 2 Z_2` (SPEC 9.3.2). `cos` of the ball `4 Z_2` is
known to more digits than its input, `1 + O(2^3)`. 2 is not in `1 + 3 Z_3`: `DOMAIN`.

### 3.5 Roots and branches at a prime

There is no "positive" `p`-adic root. A root at a prime is chosen by a *seed*, the residue of the root's unit part
modulo `p` (at 2: the sign `1` or `-1`), and every answer carries its branch identifier in brackets
(`tools/adf/README.md:480-513`; `docs/SPEC.md` 9.3.3). `roots_at` lists all branches, `root_at` picks one.

```adf
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

`2` is a square in `Q_7` (`3^2 = 9 = 2 + 7`), with the branches 3 and 4 modulo 7. `-1` is a square in `Q_5` (`2^2 =
4 = -1 + 5`). `2` is not a square modulo 5: `DOMAIN`. `9` has the rational roots `3` and `-3`, which print exactly;
at 2 the label `[+1]` or `[-1]` is the unit modulo 4 (`-3` is 1 modulo 4), not the real sign
(`tools/adf/README.md:502`). A ball input gives a ball: `46 + 125 Z_5` has the square root `36 + O(5^3)` on branch 1
(check: `36^2 = 1296 = 46 + 10 * 125`).

Powers at a prime (`tools/adf/README.md:515-545`): `powrat_at` is `x^(e/n)` on a root branch, and `powunit_at`
is `u^s` for a principal unit `u` (in `1 + p Z_p`) and an exponent `s` in `Z_p`.

```adf
> powrat_at 9 with 5 with 3/2 with 3
5 [3]: 27
> powunit_at 6 with 5 with (* ; 2 mod 25)
5: 36 + O(5^3)
```

### 3.6 Functions at all places at once

`root X with N` takes the root at every place at once (`tools/adf/README.md:86`, `:547-581`). For a rational it
returns the rational root, and refuses a rational that has none: such a rational has no root at some place
(`docs/SPEC.md` 9.3.3; `docs/proofs/functions.md` Proposition 16). The sign `-1` picks the non-positive branch at
every place.

```adf
> root 8 with 3
2
> root 2 with 2
error: DOMAIN
> root (4 ; 9/4) with 2 with -1
(-2 ; -3/2)
> root (4 ; 2 mod 4) with 2
error: NOT_DETERMINED
```

The last adele has a finite part that is not one rational, so the root at all places is not decided
(`tools/adf/README.md:575-577`).

The series `exp`, `sin`, `sinh`, `cos`, `cosh` at all places (`tools/adf/README.md:87`, `:583-608`) apply only to
an adele whose finite part is exactly 0: their common finite domain contains no other rational and no ball of
positive radius (`docs/SPEC.md` 9.3.2, "On all primes at once").

```adf
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

The finite part of each answer is the exact `f(0)`, here 1. The rational 1 is outside the finite domain at 2
(`DOMAIN`); the ball `4 Zhat` meets it and leaves it (`NOT_DETERMINED`).

### 3.7 Polynomials: roots at a prime and real roots

`roots POLY with P with K` lists the roots in `Z_p` of an integer polynomial, given by its coefficients from the
constant term up; `realroots POLY` isolates its real roots (`tools/adf/README.md:69-70`, `:151-182`). Each root
at a prime is printed as `A mod P^K`, with the precision of its certificate; the word `none` is a complete empty
list (`docs/SPEC.md` 9.1).

```adf
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

`X^2 - 2` has two roots in `Z_7`, `X^2 + 1` two in `Z_5` and none in `Z_3`. `docs/SPEC.md` 9.2 warns that
local roots do not make a rational root: `(x^2 - 13)(x^2 - 17)(x^2 - 221)` has a root in `R` and in every `Q_p`,
and none in `Q`.

### 3.8 Reconstruction

`reconstruct` and `recover` were met in section 1.5 (`tools/adf/README.md:65`, `:71`, `:91-95`, `:184-198`).
`reconstruct` takes an adele, or a finite ball and a closed real interval as two more operands:

```adf
> reconstruct (0.33 +/- 0.01 ; 1/3 mod 1)
1/3
> reconstruct 146 mod 1000 with 0 with 999
146
> reconstruct 146 mod 1000 with -1000 with 1000
error: NOT_UNIQUE
> recover 3 mod 7 with 1 with 2
-1/2
```

`1/2` is `4` modulo 7 and `-1/2` is `3`; so `recover 3 mod 7` with `|n| <= 1`, `d <= 2` finds `-1/2`.

### 3.9 Ideles and idele classes

An *idele* is an invertible adele. The library stores it as `(x_real ; r * [c mod N])`: a real ball that excludes
0, a positive rational `r` (the *content*), and a *unit coset* `[c mod N]`, the profinite units congruent to `c`
modulo `N` (`docs/SPEC.md` section 5; conventions 9.2, 9.4). The norm is `|x_real| / r`; on the idele of a rational
it is 1, the product formula of section 1.1. An *idele class* `<t ; u>` is the class modulo `Q^*` of an idele
(`docs/SPEC.md` section 5). The commands are in `tools/adf/README.md:77-85` and `:346-377`.

```adf
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

`hull` is the smallest adele that contains the idele (`:82`); `unitof` is the other way, and succeeds only for an
adele that is certified invertible (`:83`), here the exact 3. `Log` is the Iwasawa logarithm of an idele; its
finite part is enclosed in `4 Zhat`, refined at named primes by `Log_refine`, and `Log_at` gives one place
(`tools/adf/README.md:610-653`; `docs/SPEC.md` 9.3.2):

```adf
> prec 5
> Log (1 ; 4 * [1 mod 9])
(0 ; 0 mod 4)
> Log_at (1 ; 4 * [1 mod 9]) with 3
3: 3 + O(3^2)
> Log_refine (1 ; 4 * [1 mod 9]) with 3 5
(0 ; 120 mod 180)
```

### 3.10 Symbols

The Legendre, Jacobi and Kronecker symbols, and the Hilbert symbol at one place (`tools/adf/README.md:655-676`).
The first operand may be an integer, an integral finite ball or a unit coset; a ball must determine the symbol.

```adf
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

`1 mod 4` holds 1 and 5, whose Kronecker symbols at 2 differ: `NOT_DETERMINED`. The Hilbert symbols of `(-1, -1)`
are `-1` at `real` and at 2, and `1` at 3 and at every other odd prime, where both entries are units
(`refs/src/hilbert-bristol/lecture19.txt:40-49`). Their product over all places is 1, as Hilbert's product formula
says (`refs/src/hilbert-bristol/lecture19.txt:89-91`).

### 3.11 The arithmetic catalogue

Binomial coefficients of a finite ball, powers with a profinite exponent, the Haar volume of a ball, and the
exponent of the cyclotomic action of a class (`tools/adf/README.md:677-702`).

```adf
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

`haar_volume` of `a + N Zhat` is `1/N`, with the Haar measure of `Zhat` equal to 1 (`docs/conventions.md` 6.2).

### 3.12 The quotient A/Q and the additive character

`A/Q` is compact (section 1.6). A class modulo `Q` is written `(m ; F) + Q`, a lift and the marker `+ Q`, or a
union of pieces (`tools/adf/README.md:723-770`). Translation by a rational changes the lift by that rational
exactly; `qreduce` rewrites a lift as a union of closed balls with real part near `[0, 1]`.

```adf
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

The *additive character* `psi` maps an adele to a complex number of modulus 1, and is 1 on `Q`. For an exact
`(m ; a)` it is `E(a - m)` with `E(t) = exp(2 pi i t)` (`tools/adf/README.md:774-805`; the sign convention is
`docs/conventions.md` 6.1). `psi_at` gives one local factor, `psi_phase` the exact angle.

```adf
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

`psi (0 ; 1/3) = E(1/3)`. The local factor at 3 carries the whole phase; at 5 it is 1, since `1/3` is a 5-adic
integer. The ball `0 + (1/2) Zhat` holds points with the phases `E(0) = 1` and `E(1/2) = -1`: the default `psi`
encloses both, the strict one refuses.

### 3.13 Dirichlet characters and Gauss sums

`char(q=q, n=n, s=...)` is the Dirichlet character of Conrey label `n` modulo `q`, times `|.|^s`; the driver
reduces it to the primitive character that induces it (`tools/adf/README.md:824-842`, `:893-915`;
`docs/conventions.md` 5.13).

```adf
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

`gauss` prints the parity `e`, the Gauss sum `tau` and the root number `W = tau / (i^e sqrt(C))`
(`tools/adf/README.md:828-829`).

### 3.14 Test functions on the adeles

The functions that Tate integrates are products `f_real(x_real) * f_fin(x_fin)` (`docs/SPEC.md` section 7). The
real factor is an `rfun`, a sum of terms `P(x) exp(-pi A x^2 + B x + C)`; the finite factor is an `ffun`, a
function of `x mod M` on `(1/D) Zhat` (conventions 9.2). `rfun_eval` evaluates a real factor
(`tools/adf/README.md:844-863`). The Gaussian `exp(-pi x^2)` is its own Fourier transform (`:877-891`).

```adf
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

`ffun(D=1, M=2; 1, 0)` is the indicator of `2 Zhat`, of volume 1/2. Its Fourier transform is half the indicator
of `(1/2) Zhat`, written with `D=2, M=1` (`tools/adf/README.md:865-875`). The finite and the real factor are
combined by `tensor_eval`, `tensor_integral` and the other commands of `:917-948`, and Poisson summation over `Q`
with a certified tail is `poisson` (`:950-961`):

```adf
> prec 144
> digits 10
> tensor_eval rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)) with ffun(D=1, M=1; (1) + (0)*i) with (0.5 ; 0 mod 2)
(0.4559381278 +/- 3.5e-11) + (0)*i
> poisson rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)) with ffun(D=1, M=1; (1) + (0)*i) with 20
(1.086434811 +/- 2.2e-10) + (0 +/- 1.1e-12)*i | (1.086434811 +/- 2.2e-10) + (0 +/- 1.1e-12)*i | NL=2 NR=2
```

`poisson` prints both sides of the sum formula, computed independently, and the two lattice cutoffs: here
`sum_n exp(-pi n^2)` on the left and the sum of the transform on the right, equal as they must be.

### 3.15 Tate integrals

Tate's thesis writes the completed zeta function as an integral over the ideles: for the Gaussian at the real
place and the indicator of `Zhat` at the finite places,

    integral = pi^(-s/2) Gamma(s/2) prod_p (1 - p^(-s))^(-1) = Lambda(s),   and   Lambda(s) = Lambda(1 - s)

(`refs/src/tate-warwick/tatesthesis_notes.txt:501-512`, `:518-530`; the local factors,
`refs/src/tate-kudla/kudla-1.txt:643-647`, the functional equation `:966-968`). The library computes it for a
Dirichlet character, with a certified width (`tools/adf/README.md:963-976`, `:1024-1054`). `char(q=1, n=1, ...)` is
the trivial character, whose integral is `pi^(-s/2) Gamma(s/2) zeta(s)`. At `s = 2` it is `pi^(-1) * 1 * pi^2/6 =
pi/6`, with Euler's `zeta(2) = pi^2/6` (`refs/src/tate-warwick/tatesthesis_notes.txt:111-115`).

```adf
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

`tate_vector` prints the vector integrated: the Gaussian and the indicator of `Zhat`. The value at `s = 2` is
`0.5235987756`, which is `pi/6`. The integral converges for `Re(s) > 1` only: `s = 1` is `DOMAIN`, and a ball that
straddles `Re(s) = 1` is `NOT_DETERMINED`. The operand `s` is written as a complex adele whose finite part is
ignored (`:968-970`). The last argument, 53, asks for each coordinate of the answer to be known to `2^-53`.

Beyond the half-plane, `tate_continue` gives the continuation and `tate_completed` the completed function
`Lambda` (`:1024-1035`); `tate_fe` prints both sides of the functional equation (`:1037-1054`):

```adf
> prec 160
> digits 10
> tate_continue char(q=1, n=1, s=(0) + (0)*i) with ((-1) + (0)*i ; 0) with 80
(0.5235987756 +/- 1.8e-12) + (0 +/- 2.3e-26)*i | continuation | s=(-1) + (0)*i
> tate_completed char(q=1, n=1, s=(0) + (0)*i) with ((0.5) + (0)*i ; 0) with 80
(-3.976966226 +/- 5e-10) + (0 +/- 2.1e-26)*i | completed | s=(0.5) + (0)*i
> tate_fe char(q=1, n=1, s=(0) + (0)*i) with ((0.5) + (0)*i ; 0) with 80
(-3.976966226 +/- 5e-10) + (0 +/- 2.1e-26)*i | (-3.976966226 +/- 5e-10) + (0 +/- 1.5e-27)*i | overlap=yes | s=(0.5) + (0)*i
```

The continuation at `s = -1` is again `pi/6`, because `Lambda(-1) = Lambda(2)`. `Lambda(1/2)` is printed twice
by `tate_fe`, once from each side, and `overlap=yes` says that the two enclosures meet.

The local factors of one place (`tools/adf/README.md:704-721`, `:987-1022`): the zeta factor
`(1 - p^(-s))^(-1)` at a prime and `pi^(-s/2) Gamma(s/2)` at `real`, and the local Tate integral, gamma and
epsilon factors:

```adf
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

At `s = 2` and `p = 2`: `(1 - 1/4)^(-1) = 4/3`; at `real`, `pi^(-1) Gamma(1) = 1/pi = 0.31831`. The local
integral at 2, with the uniformizer value `alpha = 1` and the trivial character, is the same `4/3`
(`tools/adf/README.md:997`), and its gamma factor is `(1 - 1/4) / (1 - 2^(2-1)) = -3/4` (`:1012-1013`). The
words `continuation S=...` begin every line of these commands (`:995-996`).

### 3.16 Dumps and loading

The value form of a real ball is decimal, so reading a printed value back can widen it (conventions 9.6). The
*dump form* keeps every bit, in hexadecimal fields (`tools/adf/README.md:67-68`, `:116-142`; conventions 10.1):

```adf
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

`rat 16 7` is 22/7 (hexadecimal 16 is 22); `fball g 92 3e8 1` is the global triple `A = 146`, `H = 1000`,
`d = 1`. In `adele 1 3333333333333333 -41 ccccccd -5f`, the 1 counts the real coordinates, then come the mantissa
and the exponent of the midpoint and of the radius (conventions 10.1): the midpoint of the ball read from `0.1` is
`3333333333333333` (hexadecimal) times `2^-65`. Loading a dump gives the stored value back bit for bit; its printed
text is an enclosure of it, as always. Test functions have dumps too (`tools/adf/README.md:978-985`).

## 4. The C interface in one program

The driver is a thin program over the public C interface (`tools/adf/README.md:8-13`); everything it does, a C
program can do. The interface is the set of headers `include/adelefeld/*.h`, all included by
`include/adelefeld.h`. The program `docs/examples/hello.c` builds the finite ball `146 + 1000 Zhat` from its raw
triple, makes it the finite part of an adele, adds a second adele read from text, reconstructs the rational of
the sum, projects the sum to the prime 5, and takes a square root there on branch 1. It prints every status with
`adf_status_str` and every value with a printer of `adelefeld/text.h`, and frees everything.

<!-- check: file docs/examples/hello.c -->
```c
/* docs/examples/hello.c: one program through the C interface of adelefeld (docs/GUIDE.md, chapter 4).

   It builds the finite ball 146 + 1000 Zhat from its raw triple, makes it the finite part of an
   adele with a real ball, adds a second adele read from text, reconstructs the rational of the sum,
   projects the sum to the prime 5 and takes a square root there on a named branch. Every value is
   printed with the text functions of adelefeld/text.h, every status with adf_status_str, and every
   value and string is freed.

   Compile and run from the repository root (docs/examples/README.md):

       make -j2
       cc -std=c11 -Wall -Wextra -Werror -Iinclude docs/examples/hello.c build/libadelefeld.a \
          -lflint -lgmp -lm -o build/hello
       ./build/hello

   The contracts used, by header: fball.h (adf_fball_set_fmpz3), adele.h (adf_adele_set_arb_fball,
   adf_adele_add), text.h (the parsers adf_x_set_str and printers adf_x_get_str), recon.h
   (adf_adele_reconstruct), lball.h (adf_lball_set_fball), lroot.h (adf_lball_root_count,
   adf_lball_root_seed), place.h (adf_place_prime), status.h (adf_status_str), common.h
   (adf_str_free). */

#include <stdio.h>
#include <string.h>

#include <adelefeld.h>

/* Print a label, then a string from a printer of text.h, then free the string. */
static void
put(const char * label, char * s)
{
    printf("%-26s %s\n", label, s != NULL ? s : "(printer refused the value)");
    adf_str_free(s);
}

int
main(void)
{
    const slong prec = 64; /* bits of the real coordinate */
    const slong digits = 10; /* significant digits of the real-ball printer */
    size_t len;
    int status;

    fmpz_t A, H, d;
    arb_t r;
    adf_fball_t f;
    adf_adele_t x, y, z;
    adf_rat_t q;
    adf_place_t five;
    adf_lball_t z5, w;
    ulong count;
    const char * text = "(-100 +/- 0.001 ; -100)";

    fmpz_init_set_ui(A, 146);
    fmpz_init_set_ui(H, 1000);
    fmpz_init_set_ui(d, 1);
    arb_init(r);
    adf_fball_init(f);
    adf_adele_init(x);
    adf_adele_init(y);
    adf_adele_init(z);
    adf_rat_init(q);
    adf_lball_init(z5);
    adf_lball_init(w);

    /* 1. The finite ball (A + H Zhat)/d = 146 + 1000 Zhat (fball.h, adf_fball_set_fmpz3). */
    status = adf_fball_set_fmpz3(f, A, H, d);
    printf("%-26s %s\n", "set_fmpz3:", adf_status_str(status));
    put("f =", adf_fball_get_str(&len, f));

    /* 2. The adele x = (146.25 +/- 0.25 ; f). The real ball is exact in binary: 585/4 with radius 1/4. */
    arb_set_si(r, 585);
    arb_div_si(r, r, 4, prec);
    arb_add_error_2exp_si(r, -2);
    status = adf_adele_set_arb_fball(x, r, f);
    printf("%-26s %s\n", "set_arb_fball:", adf_status_str(status));
    put("x =", adf_adele_get_str(&len, x, digits));

    /* 3. The adele y read from its value form (text.h, adf_adele_set_str; NULL means default limits). */
    status = adf_adele_set_str(y, text, strlen(text), prec, NULL);
    printf("%-26s %s\n", "adele_set_str:", adf_status_str(status));
    put("y =", adf_adele_get_str(&len, y, digits));

    /* 4. The sum, coordinate by coordinate (adele.h, adf_adele_add; void, no status). */
    adf_adele_add(z, x, y, prec);
    put("z = x + y =", adf_adele_get_str(&len, z, digits));

    /* 5. The rationals inside z (recon.h): 46 + 1000 k in [45.999, 46.501]: only k = 0. */
    status = adf_adele_reconstruct(q, z);
    printf("%-26s %s\n", "reconstruct:", adf_status_str(status));
    if (status == ADF_OK)
        put("q =", adf_rat_get_str(&len, q));

    /* 6. The finite part of z seen at the prime 5 (lball.h): 46 + 125 Z_5. The field fin of
       adf_adele_struct is part of the documented layout (adele.h, conventions 5.5). */
    status = adf_place_prime(&five, 5);
    printf("%-26s %s\n", "place_prime 5:", adf_status_str(status));
    status = adf_lball_set_fball(z5, five, &z->fin);
    printf("%-26s %s\n", "lball_set_fball:", adf_status_str(status));
    put("z at 5 =", adf_lball_get_str(&len, z5));

    /* 7. Square roots at 5 (lroot.h). 46 = 1 mod 5 is a square modulo 5, so there are two branches,
       named by the residue of the root modulo 5: 1 and 4. The precision argument 3 asks for 5^3. */
    status = adf_lball_root_count(&count, z5, 2);
    printf("%-26s %s, %lu branches\n", "root_count:", adf_status_str(status), (unsigned long) count);
    status = adf_lball_root_seed(w, z5, 2, 1, 3);
    printf("%-26s %s\n", "root_seed seed 1:", adf_status_str(status));
    if (status == ADF_OK)
        put("sqrt(z) at 5, branch 1 =", adf_lball_get_str(&len, w));

    /* 8. A refusal: 2 is not a square modulo 5, so no point of 2 + 125 Z_5 has a square root. */
    fmpz_set_ui(A, 2);
    fmpz_set_ui(H, 125);
    adf_fball_set_fmpz3(f, A, H, d);
    adf_lball_set_fball(z5, five, f);
    status = adf_lball_root_seed(w, z5, 2, 1, 3);
    printf("%-26s %s\n", "sqrt(2 + 125 Zhat) at 5:", adf_status_str(status));

    adf_lball_clear(w);
    adf_lball_clear(z5);
    adf_rat_clear(q);
    adf_adele_clear(z);
    adf_adele_clear(y);
    adf_adele_clear(x);
    adf_fball_clear(f);
    arb_clear(r);
    fmpz_clear(d);
    fmpz_clear(H);
    fmpz_clear(A);
    flint_cleanup();
    return 0;
}
```

Compile and run it from the root of the repository. The include path and the libraries are those of the
`Makefile` (`CPPFLAGS = -Iinclude`, `LDLIBS = -lflint -lgmp -lm`); `make` builds the static library only, so the
program links `build/libadelefeld.a`. It also compiles cleanly with `-Wpedantic` and with `clang`:

    make -j2
    cc -std=c11 -Wall -Wextra -Werror -Iinclude docs/examples/hello.c build/libadelefeld.a \
       -lflint -lgmp -lm -o build/hello
    ./build/hello

<!-- check: hello -->
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

Points to notice, each a rule of the interface:

- A value is initialised before use and cleared after (`adf_x_init`, `adf_x_clear`; `docs/conventions.md` 2.3).
  The types are arrays of one struct, as in FLINT, so they are passed without `&`.
- A function that can fail returns an `int` status; on any status but `ADF_OK` its output is untouched
  (`docs/conventions.md` 4.3). The ring operations of the additive types cannot fail and return `void`
  (conventions 3.2).
- A parser takes a pointer and a length, and `NULL` for the default limits (`adelefeld/text.h`, top comment). A
  printer returns a string allocated by FLINT, which is freed with `adf_str_free`.
- The finite ball and the adele cannot be added directly (SPEC 4.1); the program first makes the finite ball the
  finite part of an adele with `adf_adele_set_arb_fball`.
- The branch of the square root is a seed (here 1, the residue of the root modulo 5), not a choice of the library.

`docs/examples/README.md` repeats the compile line. `docs/api-m1.md` lists every function of milestone 1 with its
header and source; the later `docs/api-*.md` files hold the statements of the later milestones.

## 5. The Julia package in one page

The package `julia/Adelefeld` calls the same library through `ccall`. Its manual is
[`julia/Adelefeld/README.md`](../julia/Adelefeld/README.md). It loads `build/libadelefeld.so`, which
`sh tests/test_exports.sh` builds (`make` does not). Julia's bundled GMP lacks a symbol that the system FLINT
needs, so start Julia with the launcher, which preloads the system GMP
(`julia/Adelefeld/bin/adelefeld-julia`, top comment):

    sh tests/test_exports.sh
    julia/Adelefeld/bin/adelefeld-julia --project=julia/Adelefeld -e 'using Adelefeld; println(Rat(1)/3)'

Three results, the same as in sections 1.4 and 3.2:

<!-- check: julia -->
```julia
using Adelefeld
x = FBall(Rat(146), Rat(1000))
println(value_text(x))
println(value_text(add(x, Rat(22) / Rat(7))))
println(value_text(mul(x, Rat(7))))
```

<!-- check: julia-output -->
```text
(* ; 146 mod 1000)
(* ; 1044/7 mod 1000)
(* ; 1022 mod 7000)
```

## 6. The four faces

Four interfaces are built on the library as first slices, one for each vision of `docs/ux/README.md`. They are
static pages written by small programs that call the driver or the Julia package; nothing is served. They share
one status language: open, proved no, budget, not mathematics (section 2.4). Each README is its manual.

**The notebook, Adelica** (`ux/notebook/README.md`). A notebook is a Julia file of cells. Each value is drawn as
a *place strip*: the real ball, a column per interesting prime with its digits, and fog for unknown digits.
Statuses are cards, and branches are chips that write their seed into the cell. Run
`sh ux/notebook/run.sh`, then open `ux/notebook/out/a_roots.html` in a browser; `sh ux/notebook/replay.sh` on a
records file reruns every result against the driver.

**The Place Atlas** (`ux/atlas/README.md`). A map: the real line on top and a `p`-adic tree for each prime below
it, where the ball `a + p^k Z_p` is a node at depth `k`. A rational is a thread through all places. Build the
scenes with `sh ux/atlas/build_scenes.sh` and open `ux/atlas/atlas.html`; the scenes include the square roots of
9 and the eight idempotents modulo 30 of section 1.6.

**The Ledger** (`ux/ledger/README.md`). The unit of work is a claim: a statement, the driver lines that decide
it, the lines they must print, and the places it speaks about. `python3 ux/ledger/seal.py` runs the driver on
every claim and seals the ones that hold; `python3 ux/ledger/render.py` writes `ux/ledger/ledger.html`. The 86
driver fixtures are imported as claims.

**The Bench** (`ux/bench/README.md`). One question asked at many places: a wall with one row per place and one
column per input, each cell a well whose shape is its status class, and a *stop line* below which nothing is
claimed. `sh ux/bench/run.sh` sweeps and draws every experiment into `ux/bench/out/`, for example
`squares.html`: for which primes below 100 is 2 a square?

## 7. Reading the contracts

What the library promises is written in four kinds of file, from the general to the particular.

1. **`docs/SPEC.md`** says what is built and why, with the label of each statement: `[proved]` (with the proof
   that proves it), `[checked]`, `[design]`, `[quoted]` (with its source).
2. **`docs/conventions.md`** fixes the forms: canonical forms of every type (section 5), the status codes
   (section 3), the text grammar (sections 8 and 9), the dump form (section 10), the C conventions for bindings
   (section 12).
3. **The header** of each function. The top comment of `include/adelefeld/<x>.h` is the contract of its
   functions: the meaning, the statuses, the aliasing rules, the cost, and the proof or statement that backs
   each claim. `docs/api-*.md` hold the statements of each slice before code was written (for example
   `docs/api-1f9.md` for the symbols, `docs/api-5s.md` for the Tate integrals).
4. **The proofs** in `docs/proofs/`: `precision.md` (the tight rules), `policies.md`, `ideles.md`,
   `quotient.md` (reconstruction, `A/Q`), `functions.md` (series, roots, branches), `solvers.md`,
   `catalogue.md`, `analysis.md` (Fourier analysis and the Tate integral). Each records the review it received from
   a second reader.

**Finding the proof behind an answer.** Take `exp_at (* ; 5 mod 25) with 5`, which printed `6 + O(5^2)` in
section 2.3. The driver manual names the library call (`adf_sball_exp_at`, `tools/adf/README.md:223`). Its header
`include/adelefeld/rfunc.h` points to `adelefeld/lfunc.h` for the prime places; that header cites SPEC 9.3.2,
whose table says that `exp` preserves distances on `p Z_p` and cites `docs/proofs/functions.md` Propositions 6
and 10. The driver fixtures `tests/driver/*.cmd` hold the same examples with a comment on the reason of each
expected line.

**Reporting a wrong enclosure.** An enclosure is wrong when the true value lies outside the printed set, or when
a proved status (`DOMAIN`, `NO_SOLUTION`, `NOT_UNIT`) is returned for an input that has an answer. A useful
report has three parts: the driver lines (with `prec` and `digits`) and their output; the exact value, computed
independently (by hand, in exact rational arithmetic, or with another system), and why it is not in the printed
set; and the commit you built. A fixture `tests/driver/<name>.cmd` with the expected `.out` is the best form,
since it becomes a test. Work on the repository is tracked as issues in beads (`bd`, `CLAUDE.md` rule 5).

## 8. Glossary

| Term | Meaning |
|---|---|
| absolute value | a multiplicative map `\|.\|` to the reals `>= 0` with the triangle inequality, 0 only at 0 |
| `p`-adic absolute value | `\|p^m b/c\|_p = p^(-m)` for `b`, `c` prime to `p`; `\|22/7\|_7 = 7` |
| place | an absolute value of `Q` up to equivalence: a prime, or the real place `real` (`inf`) |
| Ostrowski's theorem | the places of `Q` are exactly the primes and the real place |
| product formula | `prod_v \|x\|_v = 1` for a non-zero rational `x`, over all places `v` |
| `Q_p` | the completion of `Q` for `\|.\|_p`; the `p`-adic numbers |
| `Z_p` | the `p`-adic integers, `\|x\|_p <= 1`; series `a_0 + a_1 p + ...` with digits in `0..p-1` |
| `v_p` | the valuation at `p`: the exponent `m` in `p^m b/c` |
| `O(p^k)` | "unknown beyond the first `k` digits": `c + O(p^k)` is the ball `c + p^k Z_p` |
| local ball | `[p=P: c + O(P^k)]`, a ball in `Q_p` at one prime |
| partial ball | a ball at each place of a finite list of places, `{inf: ...; p=5: ...}` |
| `Zhat` | the profinite integers: integers known modulo every `N`; the product of all `Z_p` |
| finite ball | `a + N Zhat`, written `a mod N`: a `p`-adic ball at the primes of `N`, `Z_p` elsewhere |
| radius | the `N` of `a + N Zhat`, a positive rational; radii are ordered by divisibility |
| adele | a real and a `p`-adic number at every prime, almost all in `Z_p`; stored `(real ball ; finite ball)` |
| restricted product | the construction of the adeles: families with almost all coordinates in `Z_p` |
| thread | a rational seen as an adele: the same number at every place |
| tube | a finite ball or adele of positive radius: a set of adeles, constrained at finitely many primes |
| complex adele | `(complex ball ; finite ball)`, an element of `C x A_f` (not the adeles of a number field) |
| idele | an invertible adele, stored as `(real ball ; r * [c mod N])` |
| content | the positive rational `r` of an idele |
| unit coset | `[c mod N]`, the profinite units congruent to `c` modulo `N` |
| idele class | an idele modulo `Q^*`, written `<t ; u>` |
| `A/Q` | the adeles modulo the rationals, a compact group; a class is `(m ; F) + Q` |
| additive character | `psi`, a map from `A/Q` to the complex numbers of modulus 1; `psi(0 ; 1/3) = E(1/3)` |
| ball | a set that certainly contains the true value: an interval `m +/- r`, or `a + N Zhat` |
| enclosure | the result ball contains the exact result for every point of the input ball |
| tight | no smaller ball of the same kind contains all results |
| `prec` | bits of a real ball; at a prime, the requested absolute `p`-adic precision |
| `digits` | the significant digits printed for a real ball |
| status | the code a function returns when it gives no value; printed `error: NAME` by the driver |
| `NOT_DETERMINED` | the input does not determine the answer; a smaller ball or more precision may |
| `DOMAIN` | proved: every point of the input is outside the domain, or the data break a rule of the type |
| `NO_SOLUTION`, `NOT_UNIQUE` | proved: no solution; at least two solutions |
| `NOT_UNIT`, `UNIT_NOT_CERTIFIED` | proved not invertible; not proved invertible |
| `NEEDS_SPLIT` | the answer is a union of several pieces and only one can be returned |
| `LIMIT`, `PARSE`, `UNSUPPORTED` | a size bound; a text outside the grammar; not implemented in version 1 |
| seed, branch | the residue that names one of several local roots; the root it names |
| reconstruction | finding the rationals inside an adelic ball (`reconstruct`) or a residue class (`recover`) |
| value form | the canonical text of a value, as `show` prints it |
| dump form | a lossless text of a value, with hexadecimal fields |
| test function | `f_real * f_fin`: an `rfun` (polynomial times Gaussian) times an `ffun` (a function of x mod M) |
| Tate integral | the integral over the ideles of a test function times a character; gives `Lambda(s)` |
