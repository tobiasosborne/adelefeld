# Adelefeld.jl

Adelefeld.jl is the Julia package of adelefeld, a C library on FLINT 3.0.1 that computes with adeles of the
rational numbers as a number type, with exact rationals and ball arithmetic. The package is a thin, safe binding:
every number is computed by the C library through `ccall`, and the Julia code only checks arguments, owns C
storage and copies text. It adds no arithmetic of its own (`docs/api-j.md:3`). Version 0.1.0, loaded by path
from this repository; no registry. The design is `docs/api-j.md`; what each step built is `docs/api-ja.md`.

## The words used on this page

- **Place.** A way of measuring the size of a rational number. The real place `inf` uses the absolute value
  `|x|`. Each prime `p` gives the p-adic absolute value `|x|_p = p^(-v_p(x))`, where `v_p(x)` is the exponent of
  `p` in `x` (`refs/src/baker-padic/padicnotes.txt:760-763`). Example: `v_5(50) = 2`, so `|50|_5 = 1/25`, and
  `|1/5|_5 = 5`. At `p`, a number is small when it is divisible by a high power of `p`.
- **p-adic numbers.** `Q_p` is the completion of `Q` for `|x|_p`, as `R` is the completion for `|x|`. Its
  integers `Z_p` are the numbers with `|x|_p <= 1` (`padicnotes.txt:1175-1180`); a p-adic integer is known to
  precision `p^k` when its residue modulo `p^k` is known.
- **Zhat.** The profinite integers: an integer known modulo every `N`, consistently. By the Chinese remainder
  theorem this is one p-adic integer for every prime at once (`refs/src/hertogh-thesis/thesis.txt:231-236`).
  The set `a + N Zhat`, for an integer `N >= 1`, holds the elements congruent to `a` modulo `N`. Example:
  `146 + 1000 Zhat` is `146 + 8 Z_2` at 2, `146 + 125 Z_5` at 5, and the whole of `Z_p` at every other prime.
  Rational `a` and `N` are allowed (SPEC 2 and 4.1).
- **Adele.** A pair `(x_inf ; x_f)` of a real number and a finite adele, where the finite adele has a p-adic
  coordinate at every prime, all but finitely many of them p-adic integers: a restricted product
  (`refs/src/milne-cft/CFT.txt:9372-9376`; SPEC 2). A rational `q` is the adele `(q ; q)`: the same number at
  every place.
- **Ball.** A set known to contain the exact value. A real ball `m +/- r` is the interval `[m - r, m + r]`. A
  finite ball is a set `a + N Zhat`, printed `a mod N`. A rational is a ball of radius 0.
- **Enclosure.** The contract of every operation: the result ball contains every exact result for exact inputs
  taken from the input balls. Example (SPEC 4.3): `(3 mod 12) + (5 mod 18)` is `2 mod 6`, because
  `3 + 12k + 5 + 18l = 8 + 6(2k + 3l)` runs through `2 + 6Z`, and no smaller ball contains all these sums.
- **Status.** Each operation that can fail returns one of eleven codes: `OK` or a reason such as `DOMAIN`
  (proved outside the domain, for example the text `1/0`) or `NOT_UNIQUE` (several candidates). The list is in
  step 5.

## Starting Julia: the launcher

Start Julia through `bin/adelefeld-julia`, from the repository root:

    make -j2 && sh tests/test_exports.sh      # builds build/libadelefeld.a, then build/libadelefeld.so
    env JULIA_NUM_THREADS=1 JULIA_PKG_OFFLINE=true \
        julia/Adelefeld/bin/adelefeld-julia --startup-file=no --project=julia/Adelefeld

`make` alone builds the static library; the shared object that the package loads is built by
`tests/test_exports.sh` (which also checks that every declared function is exported). The package loads
`build/libadelefeld.so` of the repository, or the file named by the environment variable `ADELEFELD_LIB`.

