# adelefeld: conventions, version 0.1 (work package 0.4, part A)

Date: 2026-09-27. Status: **draft for review; nothing is frozen.** Written by the lane `m0-conventions`. Read
`SPEC.md` first. This document fixes, as a contract, what `PLAN.md` section 4 and 5 leave as a proposal: canonical
forms and storage invariants, naming, aliasing and ownership, status codes, the text grammar, and the rules of the
foreign-function interface. Two programmers who follow it should write interchangeable code.

How to read it:

- **DECISION (proposed) CV-nn**: a choice the specification leaves open, made here with a one-line reason. All are
  collected in section 13 for review.
- **PENDING (part B): lane**: a section that depends on a proof or source written by another lane. It contains
  exactly what `SPEC.md` already fixes, and nothing more.
- FLINT's own conventions are cited from the installed headers of FLINT 3.0.1 as
  `/usr/include/flint/<file>:<line>` (version: `/usr/include/flint/flint.h:94-97`). Where the headers are silent
  and only FLINT's documentation would settle a point, the statement is marked `[source pending: ...]`. Behaviour
  observed by a probe program against `libflint.so.18.0.1` is marked **[probed]**; the probe is described in
  `lanes/m0-conventions/report.md`.
- The reference implementation of the value form and of the dump form is `proto/text_grammar.py`, with its tests in
  `proto/test_text_grammar.py`; the golden vectors are in `tests/golden/` (section 11).

Contents: 1 FLINT conventions we follow; 2 naming and argument order; 3 status codes; 4 aliasing, ownership, outputs
after a status, invalid input, threads, contexts; 5 canonical forms and storage invariants; 6 signs, measures and
normalisations (stubs); 7 places; 8 text: general rules; 9 value form; 10 dump form; 11 golden vectors;
12 foreign-function interface; 13 table of decisions; 14 findings.

## 1. FLINT conventions we follow

| Convention | FLINT evidence |
|---|---|
| A type `x` has `x_struct`, `x_t` (an array of one struct), `x_ptr`, `x_srcptr` | `/usr/include/flint/arb_types.h:32-41` |
| `init` sets a valid value (zero), `clear` releases memory; every `init` is paired with one `clear` | `fmpz.h:69-70`, `fmpq.h:28-37` (`init` gives `0/1`), `arb.h:46-52` |
| `set(dest, src)`, `swap(a, b)`; swap exchanges contents in O(1) | `fmpz.h:151-152`, `fmpq.h:78-88`, `arb.h:139` |
| Outputs first, then inputs | `fmpz.h:361` (`fmpz_add(f, g, h)`), `arb.h:382` |
| Real working precision is the last argument of each operation, not a field of a context | `arb.h:382`, `arb.h:395` (`arb_add(z, x, y, prec)`, `arb_mul(z, x, y, prec)`) |
| A modulus context is passed explicitly as the last argument; values do not point to it | `fmpz_mod.h:74`, `fmpz_mod.h:109`; context init and clear `fmpz_mod.h:40-45`; `nmod_t` is a plain struct `flint.h:502-508` |
| A p-adic value keeps its own precision `N`; the context holds the prime | `padic.h:32-41`, `padic.h:68-80` |
| A canonical form has a predicate | `fmpq.h:117-118` (`fmpq_is_canonical`), `fmpz_mod.h:55` (`fmpz_mod_is_canonical`) |
| Strings are returned in memory from `flint_malloc` | `fmpz.h:349` (`fmpz_get_str`), `arb.h:168` (`arb_get_str`), `arb.h:1088` (`arb_dump_str`); allocator `flint.h:201-204` |
| Status of generic operations is a small `int`: `GR_SUCCESS 0`, `GR_DOMAIN 1`, `GR_UNABLE 2`, combined by bitwise or | `gr.h:98-101` |
| Header inline functions are also exported symbols (`FMPZ_INLINE` is `static __inline__` except in the one file that defines `FMPZ_INLINES_C`) | `fmpz.h:15-19`; **[probed]** `nm -D libflint.so` lists `fmpz_init`, `fmpq_init`, `arb_init`, `arb_swap` as `T` |

Two points where we deliberately differ:

1. **Values may point to a modulus context** (the local backend of `adf_fball` and the scaled policy, `PLAN.md`
   section 4), whereas FLINT's `fmpz_mod` elements do not. The reason is that a value's backend is part of the value
   (`SPEC.md` 4.1). The consequences are fixed in section 4.6.
2. **Status codes are not bit flags.** `SPEC.md` 10.3 asks for distinguishable statuses; a bitwise or of several
   meanings would not name one. The combination rule is a maximum (section 3.3). DECISION (proposed) CV-01: status
   codes are ordered integers combined by maximum; reason: one code per result, and a cheap, deterministic
   combination.

The FLINT documentation states the general aliasing rule (outputs may alias inputs unless stated otherwise)
[source pending: FLINT 3.0.1 documentation, section on aliasing]. We state our own rule in section 4.1 and do not
rely on FLINT's wording.

## 2. Naming, argument order, life cycle

### 2.1 Names

- Public symbols: `adf_<type>_<verb>` (`PLAN.md` section 2), e.g. `adf_fball_add`, `adf_idele_inv`.
  Types: `adf_<type>_struct`, `adf_<type>_t` (array of one), `adf_<type>_ptr`, `adf_<type>_srcptr`.
- Functions with a leading underscore, `_adf_<type>_<verb>`, are public but low level: they may take raw or
  non-canonical data and may forbid aliasing; each states its preconditions. As in FLINT.
- Functions at named places end in `_at` (`adf_adele_exp_at`); the all-places form has no suffix (`SPEC.md` 9.3.1).
- Macros: `ADF_` prefix, upper case. Status codes `ADF_<NAME>`.
- The verbs of set predicates are exactly `equal_set`, `overlaps`, `contains` (`SPEC.md` 4.2). DECISION (proposed)
  CV-02: there is no `adf_<type>_equal` for ball types; representation identity is `adf_<type>_identical` (same
  backend, context and fields); reason: `equal` would invite reading it as equality of points, which is undecided
  (`SPEC.md` 4.2).
- The comparison of points returns `int` with the fixed values `ADF_CMP_EQUAL = 0` (both exact and equal),
  `ADF_CMP_DIFFERENT = 1` (disjoint), `ADF_CMP_UNDECIDED = 2` (`SPEC.md` 4.2). It is not a status.

### 2.2 Argument order

Outputs; then optional output reports (`adf_place_t * where`, `int * lost`); then inputs; then integer parameters
(degree, limits, number of digits); then `slong prec`; then a context argument, if any. Example:

    int adf_adele_exp_at(adf_sball_t y, adf_place_t * where, const adf_adele_t x, const adf_places_t S, slong prec);

`prec` is the real working precision in bits, as in `arb` (`arb.h:382`). It is an argument of every operation that
computes a real or complex ball, including the parser of the value form (a decimal is read into an `arb` at `prec`).
It is never stored in a context (`PLAN.md` section 4). Operations that involve no real or complex ball take no
`prec`.

### 2.3 Life cycle

For every type `x`:

| Function | Contract |
|---|---|
| `adf_x_init(x)` | sets the init value of section 5 (a valid canonical value). Types that need a context take it: `adf_scaled_init(x, ctx)` |
| `adf_x_clear(x)` | releases all memory owned by `x`. After it `x` must not be used except by `init` |
| `adf_x_set(y, x)` | `y` becomes a copy (same backend, same context pointer). `y` may be `x` |
| `adf_x_swap(x, y)` | exchanges contents; O(1), no allocation; context pointers travel with the values |
| `adf_x_is_canonical(x)` | returns 1 if the storage invariant of section 5 holds, else 0. Never aborts |
| `adf_x_identical(x, y)` | representation identity (type, backend, context blocks, all fields) |
| `adf_x_get_str`, `adf_x_set_str` | value form (section 9) |
| `adf_x_dump_str`, `adf_x_load_str` | dump form (section 10) |

Functions that cannot fail return `void` (as `arb_add`). Functions that can fail return `int`, a status (section
3). Predicates return `int` 0 or 1.

## 3. Status codes

### 3.1 The list

DECISION (proposed) CV-03: the numeric values below, in increasing order of precedence; reason: the combination of
section 3.3 is then the maximum. `PLAN.md` section 4 gives the names; `SPEC.md` 10.3 the required distinctions.

| Code | Value | Exact meaning |
|---|---|---|
| `ADF_OK` | 0 | the output holds the result, which satisfies the function's contract (enclosure, exactness as stated) |
| `ADF_NOT_DETERMINED` | 1 | the inputs are valid, but the requested quantity is not fixed by the inputs at their precision (a valuation, a sign, a symbol, a root that the ball does not fix); more precision in the inputs may resolve it |
| `ADF_UNIT_NOT_CERTIFIED` | 2 | an invertible value was required and the enclosure does not prove invertibility (for example every additive ball of positive radius, `SPEC.md` 4.5) |
| `ADF_NEEDS_SPLIT` | 3 | the true result is a union of several pieces and the function may return one piece only (`SPEC.md` 6); exceeding a piece limit given as argument is `ADF_LIMIT` |
| `ADF_NOT_UNIQUE` | 4 | several candidates satisfy the problem (reconstruction, solving, `SPEC.md` 9.2) |
| `ADF_NO_SOLUTION` | 5 | proved: no value satisfies the problem |
| `ADF_NOT_UNIT` | 6 | proved: the value is not invertible (for example the exact 0) |
| `ADF_DOMAIN` | 7 | proved: every point of the input lies outside the domain of the function (a ball that meets both the domain and its complement gives `ADF_NOT_DETERMINED`); or a constructor, parser or loader received data that violates the type's invariant (zero denominator, composite prime, non-finite real ball). Reported with the place, where there is one |
| `ADF_UNSUPPORTED` | 8 | valid request that version 1 does not implement (a prime or modulus above one word where a word is required, a dump of an unknown version, a complex branch that is not specified) |
| `ADF_PARSE` | 9 | the text is not a sentence of the grammar of section 9 or 10 |
| `ADF_LIMIT` | 10 | a resource limit was reached: a limit of section 8.4, a piece limit given as argument, a size bound of an algorithm |

`ADF_NOT_DETERMINED` and `ADF_UNIT_NOT_CERTIFIED` say "not proved either way"; `ADF_NO_SOLUTION`, `ADF_NOT_UNIT` and
`ADF_DOMAIN` say "proved". A function never returns a "proved" code on the basis of an enclosure that merely fails
to prove the opposite.

### 3.2 Which functions may return which

