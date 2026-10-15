"""Shared code of the Ledger slice: claims, status classes, the comparison of lines, hashes.

The format is ux/ledger/FORMAT.md. Standard library only.
"""
import hashlib
import json
import os
import re

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

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
# A status line: `error: STATUS`, or `<prefix>: error: STATUS` as the Tate commands print it
# (tests/driver/tate-gamma.out: `continuation S=((1 +/- 0.001) + (0)*i ; 0): error: NOT_DETERMINED`).
# A line of several items separated by `; ` outside brackets is a value line, not one status.
ERROR_RE = re.compile(r'^(?:.*: )?error: ([A-Z_]+)$')
FIELDS = ('id', 'statement', 'script', 'expect', 'places', 'tail', 'argument', 'proposer', 'state')
TAILS = ('nothing said', 'argued')


# The places of a driver line (D2 and U6 of the drive). A command with a place operand computes at
# that place only; its index counts the operands separated by ` with `, 0 being the value. Read from
# tools/adf/README.md: project 222, exp_at and log_at 223-224, sin_at to cosh_at 465-466, roots 161,
# roots_at and root_at 482-483, powrat_at and powunit_at 517-518, Log_at and log_abs_at 614-615,
# Log_refine and log_abs_refine 637-638 (a list), valuation and abs 367, legendre 657, hilbert_at 668,
# local_zeta_factor_at 706, psi_at and psi_strict_at 791, tate_local 989, tate_gamma and tate_epsilon 1009.
PLACE_OPERAND = dict(
    [(c, 1) for c in ('project', 'exp_at', 'log_at', 'sin_at', 'cos_at', 'sinh_at', 'cosh_at', 'roots',
                      'roots_at', 'root_at', 'powrat_at', 'powunit_at', 'Log_at', 'log_abs_at', 'Log_refine',
                      'log_abs_refine', 'valuation', 'abs', 'legendre', 'local_zeta_factor_at', 'psi_at',
                      'psi_strict_at', 'tate_local', 'tate_gamma', 'tate_epsilon')]
    + [('hilbert_at', 2)])
REAL_COMMANDS = ('realroots',)  # the real roots of a polynomial: computed at R
SMALL_PRIMES = [p for p in range(2, 1000) if all(p % q for q in range(2, int(p ** 0.5) + 1))]
MODULUS_RE = re.compile(r'(?:\bp=|\bmod\s+)(\d+)')
REAL_WORD_RE = re.compile(r'\breal\b')


def split_top(text, sep):
    """Split text at sep outside parentheses, brackets and angle brackets."""
    parts, depth, start, i = [], 0, 0, 0
    while i < len(text):
        ch = text[i]
        if ch in '([<':
            depth += 1
        elif ch in ')]>':
            depth -= 1
        elif depth == 0 and text.startswith(sep, i):
            parts.append(text[start:i])
            i += len(sep)
            start = i
            continue
        i += 1
    parts.append(text[start:])
    return parts


def has_real_coordinate(text):
    """Whether a literal `(A ; ...)` or `<A ; ...>` has a real coordinate A other than `*`."""
    for i, ch in enumerate(text):
        if ch not in '(<':
            continue
        depth, j = 0, i + 1
        while j < len(text):
            c = text[j]
            if c in '([<':
                depth += 1
            elif c in ')]>':
                if depth == 0:
                    break
                depth -= 1
            elif c == ';' and depth == 0:
                if text[i + 1:j].strip() != '*':
                    return True
                break
            j += 1
    return False


def factor_places(n, primes, cofactors):
    """The primes below 1000 of n by trial division; what is left over 1 is a cofactor."""
    for p in SMALL_PRIMES:
        if n % p == 0:
            primes.add(p)
            while n % p == 0:
                n //= p
    if n > 1:
        cofactors.add(n)


