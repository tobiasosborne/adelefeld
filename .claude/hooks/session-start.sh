#!/bin/bash
# SessionStart hook of adelefeld for Claude Code cloud sessions (remote containers only).
# Installs what the build, the tests and the lanes need and the container lacks: FLINT 3.0.1 (the Makefile
# links -lflint -lgmp -lm; adelefeld/common.h refuses any other 3.x minor), pdftotext (refs/fetch_sources.sh),
# the Python libraries the oracles may use, Julia (tests/test_julia.sh skips without it), and the sources under
# refs/src/ (gitignored; CLAUDE.md rule 4: ground truth is the local copy). Idempotent: every step is skipped
# when its result is present. Synchronous: the session starts after it has finished.
# The beads tracker (bd) is not installed: its data is an embedded Dolt database under .beads/dolt/, which is
# not in the clone, so a fresh bd would show an empty tracker (HANDOFF.md, session of 2026-10-02 evening).
set -euo pipefail

if [ "${CLAUDE_CODE_REMOTE:-}" != "true" ]; then
  exit 0
fi

REPO="${CLAUDE_PROJECT_DIR:-$(cd "$(dirname "$0")/../.." && pwd)}"
cd "$REPO"
export DEBIAN_FRONTEND=noninteractive

# 1. FLINT 3.0.1 and pdftotext.
if [ ! -f /usr/include/flint/flint.h ] || ! command -v pdftotext >/dev/null 2>&1; then
  apt-get install -y -q libflint-dev poppler-utils >/dev/null
fi
grep -q '__FLINT_VERSION_MINOR 0' /usr/include/flint/flint.h || echo "session-start: WARNING: FLINT is not 3.0.x" >&2

# 2. Python libraries of the oracles (lanes/COMMON.md rule 3 names these three).
python3 -c 'import flint, sympy, mpmath' 2>/dev/null || pip install -q python-flint sympy mpmath

# 3. Julia through juliaup, linked into the PATH for scripts that call `julia`.
if ! command -v julia >/dev/null 2>&1; then
  if [ ! -x "$HOME/.juliaup/bin/julia" ]; then
    curl -fsSL https://install.julialang.org -o /tmp/juliaup-install.sh
    sh /tmp/juliaup-install.sh -y --default-channel release >/dev/null
  fi
  ln -sf "$HOME/.juliaup/bin/julia" /usr/local/bin/julia
fi

# 4. The sources (about 81 MB; refs/fetch_sources.sh skips what is present and verifies the manifests).
if [ ! -d refs/src/flint-3.0.1 ] || [ ! -d refs/src/tate-poonen ]; then
  (cd refs && ./fetch_sources.sh >/dev/null 2>&1) || echo "session-start: WARNING: refs/fetch_sources.sh failed; sources incomplete" >&2
fi

FLINT_VER=$(awk '/define __FLINT_VERSION / {a=$3} /define __FLINT_VERSION_MINOR / {b=$3} /define __FLINT_VERSION_PATCHLEVEL / {c=$3} END {print a"."b"."c}' /usr/include/flint/flint.h)
echo "session-start: FLINT $FLINT_VER, julia $(julia --version 2>/dev/null | cut -d' ' -f3 || echo missing), refs/src $(ls refs/src 2>/dev/null | wc -l) keys"