| Class of function | Possible statuses |
|---|---|
| Ring arithmetic of `adf_rat`, `adf_fball`, `adf_adele`, `adf_cadele`, `adf_scaled` (add, sub, neg, mul, scale by exact rational, set predicates) | none: `void` |
| Division of `adf_rat` by an `adf_rat`; scaling by the inverse of an exact rational | `OK`, `NOT_UNIT` (divisor exactly 0) |
| Constructors from raw data (`_set_fmpz3`, `_set_arb_fball`, `adf_ucoset_set_fmpz2`, ...) | `OK`, `DOMAIN` |
| Parsers of the value form (`_set_str`) | `OK`, `PARSE`, `LIMIT`, `UNSUPPORTED`, `DOMAIN`, `NOT_DETERMINED` (only the sign conditions of section 9.3 at the requested `prec`) |
| Loaders of the dump form (`_load_str`) | `OK`, `PARSE`, `LIMIT`, `UNSUPPORTED`, `DOMAIN` |
| Conversion to the local backend or to a scaled context | `OK`, `DOMAIN` (the set is not representable there without loss), `UNSUPPORTED` (the context has no word blocks) |
| Conversion from tight to scaled | `OK` always; loss is reported in `int * lost` (`SPEC.md` 4.4: "says so") |
| Adele to idele, inversion of an adele-like value | `OK`, `UNIT_NOT_CERTIFIED`, `NOT_UNIT` |
| Unit coset, idele, idele class arithmetic | `OK`; `NOT_UNIT` only for an exact zero input where one is accepted |
| Functions at places (`_at`) and all-places functions (`SPEC.md` 9.3) | `OK`, `DOMAIN` (with place), `NOT_DETERMINED`, `NEEDS_SPLIT`, `UNSUPPORTED`, `LIMIT` |
| Quotient by `Q` | `OK`, `NEEDS_SPLIT`, `LIMIT` |
| Characters, Gauss sums, local factors | `OK`, `NOT_DETERMINED`, `DOMAIN` (a pole), `UNSUPPORTED` |
| Reconstruction and solvers | `OK`, `NO_SOLUTION`, `NOT_UNIQUE`, `NOT_DETERMINED` ("uniqueness not certified", `SPEC.md` 9.2), `LIMIT` |
| Integrals, Poisson summation | `OK`, `DOMAIN` (outside the stated half-plane or at a pole), `NOT_DETERMINED`, `LIMIT` |

A function documents its own subset; it may not return a code outside its class's row.

### 3.3 Combining statuses

Over several places (an all-places function, `f_at` over a set `S`, a family such as the Hilbert symbols of
`SPEC.md` 9.3.7), and over the steps of a composite function:

1. The combined status is the **maximum** of the statuses, in the numeric order of section 3.1.
2. The reported place is the first place, in the canonical order of places (section 7), whose status equals the
   combined status.
3. The result is `ADF_OK` exactly when every part is `ADF_OK`.

DECISION (proposed) CV-04: precedence by maximum, ties by canonical place order; reason: proved failures dominate
failures that more precision may repair, so a caller does not raise the precision in vain, and the reported place
does not depend on the order of evaluation.

The optional per-place variant (`SPEC.md` 9.3.1) returns one status per place in canonical order and the combined
status by the same rule.

## 4. Aliasing, ownership, state after a status, invalid input, threads, contexts

### 4.1 Aliasing

1. **Outputs may alias inputs of the same type**, for every public function without a leading underscore.
   `adf_fball_add(x, x, x)` is valid. DECISION (proposed) CV-05; reason: FLINT users expect it, and it is cheap for
   the global backend.
2. **Inputs may alias each other** freely (inputs are `const`).
3. **Two outputs of one function must not alias each other**, nor may an output alias a part of an input
   (`&x->fin` as output while `x` is an input). Undefined behaviour otherwise.
4. An output of one type never aliases an input of another type, except where a type contains the other and the
   function says so.
5. Underscore functions state their aliasing rules; by default they forbid aliasing of outputs with inputs.
6. A context argument is never an output and may be shared by any number of arguments.

Tests: every binary operation is tested with `(x, x, y)`, `(y, x, y)` and `(x, x, x)` (`PLAN.md` section 7,
"aliasing of arguments").

### 4.2 Ownership

- A value owns all memory it points to (its `fmpz` limbs, its `arb` mantissas, the residue array of a local
  `adf_fball`, the arrays of `adf_sball`, `adf_qclass`, `adf_ffun`, `adf_rfun`), except the modulus context.
- A value **borrows** its modulus context (`const adf_modctx_struct *`); it never frees or changes it.
- Arrays passed to a constructor (block lists, coefficient lists) are copied; the caller keeps ownership.
- Strings passed in are read during the call only; never retained.
- Strings returned (`_get_str`, `_dump_str`) are allocated with `flint_malloc` and freed by the caller with
  `flint_free` (`flint.h:201-204`), as for `arb_dump_str` (`arb.h:1088`) and `fmpz_get_str` (`fmpz.h:349`). The
  library also exports `adf_str_free(char *)`, which calls `flint_free`.
- Scratch space is allocated inside a call and released before it returns. There is no caller-visible scratch
  argument in version 1.

### 4.3 State of the outputs after each status

DECISION (proposed) CV-06: **on every status other than `ADF_OK` every output is left untouched** (bitwise the
same as before the call), with the single class of exceptions below; reason: a caller can retry at a higher
precision without re-creating aliased inputs, and the rule is the same for every function.

| Status | Output |
|---|---|
| `ADF_OK` | the result |
| any other | untouched. Optional report arguments (`where`, per-place status arrays) are written |

Exception: a function documented as **"enclosure with status"** writes a valid enclosure and returns
`ADF_NOT_DETERMINED`. The only such functions in version 1 are the complex archimedean wrappers when the input
ball meets a branch cut (`SPEC.md` 9.3.6: "returns the enclosure `acb` gives, with a status"). Where `SPEC.md` says
"an enclosure of all possible values, or a status" (characters on a coarse coset, `SPEC.md` 5; the additive
character on a fractional radius, `SPEC.md` 6; the profinite power, 9.3.7; the Kronecker symbol, 9.3.7), there are
two functions: the default returns `ADF_OK` with the enclosure, the `_strict` variant returns `ADF_NOT_DETERMINED`
and leaves the output untouched. DECISION (proposed) CV-07; reason: a status then never carries a value except in
the one case the specification demands it.

Implementation note (not a contract): the rule costs nothing for functions that cannot fail (`void`), and for
functions that fail before writing. Others compute into temporaries and `swap` at the end.

### 4.4 Invalid input

| Input | Behaviour |
|---|---|
| Non-finite real or complex ball (`arb_is_finite` false, `arb.h:134`) given to a constructor | `ADF_DOMAIN`, output untouched |
| Non-finite ball produced inside a computation from finite inputs | never stored; the function returns `ADF_NOT_DETERMINED` (or `ADF_DOMAIN` if a pole is proved). DECISION (proposed) CV-08; reason: the invariants of section 5 require finite balls |
| Zero denominator, negative radius, modulus `<= 0` where `>= 1` is required, `gcd(c, N) != 1` for a unit coset, composite `p` | `ADF_DOMAIN` from constructors, parsers and loaders |
| Non-canonical value passed to a public function | precondition violation: behaviour undefined. With `-DADF_CHECK_INVARIANTS` every public function checks `adf_x_is_canonical` on entry and calls `flint_abort` with a message. DECISION (proposed) CV-09; reason: checking a gcd on every add would dominate the cost that `PERF.md` measures |
| Aliasing forbidden by 4.1 | undefined behaviour |
| Text input | never undefined: every byte string of every length gives a status (sections 8-10); fuzzed |

A value that came out of any public function, parser or loader with `ADF_OK`, or out of `init`, is canonical.

### 4.5 Threads

- No global mutable state in the library (`SPEC.md` 10.1). Functions are reentrant.
- A value may be read concurrently by several threads; it must not be written while another thread reads or
  writes it.
- A context is immutable after `init` and may be read concurrently by any number of threads.
- FLINT keeps thread-local caches of constants (`ARB_DEF_CACHED_CONSTANT`, `arb.h:621-642`, declared with
  `FLINT_TLS_PREFIX`, `flint.h:156-160`) and registers cleanup functions (`arb.h:634`). These are FLINT's, per
  thread; they are not state of ours. A thread that used real functions should call FLINT's cleanup before it
  exits [source pending: FLINT documentation of `flint_cleanup`].
- The library never calls `flint_set_num_threads` and starts no threads.

### 4.6 Contexts: lifetime

DECISION (proposed) CV-10: **modulus contexts are caller-owned with a stated lifetime, not reference counted.**

Contract: the caller initialises a context with `adf_modctx_init_*`, may use it for any number of values, and calls
`adf_modctx_clear` only after every value that refers to it has been cleared or moved to another context or backend.
Clearing a context that a live value refers to is undefined behaviour. With `-DADF_CHECK_INVARIANTS` a context
counts the values that refer to it (atomically) and `adf_modctx_clear` aborts if the count is not zero.

Reasons:

1. FLINT's own contexts are caller-owned and have no count: `fmpz_mod_ctx_init`/`clear` (`fmpz_mod.h:40-45`),
   `padic_ctx_init`/`clear` (`padic.h:93-96`).
2. A count is mutable state inside an object that `SPEC.md` 10.1 calls immutable; it would have to be atomic, and
   every `init`, `set` and `clear` of a local value (the batch kernels of `PERF.md`) would write a shared cache
   line.
3. No operation needs to create a context: see the next rule.

Consequence: **no operation creates a context implicitly.** Where `SPEC.md` 4.1 allows a result that leaves its
context to "fall back to the global backend or be given a new context", version 1 always falls back to the global
backend, unless the caller passes a target context as an argument. DECISION (proposed) CV-11; reason: a context
created inside a call would have no owner.

Other context rules: a context never changes after `init` (`PLAN.md` section 4); two contexts with the same blocks
in the same order are interchangeable for every operation except `adf_x_identical`; operations on two local values
with different context pointers give a global result, even if the blocks agree. DECISION (proposed) CV-12; reason:
comparing block lists on every operation costs more than the conversion it saves.

## 5. Canonical forms and storage invariants

For each type: the struct (a proposal to be frozen with the header), the invariant as a checkable predicate (this is
what `adf_x_is_canonical` returns), the init value, who establishes the invariant and who may assume it, the exact
zero, radius zero, signs, and which representative is stored.

General rule for every type: **every public function that returns `ADF_OK` leaves each output satisfying its
predicate; every public function may assume that each input satisfies its predicate** (section 4.4). The
predicates are established from raw data only by: `init`, the constructors `adf_x_set_*` (which validate and
return `ADF_DOMAIN`), the parsers and loaders, and `adf_x_canonicalise` after an underscore function.

`gcd` of rationals is as in `proofs/precision.md` (lines 9-11). `Zhat`, `Z_p`, `U(N)` are as in `SPEC.md`.

### 5.1 `adf_rat` (exact rational)

    typedef struct { fmpq_t q; } adf_rat_struct;

- Predicate: `fmpq_is_canonical(q)` (`fmpq.h:117-118`): denominator `> 0`, `gcd(num, den) = 1`.
- Init: 0, stored `0/1` (`fmpq.h:28-32`). Exact zero: `0/1`. The sign is on the numerator.
- Meaning: the rational `q` at every place at once; exact (`SPEC.md` 4.1).

### 5.2 `adf_fball` (finite ball), global backend

    typedef struct {
        fmpz_t A, H, d;
        int backend;                     /* ADF_GLOBAL = 0, ADF_LOCAL = 1 */
        const adf_modctx_struct * mctx;  /* ADF_GLOBAL: NULL */
        ulong * res;                     /* ADF_GLOBAL: NULL */
    } adf_fball_struct;

