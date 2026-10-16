#!/bin/sh
# The session that docs/assets/terminal.svg types out. Run from the repository root after the build:
#   sh lanes/docs-readme/terminal_session.sh
# Each command is printed after "$ ", then its output, exactly as a shell shows them.
run() { printf '$ %s\n' "$1"; sh -c "$1"; }
run "echo 'add (0 ; 0) with 22/7' | build/adf"
run "echo 'project 22/7 with real 2 3 5 7' | build/adf"
run "printf 'prec 6\\nroots_at -1 with 5 with 2\\n' | build/adf"
run "echo 'roots_at -1 with 3 with 2' | build/adf"
run "echo 'mul (3.0 +/- 0.1 ; 146 mod 1000) with 7' | build/adf"
run "echo 'sin_at (* ; 0 mod 2) with 2' | build/adf"
run "echo 'recover 146 mod 1000 with 30 with 15' | build/adf"
