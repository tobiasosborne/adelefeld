# f-slice2 progress (lane worker)

- [x] read the rules, brief, headers, conventions 3, 4, 5.8, 5.9, 7, SPEC 9.3, api-1f.md, arb.rst
- [x] design: `adf_sball_*` (sball.h), `adf_real_*` and `adf_sball_<f>_at` (rfunc.h); places as an array plus n;
  canonical order of places per conventions 7 (inf first) overrides "real last" of the brief
- [x] headers sball.h, rfunc.h (+ include/adelefeld.h)
- [x] Python reference (proto/functions_checks.py, section f-slice2), gen_vectors.py, vectors (949824 bytes, 4 files)
- [x] tests: tests/test_sball.c (16 tests), tests/test_rfunc.c (8 tests); RED 1 (link), RED 2 (stub): redgreen.log
- [x] src/sball.c, src/rfunc.c: green; INV=1 green
- [x] tests/julia/sball.jl (35 tests) + tests/test_julia.sh
- [x] bite.py: 22 faults, 20 caught, 2 equivalent (bite.log); plant_ref.py: 4 of 4 planted faults in the reference rejected
- [x] docs/api-1f.md: section for the slice (S1 to S7)
- [ ] make clean && make check-all; SAN=1; CC=clang; check_headers (last lines into the logs)
- [ ] result.md
