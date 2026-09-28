# Executed check commands

Run from the repository root; checks were sequential.

1. build: exit 0; elapsed 0.033664 seconds.

```sh
make -j2 BUILD=docs/reviews/m1/dump/checks/build \
    'CFLAGS=-std=c11 -O2 -g -fPIC -Wall -Wextra -Wpedantic -Werror' \
    docs/reviews/m1/dump/checks/build/test_dump docs/reviews/m1/dump/checks/build/test_dump_ctx \
    docs/reviews/m1/dump/checks/build/test_dump_golden
```

2. bridge.so: exit 0; elapsed 0.617659 seconds.

```sh
cc -std=c11 -O2 -g -Wall -Wextra -Werror -Iinclude -fPIC -shared docs/reviews/m1/dump/checks/bridge.c \
    docs/reviews/m1/dump/checks/build/libadelefeld.a -lflint -lgmp -lm -o \
    docs/reviews/m1/dump/checks/bridge.so
```

3. bridge: exit 0; elapsed 0.617263 seconds.

```sh
cc -std=c11 -O2 -g -Wall -Wextra -Werror -Iinclude docs/reviews/m1/dump/checks/bridge.c \
    docs/reviews/m1/dump/checks/build/libadelefeld.a -lflint -lgmp -lm -o docs/reviews/m1/dump/checks/bridge
```

4. roundtrip: exit 0; elapsed 0.566698 seconds.

```sh
cc -std=c11 -O2 -g -Wall -Wextra -Werror -Iinclude docs/reviews/m1/dump/checks/roundtrip.c \
    docs/reviews/m1/dump/checks/build/libadelefeld.a -lflint -lgmp -lm -o \
    docs/reviews/m1/dump/checks/roundtrip
```

5. test_dump: exit 0; elapsed 0.064788 seconds.

```sh
docs/reviews/m1/dump/checks/build/test_dump
```

6. test_dump_ctx: exit 0; elapsed 0.165331 seconds.

```sh
docs/reviews/m1/dump/checks/build/test_dump_ctx
```

7. test_dump_golden: exit 0; elapsed 0.032722 seconds.

```sh
docs/reviews/m1/dump/checks/build/test_dump_golden
```

8. bridge: exit 0; elapsed 0.00866 seconds.

```sh
docs/reviews/m1/dump/checks/bridge
```

9. roundtrip: exit 0; elapsed 0.033248 seconds.

```sh
docs/reviews/m1/dump/checks/roundtrip
```

10. status: exit 0; elapsed 0.065363 seconds.

```sh
python3 -B docs/reviews/m1/dump/checks/status_findings.py
```

11. differential: exit 1; elapsed 10.051575 seconds.

```sh
python3 -B docs/reviews/m1/dump/checks/differential.py
```

12. valgrind-bridge: exit 0; elapsed 1.219163 seconds.

```sh
valgrind --leak-check=full --errors-for-leak-kinds=definite,indirect --error-exitcode=99 \
    docs/reviews/m1/dump/checks/bridge
```

13. valgrind-roundtrip: exit 0; elapsed 2.27269 seconds.

```sh
valgrind --leak-check=full --errors-for-leak-kinds=definite,indirect --error-exitcode=99 \
    docs/reviews/m1/dump/checks/roundtrip
```

14. fuzz-build: exit 0; elapsed 0.516906 seconds.

```sh
cc -std=c11 -O0 -g -I. -Iinclude docs/reviews/m1/dump/checks/fuzz_driver.c \
    docs/reviews/m1/dump/checks/build/libadelefeld.a -lflint -lgmp -lm -o \
    docs/reviews/m1/dump/checks/fuzz-gcc-o0
```

15. fuzz-seed: exit -6; elapsed 2.624828 seconds.

```sh
valgrind --track-origins=yes --error-exitcode=99 docs/reviews/m1/dump/checks/fuzz-gcc-o0
```

16. build: exit 0; elapsed 0.016796 seconds.