Meaning: the set `(A + H Zhat)/d`, centre `a = A/d`, radius `N = H/d` (`SPEC.md` 4.1).

Predicate `G(A, H, d)` (`SPEC.md` 4.1, D2), for `backend == ADF_GLOBAL`:

    mctx == NULL and res == NULL and d > 0 and H >= 0 and
    ( H > 0 and 0 <= A < H and gcd(A, H, d) = 1 )  or  ( H = 0 and gcd(A, d) = 1 )

- Init: the exact 0, `(A, H, d) = (0, 0, 1)`. DECISION (proposed) CV-13; reason: as `fmpq_init` and `arb_init`, and
  `0 mod 1` would be a guess.
- Exact zero: `(0, 0, 1)`. Radius zero: `H = 0`, a single rational `A/d` in lowest terms.
- Signs: `d > 0`, `H >= 0`. For `H > 0` the stored centre is non-negative; for `H = 0` the sign is on `A`.
- Stored representative: for `H > 0`, the centre `a = A/d` is the unique element of `(a + N Z)` in `[0, N)`.

**Lemma 5.2 (uniqueness).** Two triples satisfying `G` that describe the same set are equal.

*Proof.* Radius zero: the set is one rational, and lowest terms with `d > 0` are unique. Radius positive: the set
determines `N` (`SPEC.md` 4.2, `equal_set` needs `N = M`), and its rational points are `a + N Z` (Lemma 1 of
`proofs/precision.md`). A triple `(A, H, d)` for the set needs `d a'` integral for some rational point `a'` and
`d N = H` integral; since `d (a' + k N) = d a' + k H`, the first condition does not depend on the choice of `a'`.
So the admissible `d` are the positive multiples of `d0 = lcm(den(a'), den(N))`. For `d = k d0` the triple is
`k (A0 + j H0, H0, d0)` for some integer `j`, whose gcd is at least `k`; so `gcd = 1` forces `k = 1`. At `k = 1` the
gcd is 1: a prime `l` dividing `A0`, `H0` and `d0` would make `(d0/l) a'` and `(d0/l) N` integers, against the
minimality of `d0`. With `H` and `d` fixed, `A` runs through `d a' + H Z`, and `0 <= A < H` picks one element.

### 5.3 `adf_fball`, local backend

Meaning: the same set as the global triple `(A*, H, d)`, where `A*` is not stored (`SPEC.md` 4.1).

Predicate `L(x)`, for `backend == ADF_LOCAL`, with `k = mctx->nblocks` and blocks `q_1, ..., q_k` of the context:

    mctx != NULL and k >= 1 and H = q_1 ... q_k and A = 0 and res != NULL and
    0 <= res[i] < q_i for all i and G(A*, H, d),
    where A* is the unique integer in [0, H) with A* = res[i] mod q_i for all i.

- `A` is kept 0 so that `clear` and `swap` need no case distinction. DECISION (proposed) CV-22; reason: an unused
  field with a fixed value cannot be read by mistake as the centre.
- `H` is stored although the context determines it; reading `H` needs no context. `res` has length `k`, owned by
  the value.
- Established by: the conversion `adf_fball_set_local(y, x, ctx)`, which returns `ADF_DOMAIN` if `H != K` (the set
  would change; `PLAN.md` section 7, "the set is unchanged by conversion"); the local kernels when the result keeps
  `H` and the context. Any result that changes `H`, or combines different context pointers, is global
  (CV-11, CV-12).
- The denominator is applied after recombination; it is never inverted modulo a block (`SPEC.md` 4.1).

### 5.4 Scaled value (policy 2 of `SPEC.md` 4.4)

    typedef struct {
        fmpq_t s;                        /* exact = 0: s > 0.  exact = 1: the exact value, any sign */
        fmpz_t u;                        /* exact = 0: 0 <= u < K.  exact = 1: u = 0 */
        const adf_modctx_struct * mctx;  /* never NULL; K is the context's modulus */
        int exact;
    } adf_scaled_struct;

- Predicate: `mctx != NULL`, `exact` in `{0, 1}`, `fmpq_is_canonical(s)`; if `exact = 0`: `s > 0` and
  `0 <= u < K`; if `exact = 1`: `u = 0`.
- Meaning: `exact = 0`: the set `s (u + K Zhat) = s u + s K Zhat`; `exact = 1`: the rational `s`.
- The field `exact` is added to `PLAN.md` section 4. DECISION (proposed) CV-14; reason: Proposition 6(2) of
  `proofs/precision.md` produces the exact 0, and `s (u + K Zhat)` cannot hold it.
- Uniqueness: for `exact = 0` the set determines its radius `s K`, hence `s`, and then `u` modulo `K`; the stored
  `(s, u)` is unique without a normalisation step.
- Init: `adf_scaled_init(x, ctx)` gives the exact 0 in `ctx`.
- PENDING (part B): `docs/proofs/policies.md` (lane m0-proofs-ideles). The rules for exact zero, exact scalars,
  addition of an exact scalar and conversion between contexts. Only the storage is fixed here.

The absolute cap (policy 3) stores plain `adf_fball` values; the cap `C`, a positive rational, is an argument of the
capped operations, not a field. DECISION (proposed) CV-23; reason: `SPEC.md` 4.4 says "the radius stays with each
value"; nothing else is per value. PENDING (part B): `docs/proofs/policies.md` for the cap applied to exact values.

### 5.5 `adf_adele` and `adf_cadele`

    typedef struct { arb_t inf; adf_fball_struct fin; } adf_adele_struct;
    typedef struct { acb_t inf; adf_fball_struct fin; } adf_cadele_struct;

- Predicate: `arb_is_finite(inf)` (`arb.h:134`), respectively `acb_is_finite(inf)` (`acb.h:801`), and `fin`
  satisfies `G` or `L`.
- Init: `(0 ; 0)`, both coordinates exact 0.
- The two coordinates are independent; an adele with both coordinates exact is still an `adf_adele`, not an
  `adf_rat` (a real ball has a dyadic midpoint, `SPEC.md` 4.1).
- Radius zero at infinity: `arb_is_exact(inf)` (`arb.h:61`).
- The real ball is never normalised beyond what `arb` does; its midpoint and radius are whatever the operation
  produced.

### 5.6 `adf_ucoset` (unit coset)

    typedef struct { fmpz_t c, N; } adf_ucoset_struct;

Meaning: `c U(N)`, the units of `Zhat` congruent to `c` modulo `N` (`SPEC.md` 5).

Predicate:

    ( N >= 1 and 1 <= c <= N and gcd(c, N) = 1 )  or  ( N = 0 and c in {1, -1} )

- **Exact units.** `N = 0` is admitted, with `U(0) = {1}` ("congruent modulo 0" is equality), so `[1 mod 0]` is the
  exact unit 1 and `[-1 mod 0]` the exact unit -1. DECISION (proposed) CV-15; reason: `proofs/catalogue.md`
  Proposition 12 and `SPEC.md` 9.3.7 return "the exact 1", and the idele of an exact rational `q` has the exact unit
  `sign(q)`; `SPEC.md` 5 and `PLAN.md` 4 say `N >= 1` (finding F1, section 14). The product rule
  `c c' mod gcd(N, N')` of `SPEC.md` 5 holds unchanged with `gcd(0, N') = N'`.
- **Residue range 1 to `N`.** DECISION (proposed) CV-16; reason: a unit has no residue 0 except modulo 1, and the
  range `1..N` makes the whole unit group `[1 mod 1]` rather than `[0 mod 1]`.
- **Stored modulus as supplied.** The stored `N` is the one supplied to the constructor, parser or loader; it need
  not be normal. DECISION (proposed) CV-17; reason: `PLAN.md` 5 says the dump keeps the modulus as supplied, which
  is void if constructors normalise it.
- **Normal form** (used by the printer of the value form and by `equal_set`): `N' = N/2` if `N = 2 mod 4`,
  else `N' = N`; `c'` is `c` reduced into `1..N'`; exact units unchanged. `U(2m) = U(m)` for odd `m`
  (`SPEC.md` 5, N12; `proofs/catalogue.md` Proposition 13). `adf_ucoset_is_normal(x)` tests it.
- Init: the exact unit 1, `(c, N) = (1, 0)`.
- Equality of cosets compares the normal forms (`SPEC.md` 5).

### 5.7 `adf_idele` and `adf_idclass`

    typedef struct { arb_t inf; fmpq_t r; adf_ucoset_struct u; } adf_idele_struct;
    typedef struct { arb_t t; adf_ucoset_struct u; } adf_idclass_struct;

- Idele predicate: `arb_is_finite(inf)` and `arb_is_nonzero(inf)` (`arb.h:240`: the ball excludes 0);
  `fmpq_is_canonical(r)` and `r > 0`; `u` satisfies 5.6.
- Class predicate: `arb_is_finite(t)` and `arb_is_positive(t)` (`arb.h:241`); `u` satisfies 5.6.
- Signs (`SPEC.md` 5): the finite part is `r u` with `r > 0`; the sign of the finite coordinates is carried by the
  unit. The real sign is in `inf`. The class of an idele is `t = |x_inf| / r`, `u' = sign(x_inf) u`.
- The idele of an exact rational `q != 0`: `inf` an enclosure of `q` at `prec`, `r = |q|`, `u = [sign(q) mod 0]`.
- Init: the exact idele 1: `inf = 1` exact, `r = 1`, `u = [1 mod 0]`. Class init: `<1 ; [1 mod 0]>`.

### 5.8 `adf_lball` (local ball at one prime)

    typedef struct {
        ulong p;      /* a prime */
        fmpq_t u;     /* see the predicate */
        slong v;      /* valuation of the centre; 0 when u = 0 */
        slong N;      /* ball: absolute precision, the radius is p^N.  exact: 0 */
        int exact;
    } adf_lball_struct;

The fields `u, v, N` follow `padic_struct` (`padic.h:32-41`: unit `u`, valuation `v`, absolute precision `N`) so
that the centre can be handed to FLINT; unlike `padic`, the prime is in the value, since a partial ball holds balls
at several primes. `u` is an `fmpq` only for exact values.

Predicate:

    p is prime and 2 <= p < 2^64 and exact in {0, 1} and fmpq_is_canonical(u) and
    exact = 1:  N = 0 and ( (u = 0 and v = 0) or (p divides neither numerator nor denominator of u) )
    exact = 0:  u is an integer and ( (u = 0 and v = 0) or (v < N and p does not divide u and 0 < u < p^(N - v)) )

- Meaning: exact: the rational `p^v u` of `Q_p`. Ball: `p^v u + p^N Z_p`.
- The stored ball centre `c = p^v u` is the unique element of `Z[1/p]` in `[0, p^N)` that lies in the ball.
  *Proof.* `Z[1/p] ∩ p^N Z_p = p^N Z`: an element `m/p^j` with valuation `>= N` times `p^(-N)` lies in
  `Z_p ∩ Z[1/p] = Z`. So the elements of `Z[1/p]` in the ball form one class `c + p^N Z`, and it meets `[0, p^N)` in
  one point. A ball containing 0 has `c = 0`, stored as `u = 0, v = 0`.
- Exact 0 (`exact = 1, u = 0`) and the ball `O(p^N)` around 0 (`exact = 0, u = 0`) are different values
  (`SPEC.md` 9.3.1: a stored centre 0 with finite precision is not the exact 0).
