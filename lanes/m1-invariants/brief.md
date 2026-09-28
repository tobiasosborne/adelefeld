# Lane m1-invariants: the debug build `-DADF_CHECK_INVARIANTS` (adf-xk4)

Read `lanes/COMMON.md` and `lanes/COMMON-C.md`. Then `docs/conventions.md` 4.4 (the row "Non-canonical
value passed to a public function", line 288), 4.5, 4.6 (the lifetime contract, lines 323 to 328) and
section 5 (the predicates); `docs/SPEC.md` section 15 row M1-D2; `include/adelefeld/modctx.h:16-18`;
the findings `docs/reviews/m1/local/review.md` R3 (reproducer `checks/invariant_check.c`) and
`docs/reviews/m1/text/review.md` R7 (reproducer `checks/probe.c`, mode `lifetime`).

This lane runs alone: it touches every file of `src/`. It starts after the repair lanes of the review
have landed on master.

**You own:** `src/` (additions inside `#ifdef ADF_CHECK_INVARIANTS` only, and one internal header
`src/invariants.h`), `Makefile` (the variable `INV` and nothing else), `tests/test_invariants.c`,
`tests/test_invariants_lifetime.c`, `tests/README.md` (one paragraph), `lanes/m1-invariants/`.
The public headers under `include/` are not yours. Without the flag the library must not change: see 6.

## What the flag promises

1. **Entry check.** Every public function that takes a value of a type with a predicate
   (`adf_rat`, `adf_fball`, `adf_scaled`, `adf_adele`, `adf_cadele`) as an input checks
   `adf_<type>_is_canonical` on entry, for each such input, and calls `flint_abort` after it has
   written one line to `stderr` that names the function, the argument and the type.
   An argument that is only written (an output that the function overwrites without reading) is not
   checked, unless the header says the function reads a field of it (a context, for example).
2. **Borrow count.** A context counts the values that refer to it, atomically (`<stdatomic.h>`);
   `adf_modctx_free` aborts, with a line on `stderr`, if the count is not zero. The count is a field of
   `adf_modctx_struct` that exists only with the flag; the struct is incomplete in the public header, so
   no public layout changes. The layouts of the values (`tests/test_layouts.c`, `tests/test_abi.c`) do
   not change.

## Work, in this order

1. **The list first.** Write `lanes/m1-invariants/functions.tsv`: one row for each of the 193 exported
   functions (take the names from `tests/test_exports.sh` or from `nm` on the library), with the
   columns: function; file; inputs checked (argument and type); borrow begins; borrow ends; exempt, with
   the reason. Exempt by their contract are: the predicates `is_canonical` themselves (they never abort,
   M1-D2); `init`, `clear`; the functions whose header admits a raw or non-canonical input (the
   `canonicalise` functions and the raw setters: read each header comment, do not guess from the name);
   the readers of text and of the dump form, for their text argument. Every exemption cites the line of
   the header that grants it. A function that the header does not exempt is checked.
2. **Red.** `tests/test_invariants.c`: for each checked function of the list, one case that builds a
   canonical value through the public interface, breaks one field of the predicate (denominator 0; a
   common factor; a residue equal to its block; `exact = 2`; and so on, at least two different breaks
   for each type), calls the function in a child process (`fork`, `waitpid`) and requires that the
   child ended by `SIGABRT` and wrote the function's name to `stderr`. For each function also one case
   with canonical inputs that requires a normal return. The two reviewers' inputs are among the cases.
   `tests/test_invariants_lifetime.c`: free with one live borrower of each kind (local `adf_fball`,
   `adf_scaled`, and the components of an `adf_adele` if they borrow) aborts; after `clear` of every
   borrower, free returns; `set`, `swap`, a change of context (`set_context`), a change of backend and
   a value that is overwritten by a value of another context leave the counts right (checked by a free
   that must succeed and a free that must abort); aliased calls `y = x`; two threads that init and
   clear 100000 borrowers each of one context, then a free that returns.
   Without the flag both programs print that they were skipped and count as passed.
   See them fail with `make check INV=1` before any code in `src/` is written, and log it.
3. **Green.** `src/invariants.h`: the macros, empty without the flag. Then file by file.
   A check must not change any result, status or output of a call with canonical inputs.
4. **The suite under the flag.** `make clean && make -j2 check INV=1` runs all test programs. Some
   existing tests pass non-canonical values on purpose. For each test that aborts: if the function is
   exempt by its header, the list of step 1 was wrong, correct it; if the test steps outside the
   contract, do not edit the test (it is not yours): list it in the report with file, line and the
   contract it breaks. `INV=1` and `SAN=1` together must work. `make clean` between builds with
   different flags (stale objects).
5. **Mutation.** `make mutate FILES=src/invariants.h` does not apply; instead run the mutation tool
   over one file of your choice with `INV=1` in its build, if the tool can pass the variable; if it
   cannot, say so in the report and delete by hand ten checks chosen by a seeded random draw from the
   list, one at a time, and show that `tests/test_invariants.c` fails each time.
6. **The release build is unchanged.** Build the library without the flag before and after your
   work and compare the object files of each source file with `objdump -d` (without `-g`): they must be
   identical. Report the command and the result for each file.
7. `make -j2 check`, `make clean && make -j2 check SAN=1`, `make clean && make -j2 check CC=clang`,
   `make clean && make -j2 check INV=1`, `make clean && make -j2 check INV=1 SAN=1`,
   `sh tests/test_exports.sh` (no new exported name without the flag), `make bench` is not run.

## Questions to answer in the report

- Values are plain structs with public fields: a caller may copy one with `memcpy` or move it by
  assignment. Say which such moves the count cannot see, and whether any function of the library itself
  does one.
- Inline functions of the public headers (`src/inlines.c`): can they carry the check? If not, mark
  `HEADER-FINDING` and list them.
- The cost: the time of `make check INV=1` against `make check`.