```sh
make -j2 BUILD=docs/reviews/m1/dump/checks/build \
    'CFLAGS=-std=c11 -O2 -g -fPIC -Wall -Wextra -Wpedantic -Werror' \
    docs/reviews/m1/dump/checks/build/test_dump docs/reviews/m1/dump/checks/build/test_dump_ctx \
    docs/reviews/m1/dump/checks/build/test_dump_golden
```

17. bridge.so: exit 0; elapsed 0.717525 seconds.

```sh
cc -std=c11 -O2 -g -Wall -Wextra -Werror -Iinclude -fPIC -shared docs/reviews/m1/dump/checks/bridge.c \
    docs/reviews/m1/dump/checks/build/libadelefeld.a -lflint -lgmp -lm -o \
    docs/reviews/m1/dump/checks/bridge.so
```

18. bridge: exit 0; elapsed 0.618457 seconds.

```sh
cc -std=c11 -O2 -g -Wall -Wextra -Werror -Iinclude docs/reviews/m1/dump/checks/bridge.c \
    docs/reviews/m1/dump/checks/build/libadelefeld.a -lflint -lgmp -lm -o docs/reviews/m1/dump/checks/bridge
```

19. roundtrip: exit 0; elapsed 0.718442 seconds.

```sh
cc -std=c11 -O2 -g -Wall -Wextra -Werror -Iinclude docs/reviews/m1/dump/checks/roundtrip.c \
    docs/reviews/m1/dump/checks/build/libadelefeld.a -lflint -lgmp -lm -o \
    docs/reviews/m1/dump/checks/roundtrip
```

20. differential: exit 1; elapsed 13.962481 seconds.

```sh
python3 -B docs/reviews/m1/dump/checks/differential.py
```

21. san-build: exit 0; elapsed 0.016503 seconds.

```sh
make -j2 BUILD=docs/reviews/m1/dump/checks/san SAN=1 docs/reviews/m1/dump/checks/san/test_dump \
    docs/reviews/m1/dump/checks/san/test_dump_ctx docs/reviews/m1/dump/checks/san/test_dump_golden
```

22. bridge-san-build: exit 0; elapsed 0.566738 seconds.

```sh
cc -std=c11 -O1 -g -Iinclude -fsanitize=address,undefined -fno-omit-frame-pointer \
    docs/reviews/m1/dump/checks/bridge.c docs/reviews/m1/dump/checks/san/libadelefeld.a -lflint -lgmp -lm -o \
    docs/reviews/m1/dump/checks/bridge-san
```

23. roundtrip-san-build: exit 0; elapsed 0.617011 seconds.

```sh
cc -std=c11 -O1 -g -Iinclude -fsanitize=address,undefined -fno-omit-frame-pointer \
    docs/reviews/m1/dump/checks/roundtrip.c docs/reviews/m1/dump/checks/san/libadelefeld.a -lflint -lgmp -lm \
    -o docs/reviews/m1/dump/checks/roundtrip-san
```

24. san-test_dump: exit 0; elapsed 0.116015 seconds.

```sh
env ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 docs/reviews/m1/dump/checks/san/test_dump
```

25. san-test_dump_ctx: exit 0; elapsed 0.316192 seconds.

```sh
env ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 docs/reviews/m1/dump/checks/san/test_dump_ctx
```

26. san-test_dump_golden: exit 0; elapsed 0.03259 seconds.

```sh
env ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 docs/reviews/m1/dump/checks/san/test_dump_golden
```

27. san-bridge: exit 0; elapsed 0.032578 seconds.

```sh
env ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 docs/reviews/m1/dump/checks/bridge-san
```

28. san-roundtrip: exit 0; elapsed 0.115212 seconds.

```sh
env ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 docs/reviews/m1/dump/checks/roundtrip-san
```

29. cost: exit 0; elapsed 71.933129 seconds.

```sh
python3 -B docs/reviews/m1/dump/checks/cost.py 65537
```

30. near-modctx: exit 0; elapsed 134.98201 seconds.

```sh
python3 -B docs/reviews/m1/dump/checks/cost_near_limit.py modctx
```

31. near-qclass: exit 0; elapsed 74.817593 seconds.

```sh
python3 -B docs/reviews/m1/dump/checks/cost_near_limit.py qclass
```
