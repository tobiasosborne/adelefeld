#!/usr/bin/env python3
"""The sealer of the Ledger: run the driver on each claim's script and write its seal.

    python3 ux/ledger/seal.py                 seal every claim of ux/ledger/claims/
    python3 ux/ledger/seal.py --only ID       seal one claim
    python3 ux/ledger/seal.py --dry-run       print the driver lines and the expected lines, run nothing
    options: --ledger DIR (claims/ and seals/ under it), --adf PATH (default build/adf)

One driver process per claim, under a timeout of 60 s, `adf -v` with the script on standard input.
The verdicts and the reopening rule are ux/ledger/FORMAT.md. Exit status 0 when every claim (of
--only) is sealed or ratified, 1 otherwise, 2 for a fault of the ledger. Standard library only.
"""
import argparse
import datetime
import os
import re
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import ledger  # noqa: E402

TIMEOUT = 60
TRACE_RE = re.compile(r'^adf: line (\d+): (\S+): ([A-Z_]+)$')
SETTINGS = ('prec', 'digits')


def flint_identity(adf):
    """FLINT_VERSION of the flint/flint.h the build includes, and the libflint that ldd resolves."""
    version = 'not given'
    header = 'not given'
    for d in ('/usr/include', '/usr/local/include'):
        h = os.path.join(d, 'flint', 'flint.h')
        if os.path.exists(h):
            with open(h, errors='replace') as f:
                m = re.search(r'^#define FLINT_VERSION "([^"]+)"', f.read(), re.M)
            if m:
                version, header = m.group(1), h
                break
    library = 'not given'
    try:
        out = subprocess.run(['ldd', adf], capture_output=True, text=True, timeout=30).stdout
        m = re.search(r'^\s*libflint\S*\s+=>\s+(\S+)', out, re.M)
        if m:
            library = os.path.realpath(m.group(1))
    except (OSError, subprocess.SubprocessError):
        pass
    return version, header, library


def build_identity(adf):
    try:
        commit = subprocess.run(['git', '-C', ROOT, 'rev-parse', 'HEAD'], capture_output=True,
                                text=True, timeout=30).stdout.strip() or 'not given'
    except (OSError, subprocess.SubprocessError):
        commit = 'not given'
    with open(adf, 'rb') as f:
        adf_sha = ledger.sha256_bytes(f.read())
    version, header, library = flint_identity(adf)
    return {'commit': commit, 'adf_sha256': adf_sha, 'flint_version': version,
            'flint_header': header, 'flint_library': library}


def build_key(b):
    """What makes two builds the same build: the driver's bytes and FLINT (the commit is information)."""
    return (b.get('adf_sha256'), b.get('flint_version'), b.get('flint_library'))


def script_lines_of_output(trace, n_out):
    """For each output line, the number of the script line that wrote it, from the trace of -v:
    every traced command writes one line, except a setting that succeeds. None if it does not fit."""
    lines = []
    for t in trace.split('\n'):
        m = TRACE_RE.match(t)
        if not m:
            continue
        n, op, st = int(m.group(1)), m.group(2), m.group(3)
        if op in SETTINGS and st == 'OK':
            continue
        lines.append(n)
    return lines if len(lines) == n_out else None


def run_driver(adf, script):
    """Run the driver on the script: (actual lines, exit status, script line of each output, note)."""
    data = ''.join(line + '\n' for line in script).encode('utf-8')
    try:
        p = subprocess.run([adf, '-v'], input=data, capture_output=True, timeout=TIMEOUT)
    except subprocess.TimeoutExpired:
        return [], None, None, 'the driver did not end within %d s' % TIMEOUT
    out = p.stdout.decode('utf-8', errors='replace')
    actual = out.split('\n')
    if actual and actual[-1] == '':
        actual.pop()
    note = ''
    if p.returncode not in (0, 1):
        note = 'the driver ended with status %d' % p.returncode
    mapping = script_lines_of_output(p.stderr.decode('utf-8', errors='replace'), len(actual))
    return actual, p.returncode, mapping, note


def run_verdict(claim, actual, exit_status, note):
    if exit_status is None:
        return 'open'  # the timeout is a budget, never a refutation
    if exit_status not in (0, 1):
        return 'refuted'  # a crash of the driver
    return ledger.compare(claim['expect'], actual, claim.get('expect_exit'), exit_status)[0]


def utc_now():
    return datetime.datetime.now(datetime.timezone.utc).strftime('%Y-%m-%dT%H:%M:%SZ')


