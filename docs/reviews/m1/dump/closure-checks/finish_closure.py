"""Audit retained evidence, remove this closure's binaries, and write the complete lane report."""
import ast
import hashlib
import json
from pathlib import Path
import re
import shutil

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[4]
LANE = ROOT / "lanes/m1-closure-dump"
originals = ("bridge.c", "roundtrip.c", "fuzz_driver.c", "status_findings.py", "cost.py",
             "cost_near_limit.py", "differential.py")
for name in originals:
    assert (HERE / name).read_bytes() == (HERE.parent / "checks" / name).read_bytes(), name
scripts = sorted(HERE.glob("*.py"))
for path in scripts:
    ast.parse(path.read_text())

inputs = sorted(set(list((ROOT / "src").glob("*.[ch]"))
                    + list((ROOT / "include").rglob("*.h"))
                    + list((ROOT / "tests").glob("test_dump*.c"))
                    + [ROOT / path for path in (
                        "Makefile", "docs/SPEC.md", "docs/PLAN.md", "docs/conventions.md",
                        "proto/text_grammar.py", "proto/test_text_grammar.py", "tests/golden/dump.tsv",
                        "tests/fuzz/fuzz_dump.c", "lanes/m1-repair-dump/report.md")]))
hashes = [hashlib.sha256(p.read_bytes()).hexdigest() + "  " + str(p.relative_to(ROOT)) for p in inputs]
(HERE / "reviewed-inputs.sha256").write_text("\n".join(hashes) + "\n")

removed_dirs = []
removed_files = []
for name in ("build", "san"):
    path = HERE / name
    if path.exists():
        removed_dirs.append(name)
        shutil.rmtree(path)
for path in sorted(HERE.rglob("*")):
    if path.is_file():
        magic = path.read_bytes()[:8]
        if magic.startswith(b"\x7fELF") or magic == b"!<arch>\n":
            removed_files.append(str(path.relative_to(HERE)))
            path.unlink()

sessions = json.loads((LANE / "session-commands.json").read_text())
children = [json.loads(line) for line in (HERE / "commands.jsonl").read_text().splitlines()]
by_name = {row["name"]: row for row in children}
assert all(row["seconds"] < 170 for row in children)
expected_failures = {"original-differential": 1, "red-R1": 1, "red-R2": 1, "red-R4": 1, "red-R3": -6,
                     "fuzz": 2}
for row in children:
    assert row["exit"] == expected_failures.get(row["name"], 0), row

closure = HERE.parent / "closure.md"
assert closure.read_text().splitlines()[1].endswith("NO BLOCKER OPEN.")
assert not [(i, len(s)) for i, s in enumerate(closure.read_text().splitlines(), 1) if len(s) > 116]

audit = dict(unchanged_reproducer_copies=len(originals), python_scripts_parsed=len(scripts),
             syntax_errors=0, reviewed_input_hashes=len(inputs), child_commands=len(children),
             explained_nonzero_children=len(expected_failures), unexplained_nonzero_children=0,
             maximum_child_seconds=max(row["seconds"] for row in children),
             removed_build_directories=removed_dirs, removed_binaries=removed_files,
             remaining_binaries=0, closure_lines_over_116=0)
if (HERE / "final-audit.json").exists():
    previous = json.loads((HERE / "final-audit.json").read_text())
    audit["removed_build_directories"] = sorted(set(previous["removed_build_directories"] + removed_dirs))
    audit["removed_binaries"] = sorted(set(previous["removed_binaries"] + removed_files))
(HERE / "final-audit.json").write_text(json.dumps(audit, indent=2) + "\n")

