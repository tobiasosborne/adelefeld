#!/bin/sh
# Run every suite of the reviewer's review_checks.py against proto/solvers_checks.py, one after the other.
export OPENBLAS_NUM_THREADS=1 OMP_NUM_THREADS=1
cd "$(dirname "$0")/../.." || exit 1
for suite in recon roots certificates real extras matrix; do
  echo "=== suite $suite"
  timeout 300 python3 docs/reviews/s-design/review_checks.py $suite
  echo "exit $?"
done
