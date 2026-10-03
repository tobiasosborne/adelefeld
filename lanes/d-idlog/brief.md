# Lane d-idlog: design and proofs of `Log` on ideles (milestone 1F, WP 1F.8, third part), before any code

PLAN row 1F.8 (`docs/PLAN.md` line 278) asks for "`Log` on ideles as the enclosure `4 Zhat` refined at named
primes". SPEC 9.3.2 lines 598-600: "`Log` at all places: the finite image of any idele lies in `4 Zhat`, which is
the conservative result; it is refined at finitely many named primes by the table above. The real coordinate
needs a positive input, or the separately named `log_abs`." Proposition 12 (`docs/proofs/functions.md:375-408`;
line 386: "For any idele, its finite Iwasawa Log belongs to D, hence to 4 Zhat"; step 4, lines 402-406).

Nothing of this exists in code, and two things are not decided (questions 8 and 9 of
`lanes/f-slice10/brief-draft.md`, section 2; read sections 1.2, 1.4, 1.6 of that file for the facts with file and
line, and open every citation you rely on yourself):
- No header gives the local component of an idele at a prime as a local ball. An idele is a real ball, a content
  `r` (a positive rational) and a unit coset `c U(N)` (`include/adelefeld/idele.h` lines 1-24, `ucoset.h`;
  Definition 4 and Lemma 5, `docs/proofs/ideles.md:86-110`; the factor-2 rule, Lemma 7, `ideles.md:132`). At a
  prime `p` dividing `N` the component is the ball `r c + p^(v_p(r) + v_p(N)) Z_p`; at a prime not dividing `N`
  it is `r Z_p^x`, which is NOT a ball of the library's kind (`adf_lball` is `a + p^M Z_p`), and the hull of
  `idmap.h:53` contains non-units there, so `adf_lball_Log` on it is `NOT_DETERMINED` although the image of
  `Log` on `r Z_p^x` is known exactly.
- The result type and the real coordinate: an adele `(log of the real ball ; 0 + 4 Zhat)`? a partial ball over
  the named primes? `DOMAIN` for a negative real ball, or `log_abs`?

Milestone S showed that a review of a design before code finds the defects cheaply (`docs/workflow.md`, table).
You write the design, prove it, and build its oracle. You write NO C code and change no header.

**You own:** `docs/design/idele-log.md` (new), `proto/idlog_checks.py` (new; exact integers only: no p-adic
library, no floating logarithm for the finite part), `lanes/d-idlog/`. Everything else is read-only. No git
command that changes state, no `bd`. At most 2 cores; every program under `timeout`; none over 120 s. `refs/src/`
is on disk. Another lane (f-slice10) is writing `include/adelefeld/gfunc.h`, `src/gfunc.c`, `docs/api-1f8.md`
(the root and the five series at all places) in another worktree: your design must fit beside it (the same
header, the names in its style: read `lanes/f-slice10/brief.md`), and you do not write those files.

Read first: `CLAUDE.md`, `docs/workflow.md`, `lanes/COMMON.md`, `lanes/f-slice10/brief-draft.md` and `brief.md`,
`docs/SPEC.md` 5 (unit precision), 9.3.1, 9.3.2 (lines 574-600), 15.4 (N-D8, N-D9, N-D10, N-D14),
`docs/proofs/functions.md` Lemma 9, Propositions 10, 11, 12, 22 (and the definition of the Iwasawa `Log`),
`docs/proofs/ideles.md` Definition 4, Lemmas 5 to 7, `docs/conventions.md` 2.1, 3.1 to 3.3, 5.6, 5.7, 5.8, 5.9,
`include/adelefeld/idele.h`, `ucoset.h`, `idmap.h`, `lball.h`, `lfunc.h` (`adf_lball_Log` and its exponent rule
F6: `docs/api-1f4.md`), `sball.h`, `rfunc.h` (lines 91-123: `Log_at`, `log_abs_at`, the real `log`),
`docs/api-1f4.md` and `docs/api-1f6.md` (the style of statements with proofs and "Check:" lines),
`proto/functions_checks.py:433` (`check_global`), `lanes/f-review6/oracle.py` (an oracle in exact integers).

## What `docs/design/idele-log.md` contains

1. **The local component of an idele** at a prime `p`, as a SET, for every case: `p | N` (with `k = v_p(N)`), `p`
   not dividing `N`, the exact unit (`N = 0`: `[1]`, `[-1]`), the prime 2 with the factor-2 rule of the normal
   form (`N != 2 mod 4`). A statement with proof (from Lemma 5), and the form in which the library can hold it
   (a local ball where it is one; what where it is not).
