# Dump review checks

Run from the repository root. No check changes implementation files.

Use `python3 -B docs/reviews/m1/dump/checks/verify.py PHASE` with phases:
`build`, `unit`, `diff`, `memory`, `san`, `cost`, `near-modctx`, `near-qclass`.
Run build first, then one phase at a time. The cost phases run one measured constructor at a time.
Run `finish.py` last to remove build directories and binaries and write the command inventory.

The phase runner records each child command and its result. Its own exit code does not mean all child
checks passed. The differential assertion currently fails with 28 reference-limit mismatches. The unchanged
fuzz target aborts on a valid seed. The status and cost scripts print the contract violations they expose.

The verified command ledger and prefixed logs are evidence for the current review. Earlier unprefixed logs
and run.sh were present before this continuation. They are retained but not used as evidence. Two early
compilation logs reused the names verified-bridge.log and verified-roundtrip.log; the runtime results are
preserved in verified-san-bridge.log, verified-san-roundtrip.log and the corresponding Valgrind logs.

The independent predicates in differential.py use Python integer arithmetic. Reference parsing is used to
select syntax-valid texts; reference semantic predicates do not supply the independent expected values.
The constructor differential covers 15 bodies; typed inspection and round trips cover the five implemented
dump value types. The byte budget search establishes the slowest of its measured candidates, not a global
maximum of parser cost.