def line_places(line):
    """The places a driver line computes at: (set of primes, set of cofactors, R named).

    A command of PLACE_OPERAND: the primes and `real` of its place operand (a composite there is no
    place; a number without a factor below 1000 is a cofactor). Any other command: the moduli of
    `p=<n>` and `mod <n>` tokens by trial division, R for the word `real`, a real coordinate or a
    command of REAL_COMMANDS. A comment or an empty line has none."""
    primes, cofactors = set(), set()
    s = line.strip(' \t')
    if not s or s.startswith('#'):
        return primes, cofactors, False
    word = s.split()[0]
    if word in PLACE_OPERAND:
        ops = split_top(s[len(word):], ' with ')
        k = PLACE_OPERAND[word]
        if k >= len(ops):
            return primes, cofactors, False
        real = False
        for tok in re.split(r'[\s,]+', ops[k].strip()):
            if tok == 'real':
                real = True
            elif tok.isdigit() and int(tok) >= 2:
                n = int(tok)
                if n in SMALL_PRIMES:
                    primes.add(n)
                elif n >= 1000 and all(n % p for p in SMALL_PRIMES):
                    cofactors.add(n)
        return primes, cofactors, real
    for m in MODULUS_RE.finditer(s):
        n = int(m.group(1))
        if n >= 2:
            factor_places(n, primes, cofactors)
    real = bool(REAL_WORD_RE.search(s)) or has_real_coordinate(s) or word in REAL_COMMANDS
    return primes, cofactors, real


def cofactor_text(n):
    s = str(n)
    if len(s) > 40:
        s = '%s...%s (%d digits)' % (s[:12], s[-12:], len(s))
    return 'cofactor ' + s


def script_places(lines):
    """The places of the given driver lines, as FORMAT.md writes them: "R", the primes, the cofactors."""
    primes, cofactors, real = set(), set(), False
    for line in lines:
        p, c, r = line_places(line)
        primes |= p
        cofactors |= c
        real = real or r
    return (['R'] if real else []) + sorted(primes) + [cofactor_text(n) for n in sorted(cofactors)]


def computed_places(claim, seal):
    """The places at which a script line that printed ran (by the seal's script_line), or of every
    line when the seal has no mapping. Empty without a seal."""
    if seal is None:
        return []
    script = claim['script']
    lines = seal.get('script_line')
    if lines is None:
        return script_places(script)
    return script_places(script[n - 1] for n in sorted(set(lines)) if n and n <= len(script))


# The root commands and the operand that is the seed of the branch (U3 of the drive), index as in
# PLACE_OPERAND: root_at X with PRIME with DEGREE with SEED and powrat_at X with PRIME with E/N with SEED
# (tools/adf/README.md:483, 517); root X with N with SIGN, the sign 1 by default (549-557); roots_at
# lists every branch (486), so its claim is about all branches.
ROOT_SEED = {'root_at': 3, 'powrat_at': 3, 'root': 2, 'roots_at': None}


def root_lines(script):
    """The script lines that call a root command: (line number, command, seed or None for every branch)."""
    out = []
    for i, line in enumerate(script):
        s = line.strip(' \t')
        if not s or s.startswith('#'):
            continue
        word = s.split()[0]
        if word not in ROOT_SEED:
            continue
        k = ROOT_SEED[word]
        ops = split_top(s[len(word):], ' with ')
        seed = None
        if k is not None:
            seed = ops[k].strip() if k < len(ops) else ('1' if word == 'root' else '')
        out.append((i + 1, word, seed))
    return out


def branch_fault(claim):
    """Why the sealer refuses a claim for its branch, or '' (FORMAT.md, `branch`). A claim whose script
    calls a root command names its branch: `all`, or one seed that every root line of the script uses."""
    lines = root_lines(claim['script'])
    if not lines:
        return ''
    b = claim.get('branch')
    if b is None:
        n, word, _ = lines[0]
        return ('refused: the script calls %s (line %d), and the claim names no branch: add "branch": '
                '"<seed>" or "branch": "all"' % (word, n))
    if b == 'all':
        return ''
    for n, word, seed in lines:
        if seed is None:
            return ('refused: the branch %s is one seed, but %s prints every branch (script line %d): '
                    'use "branch": "all"' % (b, word, n))
        if seed != b:
            return 'refused: the branch %s is not the seed %s of script line %d (%s)' % (b, seed, n, word)
    return ''


