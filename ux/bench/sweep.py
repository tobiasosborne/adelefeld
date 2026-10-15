#!/usr/bin/env python3
"""The sweep runner of the Bench slice (docs/ux/brief-4-sweep-bench.md, "A first buildable slice", item 1).

    python3 ux/bench/sweep.py ux/bench/experiments/<name>.json [--driver build/adf]

reads an experiment, writes the driver lines, runs build/adf once for the experiment (row by row: the real place,
then the primes in increasing order, every column of a row before the next row), and writes

    ux/bench/out/<name>.records.jsonl   one record per well and one per check; no times, so it is reproducible
    ux/bench/out/<name>.run.json        the times of the cells and of the process, the driver's sha256, the commit

The experiment format is in ux/bench/README.md. Standard library only.
"""
import argparse
import hashlib
import json
import os
import re
import select
import signal
import subprocess
import sys
import time
from fractions import Fraction
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent

# The status names of include/adelefeld/status.h:45-57 (adf_status_str), each in one class of D-UX4
# (lanes/UX-COMMON.md; docs/ux/README.md, "What could be combined"), with OK as its own class.
STATUS_CLASS = {
    'OK': 'ok',
    'NOT_DETERMINED': 'open',
    'UNIT_NOT_CERTIFIED': 'open',
    'NEEDS_SPLIT': 'open',
    'NOT_UNIQUE': 'open',
    'NO_SOLUTION': 'no',
    'NOT_UNIT': 'no',
    'DOMAIN': 'no',
    'UNSUPPORTED': 'plain',
    'PARSE': 'plain',
    'LIMIT': 'budget',
}
# Two classes of the bench that are no status of the library: a cell with no driver command at its place, and a
# cell not run for budget.
NO_COMMAND = 'no command'
BELOW = 'below the stop line'

# The value form of an exact rational (conventions 9.1): an integer or n/d.
RAT_RE = re.compile(r'^-?\d+(?:/\d+)?$')
# A polynomial operand of `roots` and `realroots`: integer coefficients, constant first, single spaces
# (tools/adf/README.md:149-155).
POLY_RE = re.compile(r'^-?\d+(?: -?\d+)*$')
# The real-ball text of conventions 9.5 (docs/conventions.md:1287-1294): fmt, or fmt +/- fmt.
_FMT = r'-?\d+(?:\.\d+)?(?:e-?\d+)?'
REAL_RE = re.compile(r'^%s(?: \+/- %s)?$' % (_FMT, _FMT))
# An entry of `roots`: `A mod P^K` (tools/adf/README.md:161-167).
CERT_RE = re.compile(r'^\d+ mod \d+\^\d+$')
# A branch of `roots_at` and `powrat_at`: `P [ID]: value` (tools/adf/README.md:489-500, 521-531).
BRANCH_RE = re.compile(r'^(\d+) \[([+-]?\d+)\]: (.+)$')
# A component of the commands at places: `real: ball` or `P: value` (tools/adf/README.md:250-258).
COMPONENT_RE = re.compile(r'^(real|\d+): (.+)$')
ERROR_RE = re.compile(r'^error: ([A-Z_]+)$')

OP_RE = re.compile(r'^(?:roots [1-9]\d*|roots|realroots|exp|log|project|powrat -?\d+(?:/\d+)? -?\d+)$')
LIST_OPS = ('roots', 'realroots')     # and `roots n`: the output is a list, the well shows its length
PREC_MAX = 100000                     # ADF_PRINT_EXP_MAX: a `prec` setting above it is error: LIMIT
                                      # (tools/adf/README.md:417-424); the runner refuses it before the run


def primes_below(bound):
    """The primes p < bound, by the sieve of Eratosthenes."""
    if bound <= 2:
        return []
    sieve = bytearray([1]) * bound
    sieve[0] = sieve[1] = 0
    for i in range(2, int((bound - 1) ** 0.5) + 1):
        if sieve[i]:
            sieve[i * i::i] = bytearray(len(range(i * i, bound, i)))
    return [i for i in range(bound) if sieve[i]]