2. **The image of `Log` over that set, exactly.** Statements with complete proofs, steps numbered, each step
   checkable:
   - `p | N`, odd `p`, `k >= 1`: the image is `Log(r c) + p^k Z_p` (the table of SPEC 9.3.2 with `N - m = k`);
     prove it from Proposition 11 or directly, and say what happens to `Log(r)` (`Log` is additive and
     `Log(p) = 0`: state exactly which part of `r` contributes).
   - `p = 2`: `k >= 2`, `k = 1` cannot occur in the normal form (check), `k = 0`; the table says the image for
     relative precision 1 is `4 Z_2`.
   - `p` not dividing `N`: the image of `Log` on `r Z_p^x` is `Log(r') + p Z_p` for odd `p` and `Log(r') + 4 Z_2`
     at 2, where `r'` is the part of `r` prime to `p` (prove surjectivity of `log` from `1 + p Z_p` onto `p Z_p`,
     and from `1 + 4 Z_2` onto `4 Z_2`, or cite the proposition that has it with file and line; what is
     `Log(r')` for a rational `r'` prime to `p`: a non-zero element of `p Z_p` in general, so the image is the
     COSET-free statement `p Z_p` only because `Log(r')` lies in `p Z_p`: say so precisely).
   - Consequence: the finite image of every idele lies in `D`, hence in `4 Zhat` (Proposition 12), and at the
     primes dividing `N` it is the smaller set above. State what "the enclosure `4 Zhat` refined at named primes"
     is as a set, and whether a finite ball `0 + 4 Zhat` of the library (`adf_fball`) holds the unrefined result.
3. **The interface**, as declarations with full comment blocks in the style of `lfunc.h` and `rfunc.h` (in the
   design document, NOT in a header): the all-places form and the form refined at named primes; types of input
   and output; `prec` at the real place and `N` at the primes (N-D10, N-D14: `K = min(N, E)`); every status with
   the input that gives it and `where`; the real coordinate (a negative real ball: `DOMAIN` at the real place for
   `Log`, and whether a `log_abs` variant is offered: SPEC line 599-600 names it); aliasing; limits. For each
   decision the alternatives and your recommendation with the reason (the orchestrator decides and records it in
   SPEC 15.4). Say also whether a public function "the local component of an idele at `p`" should exist, what it
   returns at a prime not dividing `N`, and which existing function (`adf_sball_project` for adeles) it mirrors.
4. **The oracle** `proto/idlog_checks.py`, written and RUN: for `p` in 2, 3, 5, 7, contents `r` with positive and
   negative valuations, moduli `N` with `v_p(N)` from 0 to 4 and other prime factors, residues `c`: the set
   `{Log(r u) mod p^H : u in c U(N) at p}` by enumeration of the units modulo `p^H'` (`Log` computed in exact
   integers: for example `log u = log(u^(p-1)) / (p-1)` with the series on `1 + p Z_p` truncated with a proved
   tail bound, or by the characterisation `exp`/`log` are inverse isometries; state the precision `H` of every
   comparison and prove that `H'` suffices), compared with the statements of part 2: equality of sets, not only
   inclusion (the smallest ball). Give the counts and what would have made a case fail. The implementation lane
   will use this file as its oracle: give it a function that returns the expected ball for `(p, r, c, N, K)`.
5. **The test plan for the implementation lane**: the fixtures to generate from the oracle, the statuses to
   cover, eight faults to plant (for example: `v_p(N)` taken as the exponent without `v_p(r)`; the image at a
   prime not dividing `N` taken as `Z_p`; `Log(r)` dropped; the case `p = 2`, `k = 0` given `2 Z_2`), the driver
   commands and the Julia example a user would run.
6. **Open points for TJO**, at most five, each with the alternatives and what you recommend.

## Report

`lanes/d-idlog/report.md`, written once, at the end; running notes in `lanes/d-idlog/progress.md` as you go (a
note after every statement proved, so that an interruption loses little). In the report: the statements proved
(one line each) and the ones you could not prove as the SPEC or the draft suggests them, with the counterexample;
the oracle's runs with commands and numbers; the recommended interface in ten lines; findings against the
specification or the proofs (a false sentence of SPEC 9.3.2, Proposition 12 or Lemma 5 is a finding with its
counterexample); sources pending. No praise, no summary of what the files already say.