- Primes are one word in version 1. DECISION (proposed) CV-18; reason: primality is then certified by `n_is_prime`
  (`ulong_extras.h:335`) and places fit a `ulong` (section 7). Larger primes: `ADF_UNSUPPORTED`.
- Init: `p = 2`, exact 0. DECISION (proposed) CV-19; reason: some prime must be chosen; 2 is the first.
- Whether FLINT's own reduced form of a `padic` uses the same range `0 <= u < p^(N - v)` is
  [source pending: FLINT 3.0.1 `padic` documentation, "canonical" and "reduced"]; the headers show only the fields
  and `_padic_reduce` (`padic.h:163-165`).

### 5.9 `adf_sball` (partial ball over a finite set of places)

    typedef struct {
        int arch;                  /* ADF_ARCH_NONE = 0, ADF_ARCH_REAL = 1, ADF_ARCH_COMPLEX = 2 */
        acb_t inf;                 /* REAL: imaginary part exact 0; NONE: exact 0 */
        slong len;                 /* number of primes */
        adf_lball_struct * loc;    /* owned, primes strictly increasing */
    } adf_sball_struct;

- Predicate: `arch` in `{0, 1, 2}`; `acb_is_finite(inf)`; `arch = 1` implies the imaginary part is exact 0;
  `arch = 0` implies `inf` is exact 0; `len >= 0`; each `loc[i]` satisfies 5.8; `loc[i].p < loc[i+1].p`.
- The set of places is `{infinity}` (if `arch != 0`) together with the primes `loc[i].p`. The archimedean place
  carries the tag real or complex (`SPEC.md` 9.3.1).
- Init: the empty set of places (`arch = 0`, `len = 0`).

### 5.10 `adf_qclass` (element or set modulo `Q`)

    typedef struct { int form; slong len; adf_adele_struct * piece; } adf_qclass_struct;
                                          /* form: ADF_QCLASS_LIFT = 0, ADF_QCLASS_PIECES = 1 */

Fixed by `SPEC.md` 6 and used here:

- LIFT: `len = 1`; `piece[0]` is any adele (5.5); the value is its class modulo `Q` ("always available, always
  exact as a set").
- PIECES: `len >= 1`; each piece has the midpoint of its real part in `[0, 1]` and a finite part inside `Zhat`
  with integer radius: `d = 1` (the centre is an integer in `[0, H)`, or `H = 0` and the centre an integer). The
  gluing rule: `(1 ; z)` is the point `(0 ; z - 1)`. `SPEC.md` 6 asks for pieces that are closed intervals inside
  `[0, 1]`; an `arb` enclosure of such an interval is in general slightly larger (its end points are rounded
  outward), so the stored invariant can only be asked of the midpoint (finding F2, section 14). The parser checks
  the midpoint of the exact decimal interval (section 9.3). DECISION (proposed) CV-45; reason: an invariant that
  outward rounding cannot keep would make every quotient operation fail at the ends of `[0, 1]`.
- Canonical order of pieces (proposed): increasing by the exact lower end of the real part, then its upper end, then
  `H`, then `A`; no two pieces identical. DECISION (proposed) CV-24; reason: any fixed order makes the printed form
  unique; this one is cheap.
- PENDING (part B): `docs/proofs/quotient.md`. Whether pieces may overlap or must be merged; whether a piece that
  touches 1 is glued to one that touches 0; the default piece limit.
- Init: LIFT of the adele `(0 ; 0)`.

### 5.11 `adf_ffun` (finite test function)

    typedef struct { ulong D, M; acb_ptr f; } adf_ffun_struct;   /* f has length L = D M; f[j] = f(j/D) */

- Predicate: `D >= 1`, `M >= 1`, `D M < 2^62`, every `f[j]` finite. Meaning as in `SPEC.md` 7: zero outside
  `(1/D) Zhat`, constant on cosets of `M Zhat`.
- No normal form: a smaller `D` or `M` may describe the same function, and equality of ball values is undecided, so
  the stored `(D, M)` is kept. DECISION (proposed) CV-20; reason: a reduction to minimal `(D, M)` is not decidable
  on balls.
- Init: `D = M = 1`, `f[0] = 0` (the zero function).

### 5.12 `adf_rfun` (real test function)

    typedef struct { acb_poly_t P; acb_t A, B, C; } adf_rterm_struct;  /* P(x) exp(-pi A x^2 + B x + C) */
    typedef struct { slong len; adf_rterm_struct * term; } adf_rfun_struct;

- Predicate: `len >= 0`; for each term: `P` has length `>= 1` and finite coefficients; `A, B, C` finite; `Re(A) > 0`
  certified (`arb_is_positive` of the real part of `A`, `arb.h:241`) (`SPEC.md` 7).
- No normal form; terms are kept in the stored order (CV-20).
- Init: `len = 0` (the zero function).

### 5.13 `adf_char` (quasi-character of the idele class group)

    typedef struct { ulong q; ulong n; int parity; acb_t s; } adf_char_struct;

Meaning: `t^s chi(u')` on `R_{>0} x Zhat^x` (`SPEC.md` 5), `chi` the Dirichlet character with Conrey label `n`
modulo `q`, as numbered by FLINT's `dirichlet` module (`dirichlet.h:37`, "conrey generator"; `dirichlet.h:114`
`dirichlet_char_log`) [source pending: FLINT 3.0.1 `dirichlet` documentation of the numbering].

- Predicate: `q >= 1`; `1 <= n <= q`; `gcd(n, q) = 1`; the character is primitive, i.e. its conductor
  (`dirichlet_conductor_char`, `dirichlet.h:111`) equals `q`; `parity` equals `dirichlet_parity_char`
  (`dirichlet.h:110`), 0 even, 1 odd; `s` finite.
- The stored character is **primitive**; `q` is the conductor (`SPEC.md` 5: "a character carries its conductor").
  A constructor given a non-primitive `(q, n)` lowers it with `dirichlet_char_lower` (`dirichlet.h:136`).
  Note **[probed]** with python-flint 0.8.0: the label of the primitive character is not always `n mod f`:
  `(16, 9)` has conductor 8, and `(8, 1)` is the principal character.
- Init: the principal character, `q = 1, n = 1`, `s = 0`.
- PENDING (part B): `docs/proofs/analysis.md`. Whether the idele class character is `chi(u')` or `conj(chi(u'))`
  (`SPEC.md` 8 fixes `conj(chi(u'))` for the Tate integral of `L(s, chi)`), and the sign convention of `s`.

### 5.14 `adf_modctx` (modulus context)

Opaque (section 12). Contents, not part of the interface: `K >= 1` (an `fmpz`); `k >= 0` word blocks
`q_1, ..., q_k`, pairwise coprime, `2 <= q_i < 2^64`, in the order supplied, with product `K` when `k >= 1`; an
`nmod_t` per block (`nmod_init`, `nmod.h:212`); for each block whether it is a certified prime power and its prime;
recombination data. Constructors:

| Constructor | Blocks |
|---|---|
| `adf_modctx_init_blocks(ctx, q, k)` | as supplied; `ADF_DOMAIN` unless pairwise coprime and each `>= 2` |
| `adf_modctx_init_prime_powers(ctx, p, e, k)` | `p_i^e_i`, primes certified; `ADF_DOMAIN` on a composite or repeated prime; `ADF_UNSUPPORTED` if a power exceeds a word |
| `adf_modctx_init_fmpz(ctx, K)` | one block `K` if `2 <= K < 2^64`; none if `K = 1` or `K >= 2^64` (then only the scaled policy can use it) |
| factorial `k!`, powers of a primorial | the prime powers of `K` in increasing order of the prime. DECISION (proposed) CV-21; reason: the factorisation is known, and prime-power blocks serve operations that name a prime |

The local backend needs `k >= 1`; conversion into a context with `k = 0` returns `ADF_UNSUPPORTED`.

## 6. Signs, measures, normalisations

All of this section is PENDING (part B). Each subsection states only what `SPEC.md` fixes today.

### 6.1 Additive character and Fourier transform

PENDING (part B): `docs/sources.md` and `refs/` (lane m0-sources: quotation of Tate's thesis section 2.2 or its
substitutes), `docs/proofs/analysis.md` (lane m0-proofs-analysis). Fixed by `SPEC.md` 6 (**[unverified]** there):

    psi(x) = exp( 2 pi i ( - x_inf + sum over p of {x_p}_p ) )
    hat f(y) = integral of f(x) conj(psi(x y)) dx

The real kernel is `exp(+2 pi i x y)`, the finite kernel `exp(-2 pi i x y)`; `psi` is 1 on `Q`. `{x_p}_p` is the
p-primary fractional part, a rational in `[0, 1)` with denominator a power of `p` (`SPEC.md` 9.3.6).

### 6.2 Measures

PENDING (part B): `docs/proofs/analysis.md`. Fixed by `SPEC.md` 7 and 8: `Z_p` has volume 1, so `N Zhat` has
volume `1/N`; Lebesgue measure at infinity; `A/Q` has volume 1. Multiplicative: `d^x x = dx/|x|` at infinity, and at
each prime the measure giving `Z_p^x` volume 1.

### 6.3 Finite Fourier transform

PENDING (part B): `docs/proofs/analysis.md`. Fixed by `SPEC.md` 7:
`hat f(k/M) = (1/M) sum_{j=0}^{L-1} f_j exp(-2 pi i j k / L)`, `L = D M`, stored as `f_j = f(j/D)`.

### 6.4 Gauss sums, root numbers, local constants

PENDING (part B): `docs/proofs/analysis.md`, `docs/proofs/functions.md`. Fixed by `SPEC.md` 9.3.7: Gauss sums use a
Dirichlet character with its conductor, extended by 0, and the additive character of 6.1; local constants are
defined by `Z(hat f, chi^-1, 1-s) = gamma(s, chi) Z(f, chi, s)` with the transform and measure of 6.1 and 6.2; the
epsilon factor needs the normalisation of the L-factor. Tests compare with `acb_dirichlet_gauss_sum`
(`acb_dirichlet.h:126`), whose normalisation is [source pending: FLINT `acb_dirichlet` documentation].

### 6.5 Class-group characters

PENDING (part B): `docs/proofs/analysis.md`. Fixed by `SPEC.md` 5 and 8: quasi-characters `t^s chi(u')`, unitary
when `Re(s) = 0`; in the Tate integral for `L(s, chi)` the idele character is `conj(chi(u'))`, so that the Euler
factors carry `chi(p)`. The value on a coset `c U(N)` is one number only when the conductor divides `N` (5.13 and
4.3 for the two function variants).

### 6.6 Reciprocity (cyclotomic action)