def is_prime(n):
    return n >= 2 and all(n % d for d in range(2, int(n ** 0.5) + 1))


def expand_places(spec):
    """A list of `R`, primes and `primes below N` to the rows of the wall: `R` first, then primes increasing."""
    real, primes = False, set()
    for item in spec:
        if item == 'R':
            real = True
        elif isinstance(item, int) and not isinstance(item, bool):
            if not is_prime(item):
                raise ValueError('not a prime: %r' % item)
            primes.add(item)
        elif isinstance(item, str) and re.fullmatch(r'primes below [1-9]\d*', item):
            primes.update(primes_below(int(item.split()[2])))
        else:
            raise ValueError('a place is R, a prime, or "primes below N": %r' % (item,))
    return (['R'] if real else []) + sorted(primes)


def input_label(inp):
    return inp.get('label') or inp.get('value') or inp.get('poly')


def validate(e):
    """Check an experiment and return a copy with `rows` (the places expanded) and `labels`."""
    e = dict(e)
    for key in ('name', 'question', 'operation', 'inputs', 'places', 'precision', 'budget'):
        if key not in e:
            raise ValueError('experiment without %r' % key)
    if not re.fullmatch(r'[a-z0-9_]+', e['name']):
        raise ValueError('a name is lower case letters, digits and _')
    if not OP_RE.match(e['operation']):
        raise ValueError('unknown operation %r' % e['operation'])
    poly_op = e['operation'] in LIST_OPS
    for inp in e['inputs']:
        if poly_op != ('poly' in inp) or ('value' in inp) == ('poly' in inp):
            raise ValueError('roots and realroots take {"poly": ...}; the other operations {"value": ...}')
        if 'poly' in inp and not POLY_RE.match(inp['poly']):
            raise ValueError('a polynomial is integer coefficients, constant first: %r' % inp['poly'])
    e['rows'] = expand_places(e['places'])
    e['labels'] = [input_label(i) for i in e['inputs']]
    prec = e['precision']
    for key, val in prec.items():
        if key not in ('R', 'p') and not (key.isdigit() and is_prime(int(key))):
            raise ValueError('a precision key is R, p or a prime: %r' % key)
        if not (isinstance(val, int) and 1 <= val <= PREC_MAX):
            raise ValueError('a precision is an integer from 1 to %d' % PREC_MAX)
    b = e['budget']
    if not (isinstance(b.get('max_cells'), int) and b['max_cells'] >= 0):
        raise ValueError('budget.max_cells is a non-negative integer')
    if not (isinstance(b.get('timeout'), (int, float)) and 0 < b['timeout'] <= 600):
        raise ValueError('budget.timeout is a number of seconds, at most 600')
    for c in e.get('checks', []):
        if not re.fullmatch(r'[a-z0-9_]+', c.get('id', '')) or not c.get('line'):
            raise ValueError('a check has an id and a driver line')
    return e


def precision_at(e, place):
    prec = e['precision']
    if place == 'R':
        return prec.get('R', 64)
    return prec.get(str(place), prec.get('p', 8))


