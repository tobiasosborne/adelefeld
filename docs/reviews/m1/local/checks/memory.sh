#!/bin/sh
# Run from the repository root. The oracle sends 900 requests over stdin.
exec valgrind --error-exitcode=99 --leak-check=full --show-leak-kinds=all \
    --log-file=docs/reviews/m1/local/checks/valgrind.log \
    docs/reviews/m1/local/checks/probe