PENDING (part B): `docs/sources.md` (Milne's notes on disk), `docs/proofs/catalogue.md`. Fixed by `SPEC.md` 9.3.7:
two conventions, arithmetic `z -> z^(1/u')` and geometric `z -> z^(u')`; the choice is part of the function's
name. Proposed names, pending the choice: `adf_idclass_cyclo_arith`, `adf_idclass_cyclo_geom`. Test vector: the
idele with `p` at the place `p` and 1 elsewhere has `u' = 1/p` away from `p`; under the arithmetic convention
`z -> z^p` for `n` prime to `p`.

### 6.7 Recommendations of the seams sketch

PENDING (part B): `docs/seams.md` (lane m0-seams), section 5 there. Nothing in this document is frozen before those
recommendations are merged (`PLAN.md` section 1, principle 1).

## 7. Places

- A place is a `ulong`: `0` is the archimedean place, any other value is a prime `p < 2^64` (CV-18).
  `typedef ulong adf_place_t;`
- **Canonical order of places:** the archimedean place first, then the primes in increasing order. It is the order
  of printing (9.4), of `adf_sball` storage (5.9), of per-place reports and of the tie-break of section 3.3.
- A set of places (`adf_places_t`) is stored sorted in this order, without repetition; constructors sort and reject
  repetitions with `ADF_DOMAIN`.

## 8. Text: general rules

Two forms (`SPEC.md` 10.2, `PLAN.md` 5). The **value form** is canonical, independent of the backend, for people
and for exact exchange of finite parts; a real ball in it is a decimal enclosure. The **dump form** is versioned,
keeps backend and context, writes real balls as exact dyadic numbers, and reading a dump gives back the identical
object.

### 8.1 Interface

    int adf_x_set_str(adf_x_t x, const char * s, size_t len, [slong prec,] const adf_text_limits_t * lim);
    char * adf_x_get_str(const adf_x_t x, [slong digits]);          /* value form, flint_malloc */
    int adf_x_load_str(adf_x_t x, const char * s, size_t len, const adf_modctx_struct * ctx,
                       const adf_text_limits_t * lim);
    char * adf_x_dump_str(const adf_x_t x);                          /* dump form, flint_malloc */

- The input is the `len` bytes at `s`; it need not be NUL-terminated, and a NUL byte inside it is an error
  (`ADF_PARSE`). DECISION (proposed) CV-25; reason: a NUL-terminated interface silently truncates at an embedded
  NUL, and bindings (section 12) pass lengths anyway.
- `prec` is present for the types with a real or complex part; `digits` likewise (default `ADF_DIGITS_DEFAULT = 20`,
  `1 <= digits <= 10^6`).
- `lim = NULL` means the defaults of 8.4.
- Output strings contain no NUL, no leading or trailing whitespace, and no newline.

### 8.2 Alphabet and whitespace

- Allowed bytes: `0x20` to `0x7E`, and TAB `0x09`, LF `0x0A`, CR `0x0D`. Any other byte (NUL, other control bytes,
  every byte `>= 0x80`, hence all non-ASCII UTF-8) gives `ADF_PARSE`. DECISION (proposed) CV-26; reason: one
  alphabet, no Unicode look-alikes of `-` or digits.
- Value form: whitespace (space, TAB, LF, CR) may stand before the first token, after the last, and between any two
  tokens; never inside a token. The printer writes exactly the templates of section 9.4 (single spaces).
- Dump form: exactly one space between tokens, nothing else (section 10).
- Keywords are case-sensitive.

### 8.3 Tokens of the value form

A token is one of: a number (section 9.1), a keyword (a maximal run of ASCII letters that must equal the keyword the
grammar expects: `mod O p R C i Q union ffun rfun term char D M P A B q n s`), `+/-`, or one of the single
characters `( ) [ ] < > { } ; , : * + = ^`. The longest match is taken, so `+/-` is one token. A number starts with
a digit or with `-` directly followed by a digit. A letter directly after a number ends the number (`5mod6` is the
three tokens `5 mod 6`; `1 e5` is `1` followed by the unknown keyword `e`).

### 8.4 Limits

    typedef struct { size_t max_len; slong max_exp10; slong max_prec; slong max_items; } adf_text_limits_t;

| Limit | Default | Applies to |
|---|---|---|
| `max_len` | 1048576 bytes | the whole input, value form and dump |
| `max_exp10` | 100000 | the absolute value of the exponent of a decimal (`1e100001` is over) |
| `max_prec` | 100000 | the absolute value of `N` in `O(p^N)`, of the fields `v` and `N` of a dumped local ball |
| `max_items` | 1048576 | `D M` of `ffun`; the number of pieces, terms, places, polynomial coefficients, blocks |

DECISION (proposed) CV-27; reason: each bounds the memory and time that a short input can demand (a 16-byte
`O(5^99999999999)` would otherwise ask for a 2^38-bit modulus). The limits are arguments, not global state.
Integers in the value form have no separate limit; `max_len` bounds them. The grammar is not recursive (no
production contains itself), so nesting depth is bounded by the grammar: `((((1))))` is simply `ADF_PARSE`.

### 8.5 Order of checks

The first failing stage determines the status; the output is untouched (4.3).

1. `len > max_len`: `ADF_LIMIT` (checked before any byte is read).
2. A forbidden byte (8.2): `ADF_PARSE`.
3. The grammar (section 9.2 or 10): `ADF_PARSE`. For the dump: first the header; an unknown version is
   `ADF_UNSUPPORTED` at this point, before the body is read.
4. Limits on literals and counts (8.4): `ADF_LIMIT`. Checked on the digit strings, before any big number is
   formed.
5. Word restrictions: a prime `p` or a character modulus `q` `>= 2^64`: `ADF_UNSUPPORTED`.
6. Semantic constraints (section 9.3, and the predicates of section 5 for the dump): `ADF_DOMAIN`.
7. Only in C, for a real or complex part: a sign condition that holds for the exact decimal interval but not for
   its enclosure at `prec` (section 9.3): `ADF_NOT_DETERMINED`.

DECISION (proposed) CV-28; reason: the status then does not depend on the order in which a parser happens to meet
the defects.

## 9. Value form

### 9.1 Lexical grammar (no whitespace inside)

    digit  = "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9" ;
    uint   = digit , { digit } ;                          (* leading zeros accepted, never printed *)
    int    = [ "-" ] , uint ;
    urat   = uint , [ "/" , uint ] ;
    rat    = int , [ "/" , uint ] ;
    exp10  = ( "e" | "E" ) , [ "+" | "-" ] , uint ;
    udec   = uint , [ "." , uint ] , [ exp10 ] ;
    dec    = [ "-" ] , udec ;
    sint   = [ "-" ] , uint ;

There is no `+` sign on a number (only in an exponent), no `.5`, no `5.`, no hexadecimal, no `inf` or `nan`.

### 9.2 Syntactic grammar (whitespace allowed between tokens)

    real      = dec , [ "+/-" , udec ] ;
    complex   = "(" , real , ")" , "+" , "(" , real , ")" , "*" , "i" ;
    fin       = rat , [ "mod" , urat ] ;
    ucoset    = "[" , int , [ "mod" , uint ] , "]" ;
    lcoord    = rat , [ "+" , "O" , "(" , uint , [ "^" , sint ] , ")" ] ;
    adele_v   = "(" , real , ";" , fin , ")" ;

    rat_v     = rat ;
    fball_v   = "(" , "*" , ";" , fin , ")"  |  rat , "mod" , urat ;
    cadele_v  = "(" , complex , ";" , fin , ")" ;
    ucoset_v  = ucoset ;
    idele_v   = "(" , real , ";" , urat , "*" , ucoset , ")" ;
    idclass_v = "<" , real , ";" , ucoset , ">" ;
    lball_v   = "[" , "p" , "=" , uint , ":" , lcoord , "]" ;
    sentry    = "R" , ":" , real  |  "C" , ":" , complex  |  "p" , "=" , uint , ":" , lcoord ;
    sball_v   = "{" , [ sentry , { ";" , sentry } ] , "}" ;
    qclass_v  = adele_v , "+" , "Q"  |  "union" , "(" , adele_v , { "," , adele_v } , ")" , "+" , "Q" ;
    ffun_v    = "ffun" , "(" , "D" , "=" , uint , "," , "M" , "=" , uint , ";" , complex , { "," , complex } , ")" ;
    rterm     = "term" , "(" , "P" , "=" , "[" , complex , { "," , complex } , "]" , "," , "A" , "=" , complex ,
                "," , "B" , "=" , complex , "," , "C" , "=" , complex , ")" ;
    rfun_v    = "rfun" , "(" , [ rterm , { "," , rterm } ] , ")" ;
    char_v    = "char" , "(" , "q" , "=" , uint , "," , "n" , "=" , uint , "," , "s" , "=" , complex , ")" ;

The start symbol of the parser of type `x` is `x_v`, and the whole input must be consumed. The languages of the
thirteen start symbols are pairwise disjoint, so the type of a text is determined by its syntax (9.7).

Meaning: `real` is the closed interval `[m - r, m + r]`, `m` the value of `dec`, `r` of `udec` (0 if absent);
`complex` is `re + im i`; `fin` is `a + N Zhat` (`N = 0` if `mod` is absent: the point `a`); `ucoset` is `c U(N)`
(`N = 0`, an exact unit, if `mod` is absent); `lcoord` is the exact rational if no `O`-term, else the ball
`a + p^N Z_p` (`O(p)` means `N = 1`); the idele `(x ; r * [c mod N])` is `x_inf = x`, finite part `r c U(N)`;
the class `<t ; u>`; the partial ball lists its places; the qclass is the class modulo `Q` of an adele, or the
union of pieces with the gluing rule; `ffun(D=, M=; f_0, ..., f_{L-1})` has `f_j = f(j/D)`; a term is
`P(x) exp(-pi A x^2 + B x + C)` with `P = [c_0, c_1, ...]`, `c_k` the coefficient of `x^k`; `char(q=, n=, s=)` is
`t^s chi(u')` with the Conrey label `n` modulo `q`.

### 9.3 Semantic constraints

| Where | Constraint | Status if violated |
|---|---|---|
| every `/` | denominator not 0 | `DOMAIN` |
| `ucoset` | `N >= 1`: `gcd(c, N) = 1`; `N = 0` or absent: `c = 1` or `c = -1` | `DOMAIN` |
| idele | `r > 0`; the exact interval of `real` excludes 0 | `DOMAIN` |
| idele class | the exact interval of `real` lies in `(0, infinity)` | `DOMAIN` |
| `lball_v`, `sentry` with `p=` | `p < 2^64` | `UNSUPPORTED` |
| | `p` prime (`n_is_prime`, `ulong_extras.h:335`) | `DOMAIN` |
| | the base inside `O(...)` equals `p`; `abs(N) <= max_prec` (the latter is stage 4: `LIMIT`) | `DOMAIN` |
| `sball_v` | at most one of `R`, `C`; no prime twice | `DOMAIN` |
| `qclass_v`, form `union` | each piece: the midpoint of `real` in `[0, 1]` (5.10, CV-45); the finite part is an integer with `mod` an integer (or no `mod`) | `DOMAIN` |
| `ffun_v` | `D >= 1`, `M >= 1`; `D M <= max_items` (stage 4: `LIMIT`); number of values `= D M` | `DOMAIN` |
| `rterm` | the exact interval of the real part of `A` lies in `(0, infinity)` | `DOMAIN` |
| `char_v` | `q < 2^64` (`UNSUPPORTED`); `q >= 1`; `gcd(n mod q, q) = 1` | `DOMAIN` |
| decimals | `abs(exponent) <= max_exp10` (stage 4) | `LIMIT` |

The sign conditions (idele, class, `A`) are decided exactly on the decimal interval. A C parser that then rounds
the interval outward at `prec` checks the condition again on its `arb`; if it fails only there, the status is
`ADF_NOT_DETERMINED` (stage 7), and a higher `prec` will succeed. The condition on pieces concerns the midpoint;
a C parser checks it on the exact decimal midpoint and stores a midpoint rounded to nearest, which stays in `[0, 1]`
because 0 and 1 are representable.

Canonicalisation performed by the parser (non-canonical but valid input): leading zeros removed; fractions reduced;
`-0` becomes `0`; a finite centre reduced into `[0, N)`; `mod 0` dropped; a bare `a mod N` becomes `(* ; a mod N)`;
unit residues reduced into `1..N` (the stored modulus stays as written, CV-17; the printed one is normal); a local
centre reduced to its canonical representative (5.8), `O(p)` printed `O(p^1)`; partial-ball entries sorted by place;
duplicate pieces removed; a character lowered to its primitive character; `n` reduced; decimals rewritten by 9.5.

### 9.4 Printing templates

`q(x)` prints a rational: `n` if the denominator is 1, else `n/d`, in lowest terms, `-` only before a negative
numerator, `0` for zero. `r(x)` prints a real ball by 9.5, `z(x)` a complex ball as `(r(re)) + (r(im))*i`.

| Type | Canonical text |
|---|---|
| `adf_rat` | `q(x)` |
| finite part `F` | `q(a)` if `N = 0`; else `q(a) mod q(N)` with `a` the centre in `[0, N)` |
| `adf_fball` | `(* ; F)` |
| scaled value (5.4) | the finite ball it denotes: `(* ; F)` with centre `s u` and radius `s K`, or `(* ; q(s))` if exact. The value form does not record the policy or the context; a scaled value is read by reading an `adf_fball` and converting it (which reports loss, 3.2) |
| `adf_adele` | `(r(x_inf) ; F)` |
| `adf_cadele` | `(z(x_inf) ; F)` |
| `adf_ucoset` | normal form (5.6): `[c mod N]`, `1 <= c <= N`; exact units `[1]` and `[-1]` |
| `adf_idele` | `(r(x_inf) ; q(r) * U)`, `U` the unit coset as above; `q(r)` also when `r = 1` |
| `adf_idclass` | `<r(t) ; U>` |
| local coordinate `L` | exact: `q(p^v u)`; ball: `q(c) + O(p^N)` with `c = p^v u` and `N` in signed decimal (`O(5^-2)`, `O(5^0)`, `O(5^1)`) |
| `adf_lball` | `[p=P: L]` |
| `adf_sball` | `{E; E; ...}` in canonical place order; `E` is `R: r(x)`, `C: z(x)` or `p=P: L`; no places: `{}` |
| `adf_qclass` | lift: `(r ; F) + Q`; pieces: `union(X, X, ...) + Q`, `X = (r ; F)`, sorted like 5.10 but by the exact end points of the *printed* real parts, each distinct printed piece once (printing can make two pieces equal or change their order; this keeps the text a fixed point) |
| `adf_ffun` | `ffun(D=D, M=M; z(f_0), z(f_1), ...)` |
| `adf_rfun` | `rfun(T, T, ...)`, `T = term(P=[z(c_0), ...], A=z(A), B=z(B), C=z(C))`; zero function `rfun()` |
| `adf_char` | `char(q=q, n=n, s=z(s))` |

Examples (all in `tests/golden/`): `7/3`; `(3.14159 +/- 1e-5 ; 5/3 mod 6)`; `(* ; 2 mod 6)`; `[p=5: 3 + O(5^4)]`;
`(2.5 +/- 1e-9 ; 3/2 * [5 mod 36])`; `<1.25 +/- 1e-30 ; [5 mod 36]>` (`PLAN.md` 5, all canonical); `[5 mod 6]`
prints as `[2 mod 3]`.

### 9.5 Real balls: reading and printing

**Reading.** `dec +/- udec` denotes the exact rational interval `[m - r, m + r]`; without `+/-` the point `m`.
The Python reference keeps this interval exactly. A C parser produces an `arb` at `prec` that contains the interval
(enclosure: the contract). If `m` is a dyadic number whose odd mantissa has at most `prec` bits and `r` is a dyadic
number whose odd mantissa is below `2^30` (`MAG_BITS`, `mag.h:117`), the `arb` is exactly `[m +/- r]` (tightness:
a tested requirement). A decimal string is an enclosure on input, not a lossless form (`SPEC.md` 10.2).

**Printing.** Input: exact rationals `mid` and `rad >= 0` (for an `arb`, its dyadic midpoint and radius), and
`n = digits`. Notation: `X(y) = floor(log10 |y|)` for `y != 0`; `ceil2(E)` for `E > 0` is `E` rounded up to two
significant digits, `ceil(E / 10^(X(E) - 1)) * 10^(X(E) - 1)`; `round(y, q)` is `y` rounded to the nearest multiple
of `10^q`, ties to the even multiple.

1. If `rad = 0` and `mid` is a decimal with at most `n` significant digits (or 0): print `fmt(mid)`. Stop.
2. If `mid = 0`: print `0 +/- fmt(ceil2(rad))`. Stop.
3. `q = X(mid) - n + 1`; if `rad > 0`, `q = max(q, X(rad) - 1)`.
4. `M = round(mid, q)`; `R = ceil2(rad + |M - mid|)`; `q' = X(R) - 1` if `M = 0`, else
   `max(X(M) - n + 1, X(R) - 1)`.
5. If `M` is not a multiple of `10^q'`, set `q = q'` and repeat step 4.
6. Print `fmt(M) +/- fmt(R)`.

`fmt(y)` prints an exact decimal: `0` for zero; otherwise write `|y| = D 10^E` with `D` a positive integer not
divisible by 10, `k` the number of digits of `D`, `X = E + k - 1`. If `-4 <= X <= 20`, positional notation with no
superfluous zeros (`2.5`, `0.0001`, `1250`, `100000000000000000000`); else `d.ddd` `e` `X` with the digits of `D`, a
point only if `k > 1`, and the exponent without `+` and without leading zeros (`1e-5`, `1.5e-30`, `3e21`). A minus
sign precedes a negative number.

Properties (checked by the reference): the printed interval `[M - R, M + R]` contains `[mid - rad, mid + rad]`,
since `R >= rad + |M - mid|`; the loop of step 5 ends, because `q` increases strictly, and once `q >= X(rad) + 1`
(if `rad > 0`) and `q >= X(mid) - n + 2` the next `q'` is at most `q` (then `|M - mid| <= 10^q / 2` and
`rad < 10^q` give `X(R) <= q`, and `X(M) <= max(X(mid) + 1, q)`); printing is idempotent on its own output (a
printed text re-read and printed again is the same text), because the final `M` is a multiple of `10^q(M, R)` and
`R` has two significant digits.

**Constrained printing.** Where the type requires a sign condition of the real ball (the real part of an idele
excludes 0; the real part of a class and the real part of `A` in a term are positive), the printed interval must
satisfy it too, or the printed text could not be read back. The algorithm then takes a parameter `k >= 2`, the
number of significant digits of the radius: `ceil_k(E) = ceil(E / 10^(X(E) - k + 1)) * 10^(X(E) - k + 1)` replaces
`ceil2`; `X(rad) - k + 1` and `X(R) - k + 1` replace `X(rad) - 1` and `X(R) - 1` in steps 3 and 4; and
`n + k - 2` replaces `n` in steps 1, 3 and 4 (so `k = 2` is the unconstrained algorithm). The printer uses the least
`k >= 2` for which `[M - R, M + R]` satisfies the condition. Such a `k` exists: the exact interval satisfies the
condition with a positive margin (it is closed and the condition is open), and the printed interval exceeds the
exact one by at most `10^q + 10^(X(E) - k + 1)`, where `q = max(X(mid) - n - k + 3, X(rad) - k + 1)` in the first
pass and the loop of step 5 raises `q` only to `max(X(M) - n - k + 3, X(R) - k + 1)`; all of this tends to 0 as
`k` grows. Example: `(1 +/- 0.999999 ; 1 * [1])` needs `k = 6`, since `k = 2` would print `1 +/- 1`, which
contains 0.

The least level of a printed text can be lower than that of the value it came from (the reference found
`mid = 20396493/16` with a radius near `1.258e6`: level 4 prints `1275000 +/- 1259000`, whose least level is 3).
So the constrained printer **repeats**: print at the least level, read the text back exactly, print again, until
the text does not change; the result is that text. This ends: printing at a fixed level `k` is idempotent (the
argument above, with `k` for 2), so the least level of the text read back is at most the level used, and a
repetition happens only when it is strictly smaller; the levels decrease from the first one towards 2. The result
satisfies the condition, contains the value (each step is an enclosure) and is a fixed point of printing.

DECISION (proposed) CV-29: this algorithm, with default `n = 20`; reason: it is exactly specified (two
implementations print the same text for the same `arb`), it never loses the enclosure, and like `arb_get_str`
(`arb.h:168`) it drops midpoint digits that the radius makes meaningless.

### 9.6 Round trips

- Types without a real or complex part (`adf_rat`, `adf_fball`, `adf_ucoset`, `adf_lball`, and `adf_sball` without
  archimedean place): `print(parse(t))` is canonical, and `parse(print(v))` equals `v` as a set; it is identical
  to `v` except for the unit modulus, which is printed normal (CV-17) and for the backend, which the value form
  does not carry.
- With real or complex parts: `parse(print(v))` contains `v` (enclosure), and `print(parse(print(v))) = print(v)`.
  A printed idele, class or real test function can always be read back: constrained printing (9.5) keeps the sign
  conditions of 9.3.
- Every expected output in `tests/golden/` is a fixed point of the reference: re-reading and printing it gives it
  back.

### 9.7 Type of a text

`adf_text_classify(s, len)` returns the type whose start symbol derives the text, or `ADF_PARSE` if none does, or
`ADF_LIMIT` if `len > max_len`. It checks syntax only (stages 1 to 3); semantic errors are found by the typed
parser. The driver uses it; the typed parsers do not coerce between types (`7/3` is not accepted by
`adf_fball_set_str`). DECISION (proposed) CV-30; reason: one text, one type, and no silent conversion that could
hide an exact/inexact confusion.

### 9.8 Ambiguities of `PLAN.md` section 5, resolved

| # | Question left open | Resolution |
|---|---|---|
| A1 | How does an exact finite ball print? | `(* ; 7/3)`: the `mod` part is dropped for radius 0 (CV-31) |
| A2 | Is `7/3` a rational, a finite ball or an adele? | a rational (`adf_rat`); the finite ball is `(* ; 7/3)`, the adele needs a real part (9.7) |
| A3 | May a finite ball be written without `(* ; ...)`? | yes on input, `a mod N`; printed with it (CV-32) |
| A4 | Syntax of real balls | `m +/- r` only; no `[a, b]`, no `±`, no `inf`/`nan`; point without `+/-` (9.1, 9.2) |
| A5 | Which digits are printed for a real ball? | the algorithm of 9.5 (CV-29) |
| A6 | Precision of reading a decimal | the `prec` argument; containment is the contract, exactness for dyadic input a requirement (9.5) |
| A7 | Residue range of unit cosets, the modulus 1, exact units | `1..N`; `[1 mod 1]` is all units; `[1]`, `[-1]` are exact (CV-15, CV-16) |
| A8 | Is the scale of an idele printed when it is 1? | yes, `1 * [..]` (CV-33) |
| A9 | Local ball: which centre; how is `N` printed; exact local values | centre in `Z[1/p] ∩ [0, p^N)` (5.8); `O(p^N)` always with the exponent; exact value without `O`-term (CV-34) |
| A10 | Whitespace and case | 8.2, 8.3 |
| A11 | Does the value form record the backend? | no (`PLAN.md` 5); the dump does |
| A12 | Types without an example in `PLAN.md` 5 (`adf_cadele`, `adf_sball`, `adf_qclass`, `adf_ffun`, `adf_rfun`, `adf_char`) | forms of 9.2 (CV-35) |
| A13 | Does the parser keep the unit modulus as written? | yes; printed normal (CV-17) |
| A14 | Negative radius, negative modulus, negative scale | excluded by the grammar (`udec`, `urat`, `uint`): `ADF_PARSE` |
| A15 | Zero denominator, composite `p`, non-unit residue | `ADF_DOMAIN` (9.3) |
| A16 | Leading zeros | accepted, never printed (CV-36) |

## 10. Dump form

### 10.1 Grammar

Tokens are separated by exactly one space `0x20`; no other whitespace anywhere; no leading or trailing space.

    dump    = "adf" , version , " " , body ;     (* version "1"; any other version: UNSUPPORTED *)
    h       = "0" | [ "-" ] , hnz , { hdig } ;           (* integer in lower-case hexadecimal, no leading zeros *)
    hnz     = "1" | ... | "9" | "a" | ... | "f" ;   hdig = "0" | hnz ;
    arb     = h , " " , h , " " , h , " " , h ;   (* mid mantissa, mid exponent, rad mantissa, rad exponent *)
    acb     = arb , " " , arb ;
    ctx     = h , " " , h , { " " , h } ;                (* K, k, then q_1 ... q_k *)
    fb      = "g" , " " , h , " " , h , " " , h          (* A H d *)
            | "l" , " " , h , " " , ctx , { " " , h } ;  (* d, context, then res_1 ... res_k *)
    lb      = h , " " , ( "x" , " " , h , " " , h , " " , h      (* p; exact: num(u) den(u) v *)
                        | "b" , " " , h , " " , h , " " , h ) ;  (* p; ball: u v N *)
    body    = "rat" , " " , h , " " , h
            | "fball" , " " , fb
            | "scaled" , " " , ( "x" , " " , h , " " , h | "s" , " " , h , " " , h , " " , h ) , " " , ctx
            | "adele" , " " , arb , " " , fb
            | "cadele" , " " , acb , " " , fb
            | "ucoset" , " " , h , " " , h                          (* c N *)
            | "idele" , " " , arb , " " , h , " " , h , " " , h , " " , h    (* inf, num(r) den(r), c N *)
            | "idclass" , " " , arb , " " , h , " " , h
            | "lball" , " " , lb
            | "sball" , " " , ( "n" | "r" , " " , arb | "c" , " " , acb ) , " " , h , { " " , lb }
            | "qclass" , " " , ( "lift" , " " , arb , " " , fb | "pieces" , " " , h , { " " , arb , " " , fb } )
            | "ffun" , " " , h , " " , h , { " " , acb }             (* D M, then D M values *)
            | "rfun" , " " , h , { " " , h , { " " , acb } , " " , acb , " " , acb , " " , acb }
            | "char" , " " , h , " " , h , " " , acb               (* q n s *)
            | "modctx" , " " , ctx ;

A count fixes the number of repetitions that follow it (`k` in `ctx` and in `fb`; the number of primes in `sball`;
of pieces in `qclass` (form `pieces`); `D M` values in `ffun`; the number of terms in `rfun`, and in each term the
length of `P`); a mismatch is `ADF_PARSE`. DECISION (proposed) CV-37: this dump grammar; reason: one token per
struct field, so that load and dump are a direct transcription and identity is easy to test.

### 10.2 Rules

- **Strict.** The loader accepts only canonical text: the fields must satisfy the predicates of section 5
  (`ADF_DOMAIN` otherwise; the loader never canonicalises). Dumping a loaded value gives back the same text,
  byte for byte. DECISION (proposed) CV-38; reason: `PLAN.md` 5 requires "the identical object", and a loader that
  repaired input would make two texts denote one dump.
- **Real balls** are written as `arb_dump_str` (`arb.h:1088`) writes them. **[probed]** on FLINT 3.0.1: four
  lower-case hexadecimal integers, midpoint mantissa and exponent, radius mantissa and exponent, the value being
  `mantissa * 2^exponent`, each mantissa odd (or `0 0` for zero); `arb_dump_str` of 0.5 is `1 -1 0 0`, of -0.75 is
  `-3 -2 0 0`. Non-finite values use a zero mantissa with a non-zero exponent (`+inf` as `0 -1`, `nan` as
  `0 -3`). [source pending: FLINT 3.0.1 documentation of `arb_dump_str`.] Our loader accepts an `arb` field only if
  both mantissas are odd or the pair is `0 0`, and the radius mantissa is positive and below `2^30` (`mag.h:117`);
  anything else is `ADF_DOMAIN` (non-finite or not canonical).
- **The loader never passes unchecked text to `arb_load_str`** (`arb.h:1087`). **[probed]** In FLINT 3.0.1
  `arb_load_str` aborts the process (`SIGABRT`) on `"1 0 0 0 5"` (a fifth token), on a zero midpoint mantissa
  with an exponent other than `0, -1, -2, -3` (`"0 5 0 0"`, `"0 1 0 0"`, `"0 -4 0 0"`), on a zero radius mantissa
  with an exponent other than `0, -1` (`"1 0 0 1"`, `"1 0 0 -2"`), and on an odd radius mantissa of more than 30
  bits (`"1 0 40000001 0"`, `"1 0 7fffffff 0"`). It accepts and silently changes `"2 0 0 0"` (to `1 1 0 0`),
  `"A 0 0 0"` (to `5 1 0 0`), `"1 0 -1 0"` (a negative radius mantissa becomes positive) and
  `"1 0 40000000 0"`, and accepts a trailing space. Every `arb` group of every valid vector in
  `tests/golden/dump.tsv` loads and dumps back identically. A loader may build the `arb` from the validated tokens
  directly, or pass exactly the four validated tokens; the validation of this section excludes every aborting
  case found.
- **Contexts.** A local `fball` (also inside `adele`, `cadele`, `qclass`) and a scaled value record their context
  (`ctx`). `adf_x_load_str(x, s, len, ctx, lim)` makes `x` refer to the caller's `ctx`, which must have the same
  `K` and the same blocks in the same order; otherwise, or if `ctx` is `NULL`, the status is `ADF_DOMAIN`.
  `adf_modctx_init_load_str(ctx, s, len, lim)` creates a context from a `modctx` dump, or from the context recorded
  in any value dump. DECISION (proposed) CV-39; reason: the caller owns contexts (CV-10), so the loader cannot
  create one behind the caller's back.
- **Unit moduli** are dumped as stored (as supplied, CV-17).
- **Version.** This is version 1, written `adf1`. A reader of version 1 rejects any other version with
  `ADF_UNSUPPORTED`. A later version must be able to read `adf1`.

## 11. Golden vectors

### 11.1 File format

DECISION (proposed) CV-46: the following format; reason: plain text, one vector per line, readable by C and Python
with a few lines of code, and able to carry any byte string.

- Files `tests/golden/*.tsv`, ASCII, LF line ends. A line starting with `#` is a comment; an empty line is ignored.
- A vector is `input<TAB>expected`, exactly one TAB.
- `expected` is the canonical output text, or `!` followed by a status name without `ADF_` (`!PARSE`, `!DOMAIN`,
  `!LIMIT`, `!UNSUPPORTED`, `!NOT_DETERMINED`).
- In `input` a backslash starts an escape: `\\` backslash, `\t` TAB, `\n` LF, `\r` CR, `\xHH` the byte with
  hexadecimal value `HH`. Any other backslash sequence is an error in the file.
- An input of the form `@gen:PREFIX|UNIT|COUNT|SUFFIX` (each part escaped as above; a literal `|` is `\x7c`) stands
  for `PREFIX` followed by `COUNT` copies of `UNIT` followed by `SUFFIX`; it is used for inputs longer than a line
  should be.

### 11.2 Files

| File | Input | Expected |
|---|---|---|
| `<type>.tsv` for the thirteen types of 9.2 | a text given to the parser of that type (limits of 8.4, defaults) | canonical text or status |
| `dispatch.tsv` | a text | the type name (`rat`, `fball`, ...) of 9.7, or a status |
| `realball_read.tsv` | a real ball in the value form | the exact interval as `lo hi` (two rationals in lowest terms), or a status |
| `realball_print.tsv` | `mid rad digits`, `mid` and `rad` dyadic rationals | the text printed by 9.5 |
| `dump.tsv` | a dump text | the same text if valid, or a status |

The counts are in `tests/golden/README.md`.

### 11.3 How a C test uses them

1. Status vectors: the status must be equal. The output must be untouched (4.3).
2. Types without real or complex parts: the printed text must be equal to `expected`.
3. Types with real or complex parts, read at `prec = 128`: every part that is not real or complex must print equal
   to the corresponding part of `expected`. For each real ball: if the input's decimal midpoint and radius satisfy
   the exactness condition of 9.5 at `prec = 128`, the printed ball must be equal to the expected one; otherwise the
   C `arb` must contain the exact interval of the input ball (as in `realball_read.tsv`), and its printed text,
   re-read exactly, must contain it too. The C test decides which case applies from the exact decimal it has read.
4. `realball_print.tsv`: set the `arb` exactly (every vector has a midpoint with at most 128 bits and a radius
   mantissa below `2^30`) and compare the printed text.
5. `dump.tsv`: load, then dump; the text must be identical; loading an invalid dump must leave the output
   untouched and return the status.

## 12. Foreign-function interface

Added on 2026-09-27 at the request of TJO: a Julia layer will call the C library through `ccall`. Each rule is a
contract of the public interface. Python stays for tests and proof checks only.

1. **Every public operation is an exported function.** No operation exists only as a macro or only as a
   `static inline` function. Inline fast paths may exist in addition, in the FLINT manner: the header defines
   `ADF_INLINE` as `static __inline__`, and one source file defines `ADF_INLINES_C` so that the same functions are
   also compiled as exported symbols (FLINT: `fmpz.h:15-19`; **[probed]** `nm -D libflint.so` lists `fmpz_init`,
   `fmpq_init`, `arb_init`, `arb_swap` as exported). Accessor macros (as `arb_midref`) have function equivalents.
   A test lists the exported symbols with `nm -D` and compares them with the header's function declarations.
2. **No variadic functions.** Printing takes explicit arguments; there is no `adf_printf`.
3. **Status is a plain `int` return value** with the numeric values of 3.1; predicates return `int` 0 or 1;
   comparisons return `int` with the values of 2.1. No status is returned through `errno` or a global.
4. **Structs.** Value types have a documented fixed layout (section 5), so that a binding can allocate them inline
   as Nemo.jl does for FLINT types. DECISION (proposed) CV-40: value structs are public with fixed layout, and the
   library exports `size_t adf_sizeof_<type>(void)` for every type, which a binding compares with its own layout at
   load time; reason: inline allocation avoids a heap object per number, and the size check turns a layout mismatch
   into an error at load time instead of memory corruption. The modulus context is **opaque**: its fields are not
   part of the interface; a binding allocates `adf_sizeof_modctx()` bytes and calls `init` and `clear`, or uses the
   exported `adf_modctx_t` in C. DECISION (proposed) CV-41; reason: the context holds internal tables (reduction
   constants, recombination tree) whose layout should be free to change.
5. **Contexts are explicit; no hidden global state** (4.5, 4.6). A binding must keep a context alive as long as any
   value refers to it (in Julia: the value object holds a reference to the context object).
6. **No callbacks.** Version 1 has no function-pointer parameters. If a later function needs one (an integrand), it
   takes a plain C function pointer and a `void *` user-data argument, never a closure.
7. **Strings.** Input: `(const char *, size_t)`, never retained (8.1). Output: `char *` from `flint_malloc`,
   freed by the caller with `flint_free` (`flint.h:204`) or with the exported `adf_str_free(char *)`, which calls
   it. DECISION (proposed) CV-43; reason: a binding need not locate FLINT's allocator.
8. **Arrays.** An array output is either caller-allocated with its length as the next argument, or owned by an
   output value (5.9 to 5.12) and freed by that value's `clear`. No function returns a bare heap array.
9. **Integer types.** `slong` and `ulong` are FLINT's `mp_limb_signed_t` and `mp_limb_t` (`flint.h:110-111`),
   64 bits on the supported platforms (`FLINT_BITS 64`, `flint.h:193`). DECISION (proposed) CV-42: version 1
   supports 64-bit platforms only; reason: places, blocks and primes are one word (CV-18), and the layouts below
   are stated for 64 bits. `size_t` is used for byte lengths only.
10. **FLINT types across the interface.** `fmpz`, `fmpq`, `arb`, `acb` and `padic` values may be passed by pointer,
    since Nemo.jl wraps the same types [unverified: Nemo.jl's FLINT version and layout were not read]. The interface
    assumes FLINT 3.0.1 (`flint.h:94-97`) and these layouts, measured on x86-64 Linux with the installed headers
    **[probed]**:

    | Type | Definition | Size (bytes) |
    |---|---|---|
    | `fmpz` | `typedef slong fmpz;` (`flint.h:510`); small values inline, large values a tagged pointer (`COEFF_IS_MPZ`, `fmpz.h:44`) | 8 |
    | `fmpq` | `{ fmpz num; fmpz den; }` (`flint.h:513-518`) | 16 |
    | `mag_struct` | `{ fmpz exp; mp_limb_t man; }` (`arb_types.h:21-26`) | 16 |
    | `arf_struct` | `{ fmpz exp; mp_size_t size; mantissa_struct d; }`, two inline limbs (`arf_types.h:22`, `:44-50`) | 32 |
    | `arb_struct` | `{ arf_struct mid; mag_struct rad; }` (`arb_types.h:32-37`) | 48 |
    | `acb_struct` | `{ arb_struct real; arb_struct imag; }` (`acb_types.h:21-26`) | 96 |
    | `padic_struct` | `{ fmpz u; slong v; slong N; }` (`padic.h:32-36`) | 24 |

    A binding must link against a FLINT whose `flint_version` string (`flint.h:105`) has the same major and minor
    version; the library exports `const char * adf_flint_version_compiled(void)` and checks `flint_version` against
    it in `adf_version_check(void)`, which returns `ADF_OK` or `ADF_UNSUPPORTED`. DECISION (proposed) CV-44; reason:
    a FLINT of another minor version may change these layouts.
11. **Memory of FLINT types inside our values** is FLINT's: our `clear` calls `fmpz_clear`, `arb_clear` and so on.
    A binding must not free the inner FLINT objects of an `adf` value itself.

## 13. Decisions (proposed), for review

| Id | Section | Decision | Reason (short) |
|---|---|---|---|
| CV-01 | 1 | statuses are ordered integers, not bit flags | one code per result |
| CV-02 | 2.1 | no `_equal` for ball types; `_identical` for representation | point equality is undecided |
| CV-03 | 3.1 | numeric values `OK 0` ... `LIMIT 10` in order of precedence | combination is a maximum |
| CV-04 | 3.3 | combined status is the maximum; place is the first in canonical order | proved failures dominate; deterministic place |
| CV-05 | 4.1 | outputs may alias inputs of the same type | FLINT habit; cheap |
| CV-06 | 4.3 | outputs untouched on every non-OK status | retry at higher precision |
| CV-07 | 4.3 | "enclosure with status" only at branch cuts; otherwise default and `_strict` variants | a status never carries a value except where `SPEC.md` asks |
| CV-08 | 4.4 | a non-finite result is never stored; `NOT_DETERMINED` | invariants need finite balls |
| CV-09 | 4.4 | non-canonical input is a precondition violation; checked in debug builds | gcd checks would dominate the kernels |
| CV-10 | 4.6 | contexts caller-owned with stated lifetime, no reference count | as FLINT; no shared writes; no hidden state |
| CV-11 | 4.6 | no operation creates a context; results that leave a context are global | a created context would have no owner |
| CV-12 | 4.6 | different context pointers give a global result | block comparison per operation is too costly |
| CV-13 | 5.2 | init of a finite ball is the exact 0 | as `fmpq_init`, `arb_init` |
| CV-14 | 5.4 | scaled value gets a field `exact` | the exact 0 of Proposition 6(2) must be stored |
| CV-15 | 5.6 | unit coset with `N = 0`, `c = 1` or `-1`, is an exact unit | "the exact 1" of `SPEC.md` 9.3.7 |
| CV-16 | 5.6 | unit residues in `1..N` | no residue 0 for units; `[1 mod 1]` |
| CV-17 | 5.6 | unit modulus stored as supplied; normal form for printing and equality | the dump keeps the modulus as supplied |
| CV-18 | 5.8 | primes and places are one word | certified primality; `ulong` places |
| CV-19 | 5.8 | init of a local ball is the exact 0 at `p = 2` | some prime must be chosen |
| CV-20 | 5.11 | no normal form for `adf_ffun`, `adf_rfun` | minimality is undecidable on balls |
| CV-21 | 5.14 | family contexts use prime-power blocks, increasing prime | factorisation known; prime-named operations |
| CV-22 | 5.3 | local backend keeps `A = 0` | an unused field cannot be misread |
| CV-23 | 5.4 | the absolute cap is an argument | only the radius is per value |
| CV-24 | 5.10 | order of quotient pieces | unique printed form |
| CV-25 | 8.1 | inputs are `(pointer, length)`; NUL inside is `PARSE` | no silent truncation |
| CV-26 | 8.2 | ASCII alphabet; everything else `PARSE` | no look-alike characters |
| CV-27 | 8.4 | limits `max_len`, `max_exp10`, `max_prec`, `max_items` with defaults | short input cannot demand huge work |
| CV-28 | 8.5 | fixed order of checks | status independent of parser internals |
| CV-29 | 9.5 | real-ball printing algorithm, 20 digits by default | exact, enclosing, idempotent |
| CV-30 | 9.7 | classification by syntax; typed parsers do not coerce | one text, one type |
| CV-31 | 9.8 | exact finite ball prints `(* ; 7/3)` | no `mod 0` in output |
| CV-32 | 9.8 | bare `a mod N` accepted as finite ball | natural input |
| CV-33 | 9.8 | idele scale printed even when 1 | uniform template |
| CV-34 | 9.8 | local ball printing: canonical centre, explicit exponent, exact without `O` | unique text |
| CV-35 | 9.2 | forms for `cadele`, `sball`, `qclass`, `ffun`, `rfun`, `char` | not in `PLAN.md` 5 |
| CV-36 | 9.8 | leading zeros accepted, never printed | friendly input, unique output |
| CV-37 | 10.1 | dump grammar: one token per field, hexadecimal | direct transcription |
| CV-38 | 10.2 | strict loader, never canonicalises | identical object |
| CV-39 | 10.2 | loader uses the caller's context, checks its blocks | caller owns contexts |
| CV-40 | 12 | value structs have fixed layout; `adf_sizeof_<type>` exported | inline allocation, checked layout |
| CV-41 | 12 | the modulus context is opaque | internal tables may change |
| CV-42 | 12 | 64-bit platforms only | word places and blocks |
| CV-43 | 12 | `adf_str_free` exported | bindings need not find FLINT's allocator |
| CV-44 | 12 | FLINT version check exported | layouts may change between minor versions |
| CV-45 | 5.10 | quotient pieces: invariant on the midpoint of the real part | outward rounding |
| CV-46 | 11.1 | golden file format | plain, escapable |

## 14. Findings against the specification and the plan

- **F1** (`SPEC.md` 5, `PLAN.md` 4 against `SPEC.md` 9.3.7 and `proofs/catalogue.md` Proposition 12). The unit
  coset is defined with `N >= 1`, but the profinite power with exponent 0 returns "the exact 1", and the idele of an
  exact rational has an exact sign unit. With `N >= 1` neither is representable. Proposed: CV-15.
- **F2** (`SPEC.md` 6). "Pieces are closed intervals inside `[0, 1]`" cannot hold for `arb` enclosures of such
  intervals when an end point is not dyadic: **[probed]** the enclosure of `[0.9, 1]` built as
  `arb_set_fmpq(19/20)` plus `arb_add_error(1/20)` has upper end above 1 at `prec` 20, 53 and 128 (the radius is a
  30-bit number rounded up, `mag.h:117`).
  Proposed: CV-45 (invariant on the midpoint), to be settled with `docs/proofs/quotient.md`.
- **F3** (`PLAN.md` 4). `adf_scaled_struct` has no way to hold an exact value, which Proposition 6(2) of
  `proofs/precision.md` produces (the exact 0). Proposed: CV-14.
- **F4** (`SPEC.md` 4.1, "or is given a new context"). With caller-owned contexts (CV-10) no operation may create
  a context; the option is not used in version 1 (CV-11). Not an error of the specification, a narrowing.
- **F5** (FLINT 3.0.1, relevant to `PLAN.md` 1.4 "invalid and huge inputs"). `arb_load_str` aborts the process on
  some malformed strings and silently changes others (**[probed]**, 10.2). The dump loader must validate first;
  fuzzing the C loader must not reach `arb_load_str` with unvalidated text.
- **F6** (`PLAN.md` 5, implicit). The value form of a real ball is not only lossy on input (as stated) but also on
  output: printing rounds the radius up to two significant digits, so a text read and printed again can change
  (`0.625 +/- 0.25` prints as `0.62 +/- 0.26`), and a printed idele could contain 0 unless the printer takes the
  sign condition into account. Handled by 9.5 (constrained printing) and by the fixed-point property; recorded here
  because `PLAN.md` 5 calls the value form "canonical" without saying that it is canonical only after one printing.