def command_for(op, place, inp, prec=None):
    """The driver line of one well, or (None, reason) when the driver has no command for it at the place."""
    if not OP_RE.match(op):
        raise ValueError('unknown operation %r' % op)
    at = 'real' if place == 'R' else str(place)
    if op.startswith('roots '):
        n = int(op.split()[1])
        x = inp['value']
        if place != 'R':
            return 'roots_at %s with %s with %d' % (x, place, n), None
        if not RAT_RE.match(x):
            return None, ('roots_at is UNSUPPORTED at real (tools/adf/README.md:511), and realroots needs a '
                          'rational; this input is no rational')
        q = Fraction(x)
        coeffs = [-q.numerator] + [0] * (n - 1) + [q.denominator]
        return 'realroots ' + ' '.join(map(str, coeffs)), None
    if op == 'roots':
        if place == 'R':
            return 'realroots %s' % inp['poly'], None
        return 'roots %s with %s with %d' % (inp['poly'], place, prec if prec is not None else 8), None
    if op == 'realroots':
        if place == 'R':
            return 'realroots %s' % inp['poly'], None
        return None, 'realroots is a command of the real place only'
    x = inp['value']
    if op in ('exp', 'log'):
        return '%s_at %s with %s' % (op, x, at), None
    if op == 'project':
        return 'project %s with %s' % (x, at), None
    _, expo, seed = op.split()
    if place == 'R':
        return None, ('powrat_at is a command at a prime: its statuses are those of root_at, for which real is '
                      'UNSUPPORTED (tools/adf/README.md:511, 535)')
    return 'powrat_at %s with %s with %s with %s' % (x, place, expo, seed), None


def parse_output(op, line):
    """Status, class, count (list operations) and value (the other operations) of one output line."""
    m = ERROR_RE.match(line)
    if m:
        name = m.group(1)
        if name not in STATUS_CLASS:
            raise ValueError('a status that include/adelefeld/status.h does not have: %r' % name)
        return {'status': name, 'class': STATUS_CLASS[name], 'count': None, 'value': None}
    if op.startswith('roots') or op == 'realroots':
        entries = [] if line == 'none' else line.split('; ')
        for entry in entries:
            if not (REAL_RE.match(entry) or CERT_RE.match(entry) or BRANCH_RE.match(entry)):
                raise ValueError('not an entry of a list of roots: %r' % entry)
        return {'status': 'OK', 'class': 'ok', 'count': len(entries), 'value': None}
    m = BRANCH_RE.match(line) or COMPONENT_RE.match(line)
    if not m:
        raise ValueError('not a line of a command at a place: %r' % line)
    return {'status': 'OK', 'class': 'ok', 'count': None, 'value': m.groups()[-1]}


def plan(e):
    """Every well of the experiment in the order of the run, with `run` False for a cell cut by max_cells.

    The cut is by whole rows: the rows run are the first k with k * columns <= max_cells."""
    ncols = len(e['inputs'])
    rows_run = e['budget']['max_cells'] // ncols if ncols else 0
    cells = []
    for i, place in enumerate(e['rows']):
        prec = precision_at(e, place)
        for j, inp in enumerate(e['inputs']):
            line, reason = command_for(e['operation'], place, inp, prec)
            cell = {'row': place, 'column': j, 'input': input_label(inp), 'line': line, 'prec': prec,
                    'run': i < rows_run}
            if not cell['run']:
                cell['reason'] = 'budget: max_cells = %d allows %d of %d rows of %d cells' % (
                    e['budget']['max_cells'], rows_run, len(e['rows']), ncols)
            elif line is None:
                cell['reason'] = reason
            cells.append(cell)
    return cells


def _record(e, cell, output=None, reason=None):
    rec = {'experiment': e['name'], 'kind': 'well', 'row': cell['row'], 'column': cell['column'],
           'input': cell['input'], 'line': cell['line'], 'prec': cell['prec']}
    if reason is not None:
        rec.update(status=BELOW, **{'class': 'below'}, count=None, value=None, reason=reason)
        if output is not None:
            rec['output'] = output
    elif cell['line'] is None:
        rec.update(status=NO_COMMAND, **{'class': 'na'}, count=None, value=None, reason=cell['reason'])
    else:
        rec['output'] = output
        rec.update(parse_output(e['operation'], output))
    return rec


