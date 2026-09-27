
**Form of every proof file.** Numbered definitions, lemmas, propositions; each statement names its hypotheses
completely (which `p`, which ranges, zero and negative cases, `p = 2` separately wherever it differs). Proofs are
stepwise: each step follows from named earlier steps or from a named standard theorem. A standard theorem that is
used and not proved is stated exactly and marked `[source pending: ...]` (the sources lane is fetching texts to
`refs/` in parallel; do not wait for it). After each statement: "Check:" with the name of the function in your
proto file that tests it numerically, and "Used by:" with the section of `docs/SPEC.md`. End the file with a table
of all statements: number, one-line content, status (proved here / proved modulo the named standard theorem /
open), check. A statement you cannot prove is marked open with the reason; an open statement honestly marked is
worth more than a gap hidden in a proof.

**Checks.** The proto file is plain Python 3 (standard library; `python-flint` and `mpmath` are importable). It
must test the statement itself, by an independent computation (enumeration in small quotients, exact rational
arithmetic, truncated series with proved tail bound), not an identity that a wrong formula also satisfies. It
prints one line per check with counts and exits non-zero on any failure. Total running time under 3 minutes.