def command_block(command):
    if "\n" in command:
        assert max(map(len, command.splitlines())) <= 116
        return ["```sh", *command.splitlines(), "```", ""]
    # Preserve each original shell token, including its quotes and pipeline operators.
    tokens = re.findall(r"(?:'[^']*'|\"[^\"]*\"|[^\s'\"])+", command)
    lines, current = [], ""
    for token in tokens:
        if len(token) > 104 and token.startswith("'") and token.endswith("'"):
            if current:
                lines.append(current + " \\")
            value = token[1:-1]
            chunks = [value[i:i+90] for i in range(0, len(value), 90)]
            lines.extend("'" + chunk + "'\\" for chunk in chunks[:-1])
            current = "'" + chunks[-1] + "'"
            continue
        if current and len(current) + 1 + len(token) > 108:
            lines.append(current + " \\")
            current = "    " + token
        else:
            current += (" " if current else "") + token
    lines.append(current)
    return ["```sh", *lines, "```", ""]


body = """# Lane m1-closure-dump report

3 CLOSED; 0 CLOSED WITH EDIT; 0 OPEN; 1 SETTLED BY DECISION. NO BLOCKER OPEN.

## What was done

Read the binding lane rules, dump review, repair report, SPEC section 10 and all M1 decisions,
PLAN milestone 1, the relevant headers, conventions and repaired functions and tests.
Built the current source with make -j2 after a clean, using an owned BUILD directory.
Memory checks before builds found 23 or 24 GB available and 0 other compiler/build processes.
No git command, tracker command, package installation or production-file edit was made.
At most two CPU tasks ran concurrently; builds did not overlap. Child commands had a 170-second limit.

R1, R3 and R4 are CLOSED. R2 is SETTLED BY DECISION M1-D9. There are 0 new findings.
The complete reasoning and coverage are in docs/reviews/m1/dump/closure.md.

## Check results

| Check | Result |
|---|---|
| Original dump suites | 32 tests, 41,607 checks, 0 failures |
| Limit suite, default | 9 tests, 243 checks, 0 failures; slow full case skipped |
| Limit suite, ADF_DUMP_LIMITS_FULL=1 | 9 tests, 261 checks, 0 failures; 97.609768 seconds |
| Independent closure statuses | R1: 180; R2: 152 plus 12 controls; R4: 7,692; 8,036 total, 0 failures |
| Original public-API round trips | 6,000 round trips, 27,029 checks, 0 failures |
| Original guard-page check | 561 cases, 0 errors, 223 allocation calls, 0 retained allocations |
| Original status cases | 7 cases, 0 untouched-output errors; 3 old R2 expectations superseded by M1-D9 |
| Original 65,537-block cost case | 682,067 bytes, UNSUPPORTED, 0.006549 constructor CPU seconds |
| Near-limit modctx | 65,536 blocks, 1,048,571 bytes, OK, 30.084275 constructor CPU seconds |
| Near-limit qclass | 65,536 blocks, 1,048,574 bytes, OK, 33.153818 constructor CPU seconds |
| Original differential | 150,000 texts, 28 known reference mismatches; exit 1 retained |
| Differential independent checks | 24,150 predicates, 0 mismatches; 0 typed or output-contract errors |
| Valgrind | 4 programs, 0 errors, 0 live bytes for each |
| Fuzz harness neighbours | 14 seeds, 425 prefix cases, 0 failures under Valgrind and ASan/UBSan |
| Isolated R1 reversion | 26 checks, 2 failures; repaired version 0 failures |
| Isolated R2 reversion | 19 checks, 5 failures; repaired version 0 failures |
| Isolated R4 reversion | 72 checks, 12 failures; repaired version 0 failures |
| Isolated R3 reversion | SIGABRT, 1 uninitialized-value error; repaired seed 0 errors |
| Python repair regression class | 3 tests, 0 failures |
| ASan and UBSan | 7 executables, 0 reports; ASan leak detection disabled, leaks checked with Valgrind |

The first fuzz command completed 362,060 inputs in 31 seconds, then exited 2 because LeakSanitizer
failed at shutdown under ptrace. Its empty artifact replayed with exit 0 when leak detection was disabled.
The repeated campaign used ASAN_OPTIONS=detect_leaks=0. Its exact counts appear at the end of this report.
Both logs and the first failure are retained. The separate Valgrind leak results are reported above.

The differential's 28 discrepancies are the original character-modulus reference limit, not new C findings.
All old assertions and all seven substantive reproducer sources are unchanged. Copies in closure-checks
preserve the original review logs. The old runners, historical audit and cleanup scripts were read but not
executed; they overwrite old evidence and are not finding reproducers.

The R1 test helper all_load skips typed loaders if context construction fails. New closure cases invoke
those loaders directly. The selected regression expectations come from the contract, not program output.
Isolated reversions demonstrate detection; they do not prove the original historical TDD process.

## Not examined or not done

The m1-invariants and m1-repair-tools lanes have not landed. Their invariant checks, mutation runner and
memory-checker tool were not approved here. None of this review's four findings waits for those lanes.
No complete proof over all strings, allocation-failure injection, thread-safety review, unrelated context
constructor review, full mutation sweep, or arithmetic enclosure review beyond restoration was performed.
There is no typed qclass loader; those inputs were checked through context extraction.
The complete repository make check was not repeated; the changed dump surface and its original checks were run.
No production repair or specification change was made. The stale test comment saying the header lacks the
exponent constant was recorded in closure.md, not edited in this lane.

## Sources pending

[source pending: a FLINT 3.0.1 source under refs stating the exact MAG_MAN/MAG_EXP representation
used by src/dump.c:1297]. The original gap remains. refs/src/flint-3.0.1/mag.rst:6 states a 30-bit
mantissa and arbitrary-precision exponent but does not state the representation formula.

## Findings against the specification

None found. M1-D5 is now enforced. M1-D9 permits R2's limit and specifies its stage and scope.
The old report's proposed-decision wording is superseded by the accepted SPEC decision.

## Files written

Every retained path below was created or written in this lane. The only edits used apply_patch and the
owned check scripts. No original review/check file was changed. Temporary products removed at the end were
the build/ and san/ trees under closure-checks and the standalone binaries listed in final-audit.json.
The red/ source files remain so the isolated reversions are reviewable.
""".splitlines()
body += [""]
for path in sorted([p for p in HERE.rglob("*") if p.is_file()] + [closure]
                   + [p for p in LANE.iterdir() if p.is_file()]):
    body.append("- " + str(path.relative_to(ROOT)))
