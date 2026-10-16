# The documents of adelefeld

adelefeld is a C library on FLINT 3.0.1 that computes with the adeles of the rational numbers as a number type:
exact rationals, finite balls `a + N Zhat` (all numbers congruent to `a` modulo `N`, at every prime at once),
real and complex balls, and functions and integrals built on them. This page is a map of `docs/`: where to begin,
and where each kind of document is. Paths in the tables are relative to `docs/`.

## Start here

| Document | What it is for |
|---|---|
| [GUIDE.md](GUIDE.md) | the guide for a reader who has never met an adele |
| [../README.md](../README.md) | the front page: what the library does, how to build it |
| [../tools/adf/README.md](../tools/adf/README.md) | the manual of the driver `build/adf` |
| [../julia/Adelefeld/README.md](../julia/Adelefeld/README.md) | the Julia package and its tutorial |
| [ux/README.md](ux/README.md) | four visions of an interface |

## What is built, and the rules it follows

| Document | What it holds |
|---|---|
| [SPEC.md](SPEC.md) | the specification: values, precision, ideles, `A/Q`, functions, Tate integrals, decisions |
| [conventions.md](conventions.md) | canonical forms, ownership, statuses, signs, text and dumps, the C interface |
| [PLAN.md](PLAN.md) | the order of the work: milestones and work packages |
| [PERF.md](PERF.md) | how speed is judged: space and time floors, the benchmark contract |
| [seams.md](seams.md) | version 1 tested against `Q(sqrt(-5))` and `F_3(T)` (a draft) |
| [sources.md](sources.md) | the sources under `refs/src/`: fetching and citing |

## The interface, statement by statement: `api-*.md`

Each milestone has a design file written before the code and one or more files that state what a slice of
the code adds, with the statements its tests check. The contract of a C function is the comment above its
declaration in `include/adelefeld/`; these files map the functions and prove what the headers claim.

| Milestone | File | What it covers |
|---|---|---|
| 1 | [api-m1.md](api-m1.md) | milestone 1: rationals, finite balls, scaled values, places, adeles, text, dumps |
| S | [api-s.md](api-s.md) | milestone S, the proposal: reconstruction from a residue, linear systems, roots |
| 1F | [api-1f.md](api-1f.md) | milestone 1F: local and partial balls, real functions, units, powers, local text |
| 1F | [api-1f4.md](api-1f4.md) | slice 1F.4-a: `exp`, `log` and `Log` at a prime (F1 to F7) |
| 1F | [api-1f5.md](api-1f5.md) | slice 1F.5: roots at a prime, `lroot.h` (R1 to R9) |
| 1F | [api-1f6.md](api-1f6.md) | slice 1F.6: rational powers at a prime, `lpow.h` (P1 to P9) |
| 1F | [api-1f8.md](api-1f8.md) | slice 1F.8: functions at all places, `gfunc.h` (G1 to G6) |
| 1F | [api-1f9.md](api-1f9.md) | slice 1F.9: Legendre, Jacobi, Kronecker, Hilbert symbols; local zeta factors |
| 2 | [api-2.md](api-2.md) | milestone 2: unit cosets, ideles, idele classes, norms, powers, hulls, text |
| 3 | [api-3.md](api-3.md) | milestone 3, the design: sets in `A/Q` and the additive character |
| 3 | [api-3a.md](api-3a.md) | slices 3.1-a to 3.1-f: lifts in `A/Q`, reduction, sums of classes |
| 3 | [api-3b.md](api-3b.md) | slice 3.2-a: the additive character psi |
| 3 | [api-3c.md](api-3c.md) | 3.3 and 3.4, the design: characters and Gauss sums |
| 3 | [api-3d.md](api-3d.md) | 3.3 and 3.4 as built: characters, Gauss sums, root numbers |
| 4 | [api-4.md](api-4.md) | milestone 4, the design: test functions, Poisson summation |
| 4 | [api-4a.md](api-4a.md) | finite test functions `adf_ffun`: sum, transform, product |
| 4 | [api-4b.md](api-4b.md) | slice 4d: real test functions `adf_rfun` |
| 4 | [api-4c.md](api-4c.md) | slice 4f: evaluation and integrals of test functions |
| 5 | [api-5.md](api-5.md) | milestone 5, the design: Tate integrals |
| 5 | [api-5s.md](api-5s.md) | slices 5a to 5e: local and global Tate integrals, continuation, functional equation |
| 5 | [api-5a.md](api-5a.md), [api-5b.md](api-5b.md) | stubs: moved into `api-5s.md` |
| J | [api-j.md](api-j.md) | the Julia binding, the design; section 9 is the order of the steps |
| J | [api-ja.md](api-ja.md) | the Julia binding as built, step by step |

