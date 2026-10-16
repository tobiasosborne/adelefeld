# docs/examples: programs that call the C interface

`hello.c` is the program of chapter 4 of [the guide](../GUIDE.md). It builds the finite ball 146 + 1000 Zhat from
its raw triple, makes it the finite part of an adele, adds a second adele read from text, reconstructs the
rational 46 of the sum, projects the sum to the prime 5 and takes a square root there on branch 1. It prints
every status and every value and frees everything.

From the root of the repository:

    make -j2
    cc -std=c11 -Wall -Wextra -Werror -Iinclude docs/examples/hello.c build/libadelefeld.a \
       -lflint -lgmp -lm -o build/hello
    ./build/hello

The flags are those of the [Makefile](../../Makefile): `-std=c11`, `-Iinclude` for `<adelefeld.h>`, and
`-lflint -lgmp -lm`. FLINT's headers are found under `/usr/include/flint` without an extra `-I`, and
`libflint.so` brings `libmpfr` with it (the comment above `LDLIBS` in the Makefile). `make` builds only the static
library `build/libadelefeld.a`; a shared `build/libadelefeld.so` is built by `sh tests/test_exports.sh`.

The output, as `lanes/docs-guide/check_examples.sh` checks it against a fresh run:

<!-- check: hello -->

```text
set_fmpz3:                 OK
f =                        (* ; 146 mod 1000)
set_arb_fball:             OK
x =                        (146.25 +/- 0.25 ; 146 mod 1000)
adele_set_str:             OK
y =                        (-100 +/- 0.0011 ; -100)
z = x + y =                (46.25 +/- 0.26 ; 46 mod 1000)
reconstruct:               OK
q =                        46
place_prime 5:             OK
lball_set_fball:           OK
z at 5 =                   [p=5: 46 + O(5^3)]
root_count:                OK, 2 branches
root_seed seed 1:          OK
sqrt(z) at 5, branch 1 =   [p=5: 36 + O(5^3)]
sqrt(2 + 125 Zhat) at 5:   DOMAIN
```

Under valgrind the program frees every block it allocates (`valgrind --leak-check=full ./build/hello`: "All heap
blocks were freed", run for the guide on 2026-10-16).
