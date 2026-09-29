# f-slice1 progress
- [x] read CLAUDE.md, COMMON*, workflow, brief, conventions 2-5.9,7,12, functions.md Prop 4, precision.md, d-functions notes
- [x] header include/adelefeld/lball.h written, included in adelefeld.h
- [x] docs/api-1f.md statements L0..L8
- [x] python reference in proto/functions_checks.py (check_lball_* pass), lanes/f-slice1/gen_vectors.py, 4 vector files
- [x] tests/test_lball.c red (link error, then stub assertions), src/lball.c green: 20 tests, 3937482 checks, 0 failed
- [x] bite.py: 13 planted faults, 13 caught (one needed a new test)
- [ ] tests/julia/lball.jl + test_julia.sh line
- [ ] final checks (check-all, SAN, clang, headers), result.md
