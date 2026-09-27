#!/usr/bin/env bash
# Launch the codex gpt-6-astra xhigh design review, detached. Output: astra.stdout, astra-last.md, review.md.
cd "$(dirname "$0")/../../.." || exit 1
D=docs/reviews/astra-2026-09-27
codex exec -m gpt-6-astra -c 'model_reasoning_effort="xhigh"' -s workspace-write \
  -o "$D/astra-last.md" "$(cat "$D/brief.md")" < /dev/null >> "$D/astra.stdout" 2>&1
echo "codex exit: $?" >> "$D/astra.stdout"