def records_from_outputs(e, outputs, cut=None):
    """Records from the outputs {(row, column): line}, in the order of the plan.

    A row is above the stop line when every cell of it with a command has an answer; the first row that is not,
    and every row after it, is below the stop line. `cut` is the reason given to those rows (a timeout)."""
    cells = plan(e)
    records, stopped = [], False
    for place in e['rows']:
        row = [c for c in cells if c['row'] == place]
        if not stopped and all(c['run'] for c in row):
            stopped = not all((place, c['column']) in outputs for c in row if c['line'] is not None)
        else:
            stopped = True
        for c in row:
            key = (place, c['column'])
            if not c['run']:
                records.append(_record(e, c, reason=c['reason']))
            elif stopped:
                records.append(_record(e, c, output=outputs.get(key), reason=cut or 'no output from the driver'))
            else:
                records.append(_record(e, c, output=outputs.get(key)))
    return records


def stop_line(records):
    """The first row below the stop line (None when every row ran), the number of rows below, the rows swept."""
    rows, below = [], []
    for r in records:
        if r['kind'] != 'well':
            continue
        if r['row'] not in rows:
            rows.append(r['row'])
        if r['status'] == BELOW and r['row'] not in below:
            below.append(r['row'])
    first = next((row for row in rows if row in below), None)
    swept = rows if first is None else rows[:rows.index(first)]
    return {'row': first, 'below': len(rows) - len(swept), 'swept': swept}


def driver_script(e, cells):
    """The driver lines of the cells to run: a `prec` setting when the precision changes, then the command."""
    lines, current = [], None
    if 'digits' in e:
        lines.append('digits %d' % e['digits'])
    for c in cells:
        if c['prec'] != current:
            lines.append('prec %d' % c['prec'])
            current = c['prec']
        lines.append(c['line'])
    return lines


def run_driver(lines, ncommands, driver, timeout):
    """Run the driver on the lines; return (outputs, arrival times in s, timed_out, exit status, stderr).

    The driver reads all its input before it runs (tools/adf/adf.c: fread to the end), so the lines are written and
    stdin is closed; stdout is made line buffered by stdbuf, so every output line arrives when its command is done,
    and a process killed at the deadline keeps the lines it gave."""
    t0 = time.perf_counter()
    proc = subprocess.Popen(['timeout', str(int(timeout) + 5), 'stdbuf', '-oL', str(driver)],
                            stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=ROOT,
                            start_new_session=True)
    proc.stdin.write(('\n'.join(lines) + '\n').encode())
    proc.stdin.close()
    fd = proc.stdout.fileno()
    buf, outputs, times, timed_out = b'', [], [], False
    deadline = t0 + timeout
    while True:
        wait = deadline - time.perf_counter()
        ready = select.select([fd], [], [], max(0.0, wait))[0] if wait > 0 else []
        if not ready:
            timed_out = True
            os.killpg(proc.pid, signal.SIGKILL)   # the group: timeout, stdbuf's child and anything it started
            break
        chunk = os.read(fd, 1 << 16)
        if not chunk:
            break
        buf += chunk
        while b'\n' in buf:
            line, buf = buf.split(b'\n', 1)
            outputs.append(line.decode())
            times.append(time.perf_counter() - t0)
    status = proc.wait()
    err = proc.stderr.read().decode()
    proc.stdout.close()
    proc.stderr.close()
    if not timed_out and len(outputs) != ncommands:
        raise RuntimeError('the driver wrote %d lines for %d commands (exit %d): %s'
                           % (len(outputs), ncommands, status, err.strip()))
    return outputs, times, timed_out, status, err


