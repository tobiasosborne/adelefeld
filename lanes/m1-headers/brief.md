# Lane m1-headers: the public header for the types of milestone 1

The gate of milestone 0 has passed (`docs/reviews/m0-gate/review.md`, `closure.md`); its edits are applied in
`docs/conventions.md` 0.4, `docs/SPEC.md` and `docs/PLAN.md` 1.3. The closure check allows: the global ring types
and the value text; declarations for contexts, the scaled policy and dumps only in the form of its edits.

Read: `docs/conventions.md` completely (it is your contract: naming, argument order, status codes, aliasing,
output states, canonical forms, places, text interface, foreign-function section); `docs/SPEC.md` sections 4, 9.2,
10; `docs/PLAN.md` sections 2 to 5 and rows 1.2 to 1.9; `docs/seams.md` section 5; `docs/reviews/m0-gate/closure.md`;
the FLINT headers under `/usr/include/flint` for the types you embed.

**You own:** `include/adelefeld.h`, `include/adelefeld/` (new, all of it), `tests/test_headers.c`,
`tests/test_abi.c`, `docs/api-m1.md`.

Write the headers, declarations only, no implementation:

- `include/adelefeld/status.h`: status codes, `adf_status_str`.
- `include/adelefeld/place.h`: the opaque place handle and its functions.
- `include/adelefeld/rat.h`: `adf_rat`.
- `include/adelefeld/fball.h`: `adf_fball`, global backend: init, clear, set, swap, constructors from integers and
  rationals, canonicalisation, tight add, sub, neg, mul, scaling by exact rationals, the three set predicates,
  the three-valued comparison of points, membership of an exact rational, accessors (centre, radius,
  denominator, precision at a prime), Haar volume.
- `include/adelefeld/adele.h`: `adf_adele`, `adf_cadele`, conversion of `adf_rat` at a requested precision,
  arithmetic, projection to the places.
- `include/adelefeld/recon.h`: rational reconstruction from a full ball.
- `include/adelefeld/text.h`: value form: parse and print for the types above, with (pointer, length) input,
  limits, and the string ownership rules of the conventions.
- `include/adelefeld/modctx.h`, `scaled.h`: contexts (constructors and free, as the gate decided) and the
  scaled policy and the absolute cap, in the form of the closure edits.
- `include/adelefeld/dump.h`: the dump form.
- `include/adelefeld.h`: includes all of them; version macros 0.1.0; the FLINT version check.

Every declaration carries a comment block with: what the function computes as a set statement (the enclosure
contract); the proof it implements (`docs/proofs/<file>.md`, statement number) or the convention
(`docs/conventions.md` section); which arguments may alias; the statuses it may return and the state of each
output for each; its cost class if known. A programmer who has only the header and the cited statement must be
able to implement the function, and two such programmers must produce interchangeable code.

Foreign-function rules (a Julia layer will call these through `ccall`): every public operation is an exported
function; no variadic functions; no macro-only interface; status as `int`; struct layouts fixed and documented,
with exported `adf_sizeof_*` and `adf_alignof_*` functions; nothing depends on a compiler extension.

`tests/test_headers.c`: includes each header alone and all together, in C11 with `-Wall -Wextra -Wpedantic
-Werror`, and as C++17 if `g++` is present (extern "C" guards); checks the status values against
`docs/conventions.md`. `tests/test_abi.c`: static assertions on the sizes and offsets that the conventions fix.
Both must pass `make check` while no implementation exists (they call no library function, or only ones you
mark as header-inline). `docs/api-m1.md`: one table of all functions: name, header, work package of PLAN that
implements it, proof or convention it rests on.

If the conventions leave a signature undetermined or contradict each other, do not choose silently: list each
case in your report under "Findings against the specification", with the choice you made and why.
