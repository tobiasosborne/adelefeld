"""Shared code of the Ledger slice: claims, status classes, the comparison of lines, hashes.

The format is ux/ledger/FORMAT.md. Standard library only.
"""
import hashlib
import json
import os
import re

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

# The names and the shapes of the classes, as the page shows them (D-UX4).
CLASS_INFO = {
    'ok': ('value', 'none'),
    'open': ('open', 'dashed'),
    'no': ('proved no', 'solid'),
    'budget': ('budget', 'dotted'),
    'plain': ('not mathematics', 'plain'),
}

STATES = ('proposed', 'sealed', 'refuted', 'open', 'ratified')
ID_RE = re.compile(r'^[a-z0-9][a-z0-9._-]*$')
ERROR_RE = re.compile(r'^error: ([A-Z_]+)$')
FIELDS = ('id', 'statement', 'script', 'expect', 'places', 'tail', 'argument', 'proposer', 'state')
TAILS = ('nothing said', 'argued')


def sha256_bytes(b):
    return hashlib.sha256(b).hexdigest()


def content_sha256(claim):
    """The hash of a claim without its `state`, so that a ratification does not make a seal stale."""
    c = {k: v for k, v in claim.items() if k != 'state'}
    return sha256_bytes(json.dumps(c, sort_keys=True, ensure_ascii=False).encode('utf-8'))


def line_status(line):
    """The status name of an output line of the driver: the name after `error: `, or OK."""
    m = ERROR_RE.match(line)
    return m.group(1) if m else 'OK'


def line_class(line):
    """The class of D-UX4 of an output line; a status name that status.h does not have is `plain`."""
    return STATUS_CLASS.get(line_status(line), 'plain')


def compare(expect, actual, expect_exit, exit_status):
    """Compare the expected lines with the actual ones (FORMAT.md, "The states").

    Returns (verdict, marks): verdict is sealed, refuted or open; marks has one entry per line of
    the longer list: `agree`, `refutes`, `open`, `missing` (an expected line not written) or
    `extra` (a line written that was not expected)."""
    marks = []
    for i in range(max(len(expect), len(actual))):
        if i >= len(actual):
            marks.append('missing')
        elif i >= len(expect):
            marks.append('extra')
        elif expect[i] == actual[i]:
            marks.append('agree')
        elif line_class(actual[i]) in ('open', 'budget'):
            marks.append('open')
        else:
            marks.append('refutes')
    if any(m in ('refutes', 'missing', 'extra') for m in marks):
        return 'refuted', marks
    if expect_exit is not None and expect_exit != exit_status and 'open' not in marks:
        return 'refuted', marks
    if 'open' in marks:
        return 'open', marks
    return 'sealed', marks


def check_claim(claim, stem):
    """A list of the faults of a claim against FORMAT.md; empty when it is well formed."""
    faults = ['no field %s' % f for f in FIELDS if f not in claim]
    if faults:
        return faults
    if claim['id'] != stem or not ID_RE.match(stem):
        faults.append('the id %r does not match the file name %r' % (claim['id'], stem))
    for f in ('script', 'expect'):
        v = claim[f]
        if not isinstance(v, list) or not all(isinstance(x, str) and '\n' not in x for x in v):
            faults.append('%s is not a list of lines' % f)
    if claim['tail'] not in TAILS:
        faults.append('tail is not one of %s' % (TAILS,))
    if claim['state'] not in ('proposed', 'ratified'):
        faults.append('state of a claim file is proposed or ratified, not %r' % claim['state'])
    for p in claim['places']:
        if not (p == 'R' or (isinstance(p, int) and p >= 2) or (isinstance(p, str) and p.startswith('cofactor '))):
            faults.append('place %r is not R, a prime or a cofactor' % (p,))
    return faults


def load_claims(ledger_dir):
    """The claims of a ledger directory, sorted by id, as (claim, bytes of the file)."""
    d = os.path.join(ledger_dir, 'claims')
    out = []
    for name in sorted(os.listdir(d)):
        if not name.endswith('.json'):
            continue
        with open(os.path.join(d, name), 'rb') as f:
            raw = f.read()
        claim = json.loads(raw.decode('utf-8'))
        faults = check_claim(claim, name[:-5])
        if faults:
            raise ValueError('%s: %s' % (name, '; '.join(faults)))
        out.append((claim, raw))
    out.sort(key=lambda cr: natural_key(cr[0]['id']))
    return out


def natural_key(s):
    return [int(t) if t.isdigit() else t for t in re.split(r'(\d+)', s)]


def load_seal(ledger_dir, cid):
    p = os.path.join(ledger_dir, 'seals', cid + '.json')
    if not os.path.exists(p):
        return None
    with open(p) as f:
        return json.load(f)


def load_history(ledger_dir, cid):
    """The earlier seals of a claim, oldest first."""
    d = os.path.join(ledger_dir, 'seals', 'history')
    if not os.path.isdir(d):
        return []
    out = []
    for name in sorted(os.listdir(d)):
        if name.startswith(cid + '.') and name.endswith('.json') and name[len(cid) + 1:-5].count('.') == 0:
            with open(os.path.join(d, name)) as f:
                out.append(json.load(f))
    return out


def shown_state(claim, seal):
    """The state the page shows (FORMAT.md, "The states"), and a note when the seal is stale."""
    if not claim['script']:
        return 'proposed', 'no script: the driver cannot express this claim yet'
    if seal is None:
        return 'proposed', 'not sealed yet'
    if seal.get('claim_content_sha256') != content_sha256(claim):
        return 'proposed', 'the claim was changed after its seal; it must be sealed again'
    v = seal['verdict']
    if claim['state'] == 'ratified':
        if v == 'sealed':
            return 'ratified', ''
        return v, 'the claim says ratified, but its seal is %s: the ratification is void' % v
    return v, ''


def write_json(path, obj):
    tmp = path + '.tmp'
    with open(tmp, 'w', encoding='utf-8') as f:
        json.dump(obj, f, indent=1, ensure_ascii=False)
        f.write('\n')
    os.replace(tmp, path)