def seal_claim(ledger_dir, claim, raw, adf, build):
    """Seal one claim. Returns (state, what happened)."""
    cid = claim['id']
    if not claim['script']:
        return 'proposed', 'no script; left proposed'
    fault = ledger.branch_fault(claim)
    if fault:
        return 'proposed', fault
    actual, exit_status, mapping, note = run_driver(adf, claim['script'])
    verdict = run_verdict(claim, actual, exit_status, note)
    new = {'id': cid, 'verdict': verdict, 'run_verdict': verdict, 'actual': actual, 'exit': exit_status,
           'script_line': mapping, 'note': note, 'date': utc_now(), 'build': build,
           'claim_sha256': ledger.sha256_bytes(raw), 'claim_content_sha256': ledger.content_sha256(claim),
           'reopened': None}
    seals = os.path.join(ledger_dir, 'seals')
    os.makedirs(os.path.join(seals, 'history'), exist_ok=True)
    path = os.path.join(seals, cid + '.json')
    old = ledger.load_seal(ledger_dir, cid)
    what = 'first seal'
    if old is not None:
        same_claim = old.get('claim_content_sha256') == new['claim_content_sha256']
        same_lines = old.get('actual') == actual and old.get('exit') == exit_status
        if same_claim and same_lines:
            if build_key(old['build']) == build_key(build):
                return old['verdict'], 'unchanged'
            return old['verdict'], 'unchanged; confirmed by build %s' % build['adf_sha256'][:12]
        stamp = re.sub(r'[^0-9TZ]', '', old.get('date', 'undated'))
        hist = os.path.join(seals, 'history', '%s.%s.json' % (cid, stamp))
        k = 1
        while os.path.exists(hist):
            k += 1
            hist = os.path.join(seals, 'history', '%s.%s-%d.json' % (cid, stamp, k))
        os.replace(path, hist)
        if same_claim:
            # the same claim, other lines: a later build disagrees, or the same build is not deterministic
            new['verdict'] = 'open'
            new['reopened'] = {
                'earlier': dict(old['build'], date=old.get('date'), verdict=old.get('verdict'),
                                actual=old.get('actual')),
                'later': dict(build, date=new['date'], verdict=verdict),
                'why': ('the same build wrote other lines than its earlier seal'
                        if build_key(old['build']) == build_key(build)
                        else 'a later build wrote other lines than the earlier seal'),
            }
            what = 'REOPENED (%s)' % new['reopened']['why']
        else:
            what = 'sealed again (the claim changed)'
    ledger.write_json(path, new)
    return new['verdict'], what


def dry_run(claims):
    for claim, _ in claims:
        print('== %s: %s' % (claim['id'], claim['statement']))
        if not claim['script']:
            print('   (no script: the sealer leaves the claim proposed)')
            continue
        print('   driver lines:')
        for line in claim['script']:
            print('     ' + line)
        print('   expected lines:')
        for line in claim['expect']:
            print('     ' + line)
        if 'expect_exit' in claim:
            print('   expected exit status: %d' % claim['expect_exit'])


def main(argv=None):
    ap = argparse.ArgumentParser(description='Seal the claims of a ledger with the driver.')
    ap.add_argument('--ledger', default=HERE, help='the ledger directory (claims/ and seals/)')
    ap.add_argument('--adf', default=os.path.join(ROOT, 'build', 'adf'), help='the driver')
    ap.add_argument('--only', metavar='ID', help='seal only this claim')
    ap.add_argument('--dry-run', action='store_true', help='print the lines, run nothing')
    args = ap.parse_args(argv)
    try:
        claims = ledger.load_claims(args.ledger)
    except (OSError, ValueError) as e:
        print('seal: %s' % e, file=sys.stderr)
        return 2
    if args.only:
        claims = [cr for cr in claims if cr[0]['id'] == args.only]
        if not claims:
            print('seal: no claim %s' % args.only, file=sys.stderr)
            return 2
    if args.dry_run:
        dry_run(claims)
        return 0
    if not os.access(args.adf, os.X_OK):
        print('seal: %s is not built (make -j2 && make -C tools/adf)' % args.adf, file=sys.stderr)
        return 2
    build = build_identity(args.adf)
    t0 = time.monotonic()
    counts = {}
    for claim, raw in claims:
        state, what = seal_claim(args.ledger, claim, raw, args.adf, build)
        if state == 'sealed' and claim['state'] == 'ratified':
            fault = ledger.ratification_fault(claim)
            if fault:
                what += '; ' + fault
            else:
                state = 'ratified'
        counts[state] = counts.get(state, 0) + 1
        print('%-28s %-9s %s' % (claim['id'], state, what))
    dt = time.monotonic() - t0
    print('seal: %d claims; %s; %.1f s; build adf %s, FLINT %s, commit %s' % (
        len(claims), ', '.join('%s %d' % (s, counts.get(s, 0)) for s in ledger.STATES), dt,
        build['adf_sha256'][:12], build['flint_version'], build['commit'][:12]))
    good = counts.get('sealed', 0) + counts.get('ratified', 0)
    return 0 if good == len(claims) else 1


if __name__ == '__main__':
    try:
        sys.exit(main())
    except BrokenPipeError:
        # the reader of standard output went away (`seal.py --dry-run | head`): stop quietly
        os.dup2(os.open(os.devnull, os.O_WRONLY), sys.stdout.fileno())
        sys.exit(1)