## Proofs: `proofs/`

| File | Topic |
|---|---|
| [proofs/precision.md](proofs/precision.md) | arithmetic of finite balls (SPEC 4.3) |
| [proofs/policies.md](proofs/policies.md) | precision policies (SPEC 4.4), local backend |
| [proofs/quotient.md](proofs/quotient.md) | `A/Q` (SPEC 6), reconstruction (9.2) |
| [proofs/ideles.md](proofs/ideles.md) | ideles and classes (SPEC 5) |
| [proofs/functions.md](proofs/functions.md) | functions (SPEC 9.3.1 to 9.3.6) |
| [proofs/catalogue.md](proofs/catalogue.md) | the catalogue (SPEC 9.3.7) |
| [proofs/analysis.md](proofs/analysis.md) | analysis (SPEC 5 to 8) |
| [proofs/solvers.md](proofs/solvers.md) | solvers (SPEC 9.1, 9.2) |

## Design notes: `design/`

| File | Topic |
|---|---|
| [design/idele-log.md](design/idele-log.md) | `Log` on ideles (1F.8) |
| [design/local-zeta.md](design/local-zeta.md) | local zeta factors (1F.9) |
| [design/real-roots.md](design/real-roots.md) | real roots by exact isolation |

## Reviews: `reviews/`

Each directory holds the records of the reviews of one milestone or design, most of them adversarial: the
reviewer's findings, often with the reviewer's own checks. A record is kept as written; the repairs are in the
worklog and in the files reviewed.

| Directory | What was reviewed |
|---|---|
| [reviews/astra-2026-09-27/](reviews/astra-2026-09-27/) | the first design, round 1 |
| [reviews/astra-2026-09-27-r2/](reviews/astra-2026-09-27-r2/) | the first design, round 2 |
| [reviews/astra-2026-09-27-r3/](reviews/astra-2026-09-27-r3/) | the first design, round 3 |
| [reviews/m0-proofs/](reviews/m0-proofs/) | the proofs of milestone 0: analysis, catalogue, functions, ideles |
| [reviews/m0-gate/](reviews/m0-gate/) | the gate of milestone 0 and its closure |
| [reviews/m1/](reviews/m1/) | milestone 1: arithmetic, contexts, dumps, the local backend, the surface, text |
| [reviews/s-design/](reviews/s-design/) | the design of milestone S and its closure |
| [reviews/s1/](reviews/s1/), [reviews/s2/](reviews/s2/) | slices of milestone S |
| [reviews/s3/](reviews/s3/), [reviews/s13/](reviews/s13/) | slices of milestone S |
| [reviews/f1/](reviews/f1/) | milestone 1F: local balls, functions, roots, powers, symbols, the catalogue |
| [reviews/m2/](reviews/m2/) | milestone 2: slices 2 and 3, the dumps of units, repairs |
| [reviews/m3-design/](reviews/m3-design/) | the design of milestone 3 |
| [reviews/m3/](reviews/m3/) | milestone 3: quotient lifts and reduction |
| [reviews/m4/](reviews/m4/) | milestone 4: functions on the adeles |
| [reviews/m5/](reviews/m5/) | milestone 5: Tate integrals |
| [reviews/r1/](reviews/r1/) | the real roots: the isolation and a bug hunt through its second slice |

## Visions of the interface: `ux/`

| File | What it is |
|---|---|
| [ux/README.md](ux/README.md) | the four visions and the decision D-UX0 to build a first slice of each |
| [ux/brief-1-adelica-notebook.md](ux/brief-1-adelica-notebook.md) | a notebook of place strips (`ux/notebook`) |
| [ux/brief-2-place-atlas.md](ux/brief-2-place-atlas.md) | a zoomable map of all places (`ux/atlas`) |
| [ux/brief-3-claim-ledger.md](ux/brief-3-claim-ledger.md) | claims proposed, sealed, ratified (`ux/ledger`) |
| [ux/brief-4-sweep-bench.md](ux/brief-4-sweep-bench.md) | a wall of places run by agents (`ux/bench`) |

Each brief has a static mock, `ux/mock-<n>-*.html`, to open in a browser.

## State of the work and how it is run

| Document | What it holds |
|---|---|
| [../HANDOFF.md](../HANDOFF.md) | the live state and the next steps |
| [worklog/](worklog/) | one record per session; newest [2026-10-16-night](worklog/2026-10-16-night.md) |
| [workflow.md](workflow.md) | how the work is run, and what is not done any more |
| [../lanes/COMMON.md](../lanes/COMMON.md) | the rules for every parallel lane |