# A source cited by file and line (U2 of the drive; CLAUDE.md rules 3 and 4): `refs/src/<path>:<lines>`
# or `docs/<path>:<lines>`, lines `N` or `N-M`.
CITATION_RE = re.compile(r'(?<![A-Za-z0-9_./-])((?:refs/src|docs)/[A-Za-z0-9_./+-]*[A-Za-z0-9_+-])'
                         r':(\d+)(?:-(\d+))?')


def cited_sources(text, root=ROOT):
    """The citations of a text whose file exists under root and has the cited lines: (path, first, last)."""
    out = []
    for m in CITATION_RE.finditer(text):
        path, a = m.group(1), int(m.group(2))
        b = int(m.group(3)) if m.group(3) else a
        full = os.path.join(root, path)
        if '..' in path.split('/') or not os.path.isfile(full) or not 1 <= a <= b:
            continue
        with open(full, 'rb') as f:
            n = f.read().count(b'\n')
        if b <= n + 1:
            out.append((path, a, b))
    return out


def ratification_fault(claim):
    """Why a ratification is refused, or '': a claim with an argued tail is ratified only when its
    argument cites a source by file and line that exists on disk (brief 3, Scenario B step 4)."""
    if claim.get('state') != 'ratified' or claim['tail'] != 'argued' or cited_sources(claim['argument']):
        return ''
    return ('ratification refused: no source (the tail is argued, and the argument cites no file and line '
            'under refs/src/ or docs/ that exists)')


def sha256_bytes(b):
    return hashlib.sha256(b).hexdigest()


def content_sha256(claim):
    """The hash of a claim without its `state`, so that a ratification does not make a seal stale."""
    c = {k: v for k, v in claim.items() if k != 'state'}
    return sha256_bytes(json.dumps(c, sort_keys=True, ensure_ascii=False).encode('utf-8'))


def line_status(line):
    """The status name of an output line of the driver: the name after `error: ` at the start of the
    line or after a prefix ending in `: `, or OK."""
    m = ERROR_RE.match(line)
    if not m or top_level_items(line) > 1:
        return 'OK'
    return m.group(1)


def top_level_items(line):
    """The number of items of a line separated by `; ` outside parentheses and brackets."""
    depth, n = 0, 1
    for i, ch in enumerate(line):
        if ch in '([<':
            depth += 1
        elif ch in ')]>':
            depth -= 1
        elif ch == ';' and depth == 0 and line[i + 1:i + 2] == ' ':
            n += 1
    return n


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
    if 'branch' in claim and not (isinstance(claim['branch'], str) and claim['branch'].strip()):
        faults.append('branch is not a seed or "all"')
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
    fault = branch_fault(claim)
    if fault:
        return 'proposed', 'the sealer ' + fault
    if seal is None:
        return 'proposed', 'not sealed yet'
    if seal.get('claim_content_sha256') != content_sha256(claim):
        return 'proposed', 'the claim was changed after its seal; it must be sealed again'
    v = seal['verdict']
    if claim['state'] == 'ratified':
        if v == 'sealed':
            fault = ratification_fault(claim)
            if fault:
                return 'sealed', fault + ': the ratification is void'
            return 'ratified', ''
        return v, 'the claim says ratified, but its seal is %s: the ratification is void' % v
    return v, ''


def write_json(path, obj):
    tmp = path + '.tmp'
    with open(tmp, 'w', encoding='utf-8') as f:
        json.dump(obj, f, indent=1, ensure_ascii=False)
        f.write('\n')
    os.replace(tmp, path)
