#!/bin/sh
# The numbers of docs/assets/places.svg, from the driver. Run from the repository root after the build.
# Level k of the tree at p is the ball 22/7 + p^k Z_p, printed by `project` as "p: c + O(p^k)", c in [0, p^k)
# (tools/adf/README.md, "The commands at places", the output template "<p>: <centre> + O(<p>^<N>)").
for p in 2 3 5; do
  for k in 1 2 3 4; do
    echo "project (* ; 22/7 mod $(python3 -c "print($p**$k)")) with $p"
  done
done | build/adf
printf '%s\n' 'digits 7' 'project 22/7 with real' \
  'valuation (3.1 +/- 0.1 ; 22/7 * [1]) with 2' 'valuation (3.1 +/- 0.1 ; 22/7 * [1]) with 3' \
  'valuation (3.1 +/- 0.1 ; 22/7 * [1]) with 5' 'valuation (3.1 +/- 0.1 ; 22/7 * [1]) with 7' \
  'valuation (3.1 +/- 0.1 ; 22/7 * [1]) with 11' | build/adf
