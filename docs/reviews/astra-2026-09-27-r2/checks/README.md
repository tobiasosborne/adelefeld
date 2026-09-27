# Round-2 checks

All commands below run from the repository root. The review labels mathematical proofs separately from these runs.

- `prototype.txt`: output from the unchanged `proto/precision_rules.py`; 6,000 arithmetic checks and 40,000 scaled-policy checks passed.
- `math_checks.py` / `math_checks.txt`: stdlib-only exact rational truncations with the tail bounds stated in N3; convergence-boundary witnesses, local root-residue checks, a fractional CRT neighbourhood, and the prototype's unsupported zero-radius predicates.
- `flint_probe.c` / `flint_probe.txt`: compiled against installed FLINT 3.0.1; elementary kernel statuses, output precision versus input uncertainty, root choice, Teichmüller units, type sizes, allocation usable bytes and a hardware-cycle event request. No benchmark timings were rerun.
- `padic-header.txt`: numbered excerpts from the installed header; `input-sha256.txt` identifies the reviewed documents, supplied harness/results, prototype and FLINT headers.
- `baseline_current_O2.s`: current historical harness compiled with installed GCC 13.3.0 at `-O2`; source now differs from the harness that generated the dated timing file. This assembly is not represented as the original benchmark binary.
- `performance.txt`: arithmetic of conditional ratios and layout/traffic counts; no timings.
- `verification.txt`: review structure, input hashes and absence of ELF artifacts checked at completion.

Reproduction:

```sh
python3 proto/precision_rules.py
python3 docs/reviews/astra-2026-09-27-r2/checks/math_checks.py
gcc -O2 docs/reviews/astra-2026-09-27-r2/checks/flint_probe.c -o docs/reviews/astra-2026-09-27-r2/checks/flint_probe -lflint -lgmp
./docs/reviews/astra-2026-09-27-r2/checks/flint_probe
rm docs/reviews/astra-2026-09-27-r2/checks/flint_probe
gcc -O2 -S bench/baseline.c -o docs/reviews/astra-2026-09-27-r2/checks/baseline_current_O2.s
```

Source retrieval, checked during this review:

- Installed `/usr/include/flint/padic.h` and `flint.h` are the version-specific API evidence. The probe is the evidence for installed behaviour.
- Web search retrieved excerpts of the official [FLINT 3.0.1 manual](https://flintlib.org/download/flint-3.0.1.pdf), including exponential and Teichmüller documentation (§12.1, printed pp. 957, 959), and the [current padic documentation](https://flintlib.org/doc/padic.html). Full browser PDF retrieval did not succeed. Current documentation is not presented as the installed version's source code.
- Direct downloads of five pinned GitHub files (`src/padic/{log,exp,sqrt,teichmuller}.c`, `doc/source/padic.rst`, tag `v3.0.1`) failed with `Temporary failure in name resolution`. No pinned-source audit is claimed.
- Primary uops.info [MUL](https://www.uops.info/html-instr/MUL_R64.html#ZEN2) and [VPMULUDQ](https://uops.info/html-instr/VPMULUDQ_YMM_YMM_YMM.html#ZEN2) Zen 2 sections were read. Immediate IMUL was not independently verified; its latency remains an explicit model assumption in the review.
- Keith Conrad's [Infinite series in p-adic fields](https://kconrad.math.uconn.edu/blurbs/gradnumthy/infseriespadic.pdf) was found in primary-author search results. The review supplies its own convergence and precision arguments and does not claim to have audited this whole document.

Compiled executables were removed after each run. The scripts do not import the repository prototype as a module or create repository bytecode caches.