def sweep(e, driver):
    """Run an experiment; return (records, run), run holding the times and the facts of the process."""
    cells = [c for c in plan(e) if c['run'] and c['line'] is not None]
    lines = driver_script(e, cells)
    outputs, times, timed_out, status, err = ([], [], False, None, '')
    if cells:
        outputs, times, timed_out, status, err = run_driver(lines, len(cells), driver, e['budget']['timeout'])
    answered = {(c['row'], c['column']): o for c, o in zip(cells, outputs)}
    cut = 'budget: timeout of %s s of the driver process' % e['budget']['timeout'] if timed_out else None
    records = records_from_outputs(e, answered, cut)
    checks = e.get('checks', [])
    check_out, check_times = [], []
    if checks:
        check_out, check_times, check_to, _, _ = run_driver([c['line'] for c in checks], len(checks), driver,
                                                            e['budget']['timeout'])
        for c, o in zip(checks, check_out):
            rec = {'experiment': e['name'], 'kind': 'check', 'id': c['id'], 'line': c['line'], 'output': o}
            m = ERROR_RE.match(o)
            rec['status'] = m.group(1) if m else 'OK'
            if rec['status'] not in STATUS_CLASS:
                raise ValueError('unknown status %r' % rec['status'])
            rec['class'] = STATUS_CLASS[rec['status']]
            records.append(rec)
    cell_times = []
    prev = 0.0
    for c, t in zip(cells, times):
        cell_times.append({'row': c['row'], 'column': c['column'], 'time_s': round(t - prev, 6)})
        prev = t
    run = {'experiment': e['name'], 'timed_out': timed_out, 'exit_status': status, 'stderr': err,
           'calls': len(cells), 'answered': len(outputs),
           'process_time_s': round(times[-1], 6) if times else 0.0,
           'cell_times': cell_times,
           'cell_time_note': 'time between two output lines; the first includes the start of the process',
           'checks': len(checks), 'checks_time_s': round(check_times[-1], 6) if check_times else 0.0}
    return records, run


def provenance(driver):
    info = {'driver': str(driver)}
    try:
        info['driver_sha256'] = hashlib.sha256(Path(driver).read_bytes()).hexdigest()
    except OSError:
        info['driver_sha256'] = 'not given'
    try:
        info['commit'] = subprocess.run(['git', 'rev-parse', 'HEAD'], cwd=ROOT, capture_output=True, text=True,
                                        timeout=10).stdout.strip() or 'not given'
    except (OSError, subprocess.SubprocessError):
        info['commit'] = 'not given'
    return info


def write_jsonl(path, records):
    with open(path, 'w', encoding='utf-8') as f:
        for r in records:
            f.write(json.dumps(r, sort_keys=True, ensure_ascii=False) + '\n')


def load_experiment(path):
    with open(path, encoding='utf-8') as f:
        return validate(json.load(f))


def main(argv=None):
    ap = argparse.ArgumentParser(description='Run one experiment of the Bench through the driver.')
    ap.add_argument('experiment', help='ux/bench/experiments/<name>.json')
    ap.add_argument('--driver', default=str(ROOT / 'build' / 'adf'))
    ap.add_argument('--out', default=str(HERE / 'out'))
    a = ap.parse_args(argv)
    e = load_experiment(a.experiment)
    if not Path(a.driver).exists():
        sys.exit('no driver at %s: run `make -j2 && make -C tools/adf` first' % a.driver)
    cells = plan(e)
    ncalls = sum(1 for c in cells if c['line'] is not None)
    print('%s: %d inputs x %d places = %d cells, %d driver calls; budget %d cells, %s s'
          % (e['name'], len(e['inputs']), len(e['rows']), len(cells), ncalls, e['budget']['max_cells'],
             e['budget']['timeout']))
    records, run = sweep(e, a.driver)
    run.update(provenance(a.driver))
    os.makedirs(a.out, exist_ok=True)
    write_jsonl(os.path.join(a.out, e['name'] + '.records.jsonl'), records)
    with open(os.path.join(a.out, e['name'] + '.run.json'), 'w', encoding='utf-8') as f:
        json.dump(run, f, indent=1, sort_keys=True)
        f.write('\n')
    s = stop_line(records)
    print('%s: %d records, %d calls answered in %.3f s; stop line %s; timeout %s'
          % (e['name'], len(records), run['answered'], run['process_time_s'],
             'after the last row' if s['row'] is None else 'at ' + str(s['row']), run['timed_out']))


if __name__ == '__main__':
    main()