Why a launcher. Julia loads its own copy of `libgmp.so.10` at start-up. That copy lacks the symbol
`__gmpn_modexact_1_odd`, which the system FLINT needs, so loading `libadelefeld.so` into a plain Julia fails.
Setting `LD_PRELOAD` from inside Julia is too late. The launcher finds the system `libgmp.so.10` with
`ldconfig -p`, puts it first in `LD_PRELOAD`, and then runs `julia` with the arguments given
(`docs/api-j.md:80-87`; the script's own comment). Without the preload, `using Adelefeld` stops with an
`AdfLoadError` at the gate `dlopen (GMP symbol)`; its hint gives the `LD_PRELOAD` command and the path of the
launcher.

The environment variables. `JULIA_NUM_THREADS=1`: one Julia thread is the supported setting
(`docs/api-j.md:323-329`). `JULIA_PKG_OFFLINE=true`: the package has no dependencies outside the standard library,
so nothing is fetched. `ux/notebook/julia.sh` also sets `JULIA_DEPOT_PATH` to a private depot first and
`TMPDIR` to a private directory, which keeps a run from writing into a shared depot; the commands in
`lanes/docs-julia/numbers.md` do the same.

At load time the package checks, before it allocates any value: 64-bit words, the FLINT version, the size and
alignment of every C type it allocates, and the names and the number of the status codes (`docs/api-j.md`
section 2). A failed check is an `AdfLoadError`.

## A tutorial in eight steps

Each step below is a REPL session. The outputs were produced by `lanes/docs-julia/tutorial.jl`, which runs these
lines in this order in one Julia process (`lanes/docs-julia/numbers.md` has the command and the whole output).
Error lines show the message only; the REPL also prints a stack trace. Run the steps in order: later steps use
names defined earlier.

### Step 1. Rationals

`Rat` is the exact rational: a number in lowest terms with a positive denominator. It compares with Julia
integers and `Rational`s by value. Division by an exact zero and a zero denominator are errors (step 5).

@@BLOCK 1-rationals@@

### Step 2. Finite balls and the set predicates

`FBall(c, N)` is the ball `c + N Zhat`. It is printed `(* ; c mod N)`: the `*` stands in the slot of the real
place, which a finite ball does not have. The centre is reduced into `[0, N)`. The text `3 mod 12` is read by
`parse(FBall, ...)`. Arithmetic is tight (SPEC 4.3): the result is the smallest ball that contains every result.
Tight does not keep correlations: `x - x` is a ball around 0, because the two operands vary independently.

The set predicates answer questions about the sets (SPEC 4.2): `inside(a, b)` is true when the set `a` lies inside
the set `b` (first inside second), `overlaps` when they meet, `equal_set` when they are the same set.
`compare_points(a, b)` compares two unknown points, one in each ball: `EQUAL` only when both are exact and equal,
`DIFFERENT` when the sets are disjoint, `UNDECIDED` otherwise. `==` is not defined on balls; it throws an
`ArgumentError` that names `equal_set` and `compare_points`.

@@BLOCK 2-finite-balls@@

`1 mod 6` lies inside `1 mod 3`; `1 mod 6` and `4 mod 9` meet (at 13, for example); `7 mod 6` is the set
`1 mod 6`. Overlap is not transitive: `0 mod 2` overlaps `0 mod 1`, which overlaps `1 mod 2` (SPEC 4.2).

### Step 3. Adeles, real balls and `parse`

`Adele(q)` is the rational `q` as an adele: a real ball that encloses `q` at a working precision of 64 bits
(the keyword `prec`), and the exact finite ball `q`. The real ball has a binary midpoint, so `1/3` is not exact
there; the finite coordinate stays exact. The text form is `(real ; finite)`. `RealBall` and `ComplexBall` are
numerical balls on their own; they are not adeles. `CAdele` has a complex ball at the real place (SPEC 4.1:
the ring `C x A_f`, not the adeles of `Q(i)`).

@@BLOCK 3-adeles@@

The sum `u + v` adds coordinate by coordinate: `1/3 + [1, 2]` at the real place, and `1/3 + (0 + 2 Zhat)` in
the finite part. A printed real ball encloses the stored one: `1.5 +/- 0.0009765625` is stored exactly and printed
with two digits of radius, `0.00098`, which is rounded up. At 200 bits the stored radius of `1/3` is far below
`1e-21`, but a print with 20 digits rounds the midpoint, and the printed radius must cover that rounding.

### Step 4. Reconstruction

`reconstruct` finds the rationals that lie in a real interval and in a finite ball at the same time
(`include/adelefeld/recon.h:8-12`). For a ball `a + N Zhat` with `N > 0` the candidates form the progression
`a + N k`, for the integers `k` with `ceil((lo - a)/N) <= k <= floor((hi - a)/N)`. There is at most one when
`hi - lo < N`, and at least one when `hi - lo >= N`. For `N = 0` the candidate is `a` itself, if it lies in
`[lo, hi]`. The status is `OK` for exactly one candidate, `NO_SOLUTION` for none, and `NOT_UNIQUE` for several
(`recon.h:15-16`). `reconstruct(x::Adele)` takes the interval from the real ball of `x`
(`recon.h:54-57`); `reconstruct(f::FBall, lo, hi)` takes it from the two rationals (`recon.h:49-52`).

@@BLOCK 4-reconstruction@@

`(1.5 +/- 0.5 ; 0 mod 2)` asks for an even integer in `[1, 2]`: only 2. `52 mod 101` in `[-50, 50]` gives
`k = -1` and `52 - 101 = -49`. The interval `[-0.5, 3.5]` holds the two even integers 0 and 2; `[1.25, 1.75]`
holds none.

### Step 5. Statuses, `AdfResult`, `unwrap` and `AdfError`

Every operation has two forms. `try_name(...)` returns an `AdfResult`, with the fields `status`, `value`,
`where` and `details`; `value` is `nothing` unless the status is `OK`. `name(...)` and the operators return the
value, and throw an `AdfError` on every status other than `OK`. `unwrap(r)` turns a result into the value or the
error. No status is turned into a dummy value.

@@BLOCK 5-statuses@@

The eleven statuses, in the order of `include/adelefeld/status.h:17-28`:

| Status | Meaning |
|---|---|
| `OK` | the output holds the result, which satisfies the contract |
| `NOT_DETERMINED` | the inputs do not determine the quantity; more precision or a sharper method may help |
| `UNIT_NOT_CERTIFIED` | an invertible input was required; the enclosure does not prove it |
| `NEEDS_SPLIT` | the result is a union of pieces and one piece was asked for |
| `NOT_UNIQUE` | several candidates satisfy the problem |
| `NO_SOLUTION` | proved: no value satisfies the problem |
| `NOT_UNIT` | proved: the value is not invertible |
| `DOMAIN` | proved outside the domain; invalid raw data; context mismatch |
| `UNSUPPORTED` | a valid request that version 1 does not implement |
| `PARSE` | the text is not a sentence of the grammar |
| `LIMIT` | a resource limit was reached |

`AdfAbiError` is different: a C function returned a number that is not a status, which means a broken library,
not a mathematical answer.

### Step 6. Text and dumps

There are two text forms. The value form is what `show` prints: decimal, readable, and not lossless. A real ball
read back from its printed text encloses the original, but need not be the same ball (`docs/conventions.md`
9.6). The keyword `digits` (default 20) sets the significant digits of a real ball; in `show` it is the
`IOContext` key `:adf_digits`. The dump form is lossless: `load_dump(T, dump_text(x))` is identical to `x`
whenever the dump fits the load limits (`TextLimits`; the default admits the values of this page). A
dump starts `adf1 Q` (version 1, field Q), names the type, and writes integers in lower-case hexadecimal
(`docs/conventions.md` 10.1). In an adele dump the real ball is four numbers, as FLINT's `arb_dump_str` writes
it: midpoint mantissa and exponent, radius mantissa and exponent (`refs/src/flint-3.0.1/arb.rst:285-291`). The
finite ball `g A H d` is the set `(A + H Zhat)/d` in the global storage form (SPEC 4.1).

`readvalue` reads text of any of the four types of this package; the C library decides the type from the syntax
alone (`include/adelefeld/text.h:137-143`). A text of a type that the package does not wrap yet, such as the
unit coset `[5 mod 6]`, gives `UNSUPPORTED` with the kind in `details`.

@@BLOCK 6-text-and-dumps@@

The dump of `u` and the dump of `parse(Adele, s)` differ in the radius: reading the printed text widened the
ball. The finite part `g 1 0 3` (the point 1/3) is the same in both.

### Step 7. Modulus contexts and the local backend

A finite ball can be stored in two ways with the same set (SPEC 4.1). The global form keeps the integers
`A, H, d` of `(A + H Zhat)/d`. The local form keeps `d` and the residues of `A` modulo pairwise coprime blocks
`q_1, ..., q_k` whose product is the modulus `K` (the Chinese remainder theorem joins them). The blocks live in
a modulus context, `ModCtx`. A context is made by exactly one of five construction families:

| Keyword | Blocks | C constructor |
|---|---|---|
| `blocks = [q1, ..., qk]` | as given; each `q >= 2`, pairwise coprime, else `DOMAIN` | `adf_modctx_new_blocks` |
| `prime_powers = [p => e, ...]` | `p^e` for each pair; each `p` prime | `adf_modctx_new_prime_powers` |
| `modulus = K` | one block `K` when `2 <= K < 2^64` | `adf_modctx_new_fmpz` |
| `factorial = n` | the prime powers of `n!` | `adf_modctx_new_factorial` |
| `primorial_power = (n, e)` | `p^e` for each prime `p <= n` | `adf_modctx_new_primorial_pow` |

`to_local(x, ctx)` stores `x` in the context, exactly. A ball `c + R Zhat` with `R > 0` can be stored when `K/R`
and `c K/R` are integers; otherwise the status is `DOMAIN` (`include/adelefeld/modctx.h:145-153`).
`enclose_local(x, ctx)` returns the best enclosure of `x` among the balls the context can store, and
`details.lost` says whether the set grew (`modctx.h:155-164`). `to_global` copies the set back into the global
form. The printed text does not show the storage form; `is_local` and `context` do.

@@BLOCK 7-modctx@@

`1 mod 5` can be stored in the context of `6! = 720`: `720/5 = 144` is an integer. `1 mod 7` cannot: `720/7` is
not an integer. In the context `[8, 9]` (`K = 72`) the smallest local ball around `1 mod 5` is `0 mod 1`, all of
`Zhat`; around `1 mod 32` it is `1 mod 8`, larger than the input, so `lost` is true. The blocks 4 and 6 are not
coprime.

### Step 8. Housekeeping

Each value owns C storage, which a finalizer frees when Julia's garbage collector collects the value.
`close(x)` frees it at once; a second `close` does nothing, and a closed value is refused before any C call.
A value that lives in a context keeps that context alive: `finalize(ctx)` drops the context's own claim, after
which `ctx` is refused as an argument, while the values made in it stay valid. `Adelefeld.diagnostics()`
(not exported) returns the load state, the library path, the number of live values, the number of errors caught
inside finalizers (each one a leak; it should be 0), and the counts of live and freed contexts.

@@BLOCK 8-housekeeping@@

## The exported names

Every name of the `export` lines of `src/Adelefeld.jl`, 63 in all. The description is the first sentence of the
docstring, shortened. A name marked `*` has no docstring of its own; its line is taken from the source file
named, or from the docstring of its `try_` form.

| Name | What it is |
|---|---|
@@EXPORTS@@

## What the package does not do yet

The package wraps the first part of the C library: exact rationals, finite balls, adeles, complex adeles, real
and complex balls, text, dumps, reconstruction and modulus contexts. The C library and its driver `build/adf`
(manual: `tools/adf/README.md`) do much more. The order in which the rest arrives is `docs/api-j.md` section 9:

- J-b, steps 2 to 4: scaled values and `cap` (the scaled residue policy of SPEC 4.4); places and local balls
  `LBall` (a ball in `Q_p` alone, with its p-adic digits); partial balls `SBall`, `project`, and the local text
  and dumps.
- J-c: unit cosets, ideles and idele classes, powers; the quotient `A/Q` (`QClass`); the additive character psi;
  Dirichlet characters and their evaluation.
- J-d: functions at one place and at all places, symbols (Legendre, Jacobi, Kronecker, Hilbert), the local
  factors, functions on the adeles and their Fourier transform, Poisson summation, Tate's local and global zeta
  integrals, their continuation and the functional equation.
- J-e: an optional bridge to Nemo (exact copies of `fmpq`, `arb` and `acb` values).
- J-S: the solvers of the driver, `roots` (roots of a polynomial at a prime), `realroots` and `recover`.

Also not there: a power `x^n` of a ball (refused with an `ArgumentError` until `pow` lands), division by a ball
or by an adele, `==` and an ordering on balls, numerical set predicates on real and complex balls (they arrive
with `SBall`), more than one Julia thread, and public mutating arithmetic.

## Using the driver from Julia today: the notebook slice

`ux/notebook` (its `README.md`) is a notebook of Julia cells built on this package. It calls the package for
rationals, finite balls, adeles and reconstruction, and the driver `build/adf` for what the package does not
have yet: `roots` and `root` of a polynomial at a prime (driver `roots_at`, `root_at`), `recover`, and
`project`. It writes a static HTML page and a records file, and `replay` reruns every driver line and compares.

    sh ux/notebook/run.sh        builds build/adf and build/libadelefeld.so if missing, writes ux/notebook/out/
    sh ux/notebook/test.sh       the notebook's own Julia tests

## Running the tests

From the repository root, after building `build/libadelefeld.so`:

    timeout 900 env JULIA_NUM_THREADS=1 JULIA_NUM_PRECOMPILE_TASKS=1 JULIA_PKG_OFFLINE=true \
        julia/Adelefeld/bin/adelefeld-julia --startup-file=no --project=julia/Adelefeld \
        -e 'using Pkg; Pkg.test()'

@@PKGTEST@@

The five step testsets pass 904 + 1015 + 1649 + 1313 + 488 = 5369 tests. The testset `C call counters` (43 tests)
runs in a separate Julia process against a fixture library that counts C calls, and prints its own summary.
The run took 65 seconds of wall time on this laptop (`lanes/docs-julia/numbers.md`, section 3).

`tests/test_julia.sh` runs the same `Pkg.test` as one of its blocks. The load-fault tests build small fixture
libraries with `cc` and load each in its own Julia process (`test/load_fault.jl`).

## Files named on this page

| File | What it holds |
|---|---|
| [`docs/api-j.md`](../../docs/api-j.md) | the design of the binding; section 9 is the order of the steps |
| [`docs/api-ja.md`](../../docs/api-ja.md) | what each step built; the statements its tests check |
| [`docs/SPEC.md`](../../docs/SPEC.md) | the specification: values 4.1, set predicates 4.2, arithmetic 4.3 |
| [`docs/conventions.md`](../../docs/conventions.md) | statuses (3), text (8, 9), dumps (10), the C interface (12) |
| [`include/adelefeld/status.h`](../../include/adelefeld/status.h) | status codes, point comparison |
| [`include/adelefeld/recon.h`](../../include/adelefeld/recon.h) | reconstruction |
| [`include/adelefeld/modctx.h`](../../include/adelefeld/modctx.h) | contexts, local backend |
| [`include/adelefeld/text.h`](../../include/adelefeld/text.h) | the parsers and printers of the value form |
| [`include/adelefeld/dump.h`](../../include/adelefeld/dump.h) | the dumpers and loaders |
| [`tools/adf/README.md`](../../tools/adf/README.md) | the manual of the driver `build/adf` |
| [`ux/notebook/README.md`](../../ux/notebook/README.md) | the notebook slice |
| [`bin/adelefeld-julia`](bin/adelefeld-julia) | the launcher |
| [`src/Adelefeld.jl`](src/Adelefeld.jl) | the module and its `export` lines |
| [`test/runtests.jl`](test/runtests.jl) | the tests run by `Pkg.test` |
| [`lanes/docs-julia/tutorial.jl`](../../lanes/docs-julia/tutorial.jl) | the tutorial as a script |
| [`lanes/docs-julia/numbers.md`](../../lanes/docs-julia/numbers.md) | commands and outputs |
