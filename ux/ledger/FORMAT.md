# The claim file and the seal file of the Ledger

A claim is one file `claims/<id>.json`, UTF-8 JSON, one object. Its fields:

| Field | Type | Meaning |
|---|---|---|
| `id` | string | the file name without `.json`: `a-z`, `0-9`, `.`, `_`, `-`; it begins with a letter or digit |
| `statement` | string | one sentence; a claim about a root names its branch by seed or says "all branches" |
| `script` | list of strings | the driver lines, one string each, no newline; settings and `#` comments allowed |
| `expect` | list of strings | the exact output lines, one per printing script line; a valid setting prints none |
| `expect_exit` | integer, optional | the exit status of the driver (0 or 1); when absent it is not compared |
| `places` | list | the footprint: `"R"`, primes as integers, `"cofactor <n>"` for a factor not split below 1000 |
| `branch` | string | required when the script calls `root`, `root_at`, `roots_at` or `powrat_at`: a seed (one |
| | | branch; every root line uses it, none is `roots_at`) or `"all"` (every branch the lines print) |
| `tail` | string | `"nothing said"` or `"argued"`: what the claim says about every place not in `places` |
| `argument` | string | prose for what the driver does not compute, with sources by file and line; may be empty |
| | | (an argued tail is ratified only with a source `refs/src/<path>:<lines>` or `docs/<path>:<lines>` on disk) |
| `proposer` | string | who wrote the claim, a free string |
| `state` | string | `"proposed"` when the claim is written; only a person may change it to `"ratified"` |
| `question` | string, optional | the question of the ledger the claim belongs to; the page groups by it |
| `parent` | string, optional | the id of the claim this one was split from or derived from |
| `source` | string, optional | where an imported claim came from (`tests/driver/NAME.cmd`) |

Ids (decision of the slice): `fx-<name>` for the fixture `tests/driver/<name>.cmd`; `a<k>-<slug>` and `b<k>-<slug>`
for the claims of scenarios A and B of `docs/ux/brief-3-claim-ledger.md`; a child of `b5` is `b5-1`.

A family (a claim with a parameter, decision of the slice) is ONE claim whose script has one line per member and
whose `places` lists the members; `tail` says whether the other members are argued. One failing member refutes the
family, and the card names the line.

A seal is the file `seals/<id>.json`, written only by `seal.py`: `verdict`; `run_verdict` (what this run alone
says, which differs from `verdict` only for a reopened claim); `note` (a timeout or a crash); `actual` (the lines);
`exit`; `script_line` (for each output line, the number of the script line that printed it, from the trace of
`adf -v`, or `null` when the trace could not be matched); `date` (UTC); `build` (`commit` from `git rev-parse HEAD`;
`adf_sha256`; `flint_version` from `FLINT_VERSION` in `flint_header`, the `flint/flint.h` included; `flint_library`
from `ldd build/adf`); `claim_sha256` (of the claim file's bytes); `claim_content_sha256` (of the claim without
`state`, so that a ratification does not make the seal stale); `reopened` (null, or the two builds that disagree).
Older seals of a claim are kept as `seals/history/<id>.<date>.json`.

## The states

- `proposed`: written, not sealed: no seal, an empty script, a seal of an earlier text of the claim, or a script
  with a root command and no valid `branch` (the sealer refuses it and prints why).
- `sealed`: the driver wrote exactly the expected lines (and the expected exit status, when given).
- `refuted`: the driver wrote a value or a status that contradicts the expectation: another value, a status of
  the classes "proved no" (`DOMAIN`, `NO_SOLUTION`, `NOT_UNIT`) or "not mathematics" (`PARSE`, `UNSUPPORTED`)
  where something else was expected, a missing or extra line, another exit status, a crash.
- `open`: no line contradicts, and at least one line is a status of the open class (`NOT_DETERMINED`,
  `UNIT_NOT_CERTIFIED`, `NEEDS_SPLIT`, `NOT_UNIQUE`) or the budget class (`LIMIT`, or the timeout of 60 s) where
  something else was expected. A status line is `error: S` or `<prefix>: error: S` (the Tate commands print
  `continuation S=...: error: NOT_DETERMINED`); a line of several `; ` items is a value line.
  Also `open`: a later build, or a second run of the same build, wrote other lines
  than the earlier seal; both builds are named in `reopened`.
- `ratified`: the claim file says `ratified`, its seal is `sealed`, and, when its tail is `argued`, its argument
  cites a source by file and line that exists. Otherwise the ratification is void: a refuted claim stays refuted;
  a sealed claim with an argued tail and no source shows "ratification refused: no source". No command of the
  slice sets this state.

An agreeing later build leaves the seal as it is: a seal names the first build that gave its lines.
A seal says "this build of the library computed this", not "this is true".

## The footprint

A place is computed when a script line that printed (by the seal's `script_line`) names it: the place operand of a
command that takes one (`exp_at X with P`, `roots_at X with P with D`, `hilbert_at X with Y with P`,
`project X with PLACES`, ...; the table `PLACE_OPERAND` of `ledger.py`, read from tools/adf/README.md), or, for any
other command, the moduli of `p=<n>` and `mod <n>` (trial division below 1000) and R for `real` or a real
coordinate. The page draws a computed place filled, a place of `places` without such a line outlined. The importer
writes the places of a fixture by the same rule.
