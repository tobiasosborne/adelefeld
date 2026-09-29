# Lane s2p-review: adversarial review of the search for all roots at a prime (S.2 slice 2)

Your task is to REFUTE, not to confirm. The code under review is the part of `src/roots.c` that lane
s2-slice2 added (Algorithm P: `adf_roots_padic`, `adf_roots_padic_partial`,
`adf_rootlist_get_unresolved`, `adf_rootlist_verify_complete`, with their static helpers), written by
Claude Opus. Its contract: `include/adelefeld/roots.h`; `docs/api-s.md` section 4 with its notes;
`docs/proofs/solvers.md` section 3 (Propositions 3.4 to 3.7, Algorithm P, 3.11, 3.13); decisions S-D10,
S-D13, S-D15, S-D17, S-D18 in `docs/SPEC.md` 15.3; the author's report `lanes/s2-slice2/result.md`; the
earlier review `docs/reviews/s2/review.md` (do not repeat its findings).

**You own:** `lanes/s2p-review/` only. Everything else is read-only. No git, no `bd`. At most 2 cores. Run
every program under `timeout`. Build the library with `make -j2` and link your own programs against
`build/libadelefeld.a`.

## What counts as a finding

- A list with `complete = 1` that misses a root of `f` in `Z_p`, or lists a ball with no root or with two
  roots of `g`, or two balls of one root. Your own oracle: enumerate the `x` modulo `p^M` with `f(x) = 0`
  modulo `p^M` and decide by your own argument which of them come from roots in `Z_p` (say which `M` you
  use and why it suffices); use polynomials with planted roots whose `p`-adic distances you know.
- A partial list in which some root of `f` in `Z_p` lies in no ball and no class, or in two.
- `adf_roots_padic` returning `OK` where the partial function has an unresolved class, or
  `NOT_DETERMINED` where it has none; a `NOT_DETERMINED` at a depth at or above the bound of
  Proposition 3.6.
- `adf_rootlist_verify_complete` accepting a list that is not complete, or that was changed by hand
  (build lists field by field); or refusing a result of the library at the depth that produced it.
- An abort (find an input that reaches the abort of `newton_step` through the search: the author reports
  that two of his deliberate faults were caught only by that guard, not by a test), an overflow, an
  allocation or a loop of `p` steps before `UNSUPPORTED`, `LIMIT` or `DOMAIN` is decided, a running time
  that the header does not admit (a search tree that grows beyond the bound of Proposition 3.5), a leak,
  `L` written on a status other than `OK`.
- A sentence of the header that the code does not keep; a test of `tests/test_roots_padic.c` that
  cannot fail; a statement of `docs/proofs/solvers.md` section 3 that is false.

Places to attack first: `p = 2`; roots congruent modulo a high power of `p` (`X (X - p^20)`,
`(X - 1)(X - 1 - p^7)(X - 1 + p^7)`); a root that is a multiple root of `f` and simple in `g`; content and
leading coefficient divisible by `p`; polynomials with no root modulo `p`, with every residue a root
modulo `p` (`X^p - X` and its multiples plus `p h`); degree above `p`; depth 0; `prec_p` far above and far
below `s + 1`; `p` just below `2^20` (time); the order of the output when centres change with the
precision.

## Report

`lanes/s2p-review/report.md`, written once, at the end. For each finding: severity (BLOCKER: a wrong or
incomplete list called complete, a false list accepted, a memory fault; MAJOR; MINOR), the input, what
the code returns, what is true and why, and the command that reproduces it with a program in your lane
directory. Then: what you attacked without result, with the number of cases and what would have made a
case fail. No praise, no summary of the code.