body += ["", "## Every shell command and result", "",
         "The working directory for each command is the repository root. Full read outputs are in",
         "session-commands.json; child command outputs and timings are in closure-checks/*.log and",
         "closure-checks/commands.jsonl. Build logs retain each expanded compiler command.", ""]
for i, row in enumerate(sessions, 1):
    command = row["cmd"]
    result = row["result"]
    if "closure-checks/run.py " in command:
        name = command.split("closure-checks/run.py ", 1)[1].split()[0]
        child = by_name[name]
        summary = f"child exit {child['exit']}; {child['seconds']} seconds; log {name}.log."
        if name == "red-R3":
            summary += " SIGABRT; wrapper exit 250."
    elif isinstance(result, dict):
        summary = f"exit {result['exit_code']}; output lines {len(result.get('output', '').splitlines())}."
        if result["exit_code"] == 1 and command.startswith("ps "):
            summary += " Zero matching build/compiler processes."
    else:
        summary = result
    body += [f"{i}. {summary}", "", *command_block(command)]

own = "python3 -B docs/reviews/m1/dump/closure-checks/finish_closure.py"
body += [f"{len(sessions)+1}. exit 0; final audit and cleanup results:", "", *command_block(own),
         "```text", *json.dumps(audit, indent=2).splitlines(), "```", ""]
fuzz_log = (HERE / "fuzz-no-lsan.log").read_text()
fuzz_stats = [line for line in fuzz_log.splitlines()
              if line.startswith("stat::") or line.startswith("Done ") or line.startswith("fuzz passed:")]
body += ["## Fuzz campaign output", "", "```text", *fuzz_stats, "```", ""]
long_lines = [(i, len(line)) for i, line in enumerate(body, 1) if len(line) > 116]
assert not long_lines, long_lines
(LANE / "report.md").write_text("\n".join(body))
print(json.dumps(audit))
print(f"report_lines={len(body)} report_lines_over_116=0 verdict_findings=4")
