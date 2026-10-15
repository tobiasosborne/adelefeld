#!/usr/bin/env python3
"""The wall renderer of the Bench slice (docs/ux/brief-4-sweep-bench.md, "A first buildable slice", item 2).

    python3 ux/bench/render.py ux/bench/experiments/<name>.json

reads the experiment, ux/bench/out/<name>.records.jsonl and ux/bench/experiments/<name>.findings.json (written by
hand), and writes ux/bench/out/<name>.html: one static page, CSS and JavaScript inline, no network. The markup and
the CSS follow docs/ux/mock-4-sweep-bench.html. Standard library only.
"""
import argparse
import html
import json
import os
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent
sys.path.insert(0, str(HERE))

import sweep  # noqa: E402

STATES = ('pattern', 'sourced', 'refuted', 'open')
STATE_WORDS = {
    'pattern': 'pattern: seen in the swept rows, not proved',
    'sourced': 'sourced: the argument cites a file on disk',
    'refuted': 'refuted: a well contradicts it',
    'open': 'open',
}
# The class of a well, its CSS class, its glyph and its words in the legend (D-UX4, lanes/UX-COMMON.md).
WELL = {
    'ok': ('ok', None, 'OK: the count of the list, or the value'),
    'zero': ('ok zero', '0', 'OK, the complete list is empty (none)'),
    'no': ('no', '', 'proved no: DOMAIN, NO_SOLUTION, NOT_UNIT (solid)'),
    'open': ('nd', '?', 'open: NOT_DETERMINED, UNIT_NOT_CERTIFIED, NEEDS_SPLIT, NOT_UNIQUE (dashed)'),
    'budget': ('lim', 'L', 'budget: LIMIT (dotted)'),
    'plain': ('pl', '!', 'not mathematics: PARSE, UNSUPPORTED (plain)'),
    'na': ('na', '—', 'no command of the driver at this place'),
}

CSS = """
:root {
  --bg: #e9edf1; --plate: #f8fafb; --well-bg: #dfe5eb; --ink: #17212b; --ink-2: #4b5967; --ink-3: #7d8a97;
  --rule: #cbd3dc; --ok: #1d6b86; --ok-ink: #ffffff; --no: #6c7a88; --nd: #a3680c; --arg: #5b4f94;
  --stop: #17212b; --bad: #b3261e; --lit: #f2c94c;
  --f-label: "Barlow Semi Condensed", "Arial Narrow", "Liberation Sans Narrow", sans-serif;
  --f-mono: "Roboto Mono", "DejaVu Sans Mono", ui-monospace, Menlo, monospace;
}
@media (prefers-color-scheme: dark) {
  :root:not([data-theme="light"]) {
    --bg: #12171d; --plate: #1a2129; --well-bg: #252e38; --ink: #e2e8ee; --ink-2: #a6b2be; --ink-3: #74818e;
    --rule: #2f3943; --ok: #5cb6d4; --ok-ink: #0e1a21; --no: #8d9aa7; --nd: #e5ab4f; --arg: #ab9fe6;
    --stop: #e2e8ee; --bad: #f2867e; --lit: #8a6d12; color-scheme: dark;
  }
}
:root[data-theme="dark"] {
  --bg: #12171d; --plate: #1a2129; --well-bg: #252e38; --ink: #e2e8ee; --ink-2: #a6b2be; --ink-3: #74818e;
  --rule: #2f3943; --ok: #5cb6d4; --ok-ink: #0e1a21; --no: #8d9aa7; --nd: #e5ab4f; --arg: #ab9fe6;
  --stop: #e2e8ee; --bad: #f2867e; --lit: #8a6d12; color-scheme: dark;
}
* { box-sizing: border-box; }
body { background: var(--bg); color: var(--ink); font-family: var(--f-label); font-size: 15px; line-height: 1.45;
  padding-inline: 16px; padding-block: 0 40px; margin: 0; }
.wrap { max-width: 1260px; margin: 0 auto; }
.q { padding-block: 18px 6px; display: flex; flex-wrap: wrap; gap: 6px 18px; align-items: baseline; }
.q .lab { font-weight: 700; font-size: 12px; letter-spacing: 0.14em; text-transform: uppercase;
  color: var(--ink-3); }
.q h1 { margin: 0; font-size: 22px; font-weight: 600; }
.q .cost { margin-left: auto; font-family: var(--f-mono); font-size: 12.5px; color: var(--ink-2); }
.team { font-family: var(--f-mono); font-size: 13px; color: var(--ink-2); background: var(--plate);
  border: 1px solid var(--rule); border-radius: 6px; padding: 6px 12px; margin-bottom: 14px; }
.main { display: grid; grid-template-columns: minmax(0, 1fr) 320px; gap: 18px; align-items: start; }
@media (max-width: 1000px) { .main { grid-template-columns: minmax(0, 1fr); } }
.plate { background: var(--plate); border: 1px solid var(--rule); border-radius: 10px; padding: 12px 12px 14px;
  min-width: 0; }
.tools { display: flex; flex-wrap: wrap; gap: 8px; align-items: center; margin-bottom: 10px; font-size: 13px;
  color: var(--ink-2); }
.tools select { font: 600 13px var(--f-label); }
.scroll { overflow-x: auto; }
table { border-collapse: separate; border-spacing: 0; width: 100%; }
thead th { font-weight: 600; font-size: 13px; color: var(--ink-2); padding: 4px 6px 8px; text-align: center;
  vertical-align: bottom; border-bottom: 1px solid var(--rule); }
thead th .in { font-family: var(--f-mono); font-size: 13px; color: var(--ink); display: block; }
thead th .fp { font-size: 11px; color: var(--ink-3); display: block; font-weight: 500; }
thead th.first { text-align: left; }
tbody th { text-align: left; font-family: var(--f-mono); font-weight: 500; font-size: 13.5px;
  padding: 3px 10px 3px 4px; white-space: nowrap; }
tbody th .m8 { font-family: var(--f-label); font-size: 11px; color: var(--ink-3); margin-left: 8px;
  font-weight: 400; }
tbody tr.arch th { color: var(--ok); }
tbody tr.arch td, tbody tr.arch th { border-bottom: 1px dashed var(--rule); }
td { text-align: center; padding: 3px 4px; white-space: nowrap; }
.w { display: inline-grid; place-items: center; min-width: 24px; height: 24px; border-radius: 12px;
  font: 600 12px var(--f-mono); vertical-align: middle; cursor: default; }
.w.ok { background: var(--ok); color: var(--ok-ink); padding: 0 4px; }
.w.ok.zero { background: transparent; color: var(--ok); border: 2px solid var(--ok); width: 24px; }
.w.ok.val { border-radius: 6px; font-weight: 500; font-size: 11px; padding: 0 6px; }
.w.no { width: 24px; border: 1.5px solid var(--no); position: relative; }
.w.no::after { content: ""; position: absolute; width: 2px; height: 24px; background: var(--no);
  transform: rotate(45deg); top: -1.5px; left: 9.5px; }
.w.nd { width: 24px; border: 1.8px dashed var(--nd); color: var(--nd); background: transparent; }
.w.lim { width: 22px; height: 22px; border: 2px dotted var(--ink-2); border-radius: 0; color: var(--ink-2); }
.w.pl { color: var(--ink-2); border-radius: 0; font-weight: 700; }
.w.na { color: var(--ink-3); border-radius: 0; }
.got { font: 500 10.5px var(--f-mono); color: var(--ink-2); }
.w.lit { outline: 3px solid var(--lit); outline-offset: 1px; }
tr.grp th { font: 700 11px var(--f-label); letter-spacing: 0.12em; text-transform: uppercase; color: var(--ink-3);
  padding-top: 10px; text-align: left; }
.stop { border-top: 3px solid var(--stop); padding: 8px 4px 0; display: flex; flex-wrap: wrap; gap: 6px 16px;
  align-items: baseline; }
.stop b { font-size: 14px; }
.stop span { font-size: 13px; color: var(--ink-2); }
.argband { margin-top: 8px; border: 1.5px solid var(--arg); border-radius: 4px; padding: 6px 10px;
  font-size: 13.5px;
  background: repeating-linear-gradient(135deg, color-mix(in srgb, var(--arg) 12%, transparent) 0 6px,
  transparent 6px 12px); }
.argband b { color: var(--arg); }
.legend { display: flex; flex-wrap: wrap; gap: 6px 18px; margin-top: 12px; font-size: 13px; color: var(--ink-2);
  align-items: center; }
.legend span { display: inline-flex; gap: 6px; align-items: center; }
.rail { display: grid; gap: 10px; min-width: 0; }
.rail h2 { margin: 0; font-size: 12px; letter-spacing: 0.14em; text-transform: uppercase; color: var(--ink-3);
  font-weight: 700; }
.find { background: var(--plate); border: 1px solid var(--rule); border-radius: 6px; padding: 10px 12px;
  display: grid; gap: 4px; min-width: 0; }
.find:hover, .find:focus-within { border-color: var(--lit); }
.find .id { font-family: var(--f-mono); font-size: 11.5px; color: var(--ink-3); }
.find .t { font-size: 15px; font-weight: 500; }
.find .ev { font-size: 13px; color: var(--ink-2); }
.find .ev code, .find .src code { font-family: var(--f-mono); font-size: 12px; }
.find .src { font-size: 12.5px; color: var(--ink-2); border-left: 3px double var(--ink-3); padding-left: 8px; }
.find .bad { font-size: 13px; color: var(--bad); font-weight: 600; }
.badge { justify-self: start; font-weight: 700; font-size: 11px; letter-spacing: 0.08em; text-transform: uppercase;
  padding: 1px 8px; }
.st-pattern { color: var(--ok); border: 1.5px solid currentColor; border-radius: 10px; }
.st-sourced { color: var(--arg); border: 4px double currentColor; border-radius: 0; }
.st-refuted { color: var(--bad); border: 1.5px solid currentColor; border-radius: 0;
  text-decoration: line-through; }
.st-open { color: var(--nd); border: 1.5px dashed currentColor; border-radius: 3px; }
.checks { margin-top: 14px; font-size: 13px; }
.checks code { font-family: var(--f-mono); font-size: 12px; }
.foot { margin-top: 20px; font-size: 12px; color: var(--ink-3); }
"""

JS = r"""
(function () {
  var tbody = document.getElementById('wall');
  var rows = Array.prototype.slice.call(tbody.querySelectorAll('tr[data-p]'));
  var sel = document.getElementById('order');
  function regroup() {
    Array.prototype.slice.call(tbody.querySelectorAll('tr.grp')).forEach(function (r) { r.remove(); });
    var m = parseInt(sel.value, 10);
    if (!m) { rows.forEach(function (r) { tbody.appendChild(r); }); return; }
    var groups = {}, keys = [];
    rows.forEach(function (r) {
      var p = parseInt(r.getAttribute('data-p'), 10);
      var k = (p === 0 || m % p === 0) ? 'R and p dividing ' + m : 'p = ' + (p % m) + ' mod ' + m;
      if (!(k in groups)) { groups[k] = []; keys.push(k); }
      groups[k].push(r);
    });
    keys.sort(function (a, b) {
      function key(k) { return k.charAt(0) === 'R' ? -1 : parseInt(k.slice(4), 10); }
      return key(a) - key(b);
    });
    keys.forEach(function (k) {
      var h = document.createElement('tr'); h.className = 'grp';
      var th = document.createElement('th'); th.colSpan = 99; th.textContent = k + ' (' + groups[k].length + ')';
      h.appendChild(th); tbody.appendChild(h);
      groups[k].forEach(function (r) { tbody.appendChild(r); });
    });
  }
  sel.addEventListener('change', regroup);
  var m = /^#mod(\d+)$/.exec(location.hash);
  if (m && sel.querySelector('option[value="' + m[1] + '"]')) { sel.value = m[1]; regroup(); }
  Array.prototype.slice.call(document.querySelectorAll('.find[data-cells]')).forEach(function (f) {
    var ids = f.getAttribute('data-cells').split(' ').filter(Boolean);
    function set(on) {
      ids.forEach(function (id) { var w = document.getElementById(id); if (w) w.classList.toggle('lit', on); });
    }
    f.addEventListener('mouseenter', function () { set(true); });
    f.addEventListener('mouseleave', function () { set(false); });
    f.addEventListener('focusin', function () { set(true); });
    f.addEventListener('focusout', function () { set(false); });
  });
})();
"""

E = html.escape


def wells(records):
    return [r for r in records if r['kind'] == 'well']


def place_text(p):
    return 'R' if p == 'R' else str(p)


def cell_id(r):
    return 'w-%s-%d' % (place_text(r['row']), r['column'])


def shorten(text, width=12):
    """A value text cut for a well: the first digits, an ellipsis, and the precision term if there is one."""
    m = re.match(r'^(.*?)( \+ O\(\d+\^-?\d+\))$', text)
    head, tail = (m.group(1), m.group(2).strip()) if m else (text, '')
    if len(head) > width:
        head = head[:width - 1] + '…'
    return (head + ' ' + tail).strip()


O_RE = re.compile(r'O\((\d+)\^(-?\d+)\)')


def got_html(r):
    """The printed precision of a list well when it is below the requested one: the smallest k of the terms
    O(p^k) in the driver's output, read off the output line (kept in the record and the tooltip)."""
    if r['row'] == 'R' or not r.get('output'):
        return ''
    terms = [(int(k), p) for p, k in O_RE.findall(r['output'])]
    if not terms or min(terms)[0] >= r['prec']:
        return ''
    k, p = min(terms)
    return (' <span class="got" title="printed precision, read off the output; requested prec %d">O(%s^%d)</span>'
            % (r['prec'], p, k))


def well_html(r):
    """One well: its shape is the class (D-UX4); its label the count or the shortened value; the record on hover."""
    cls = r['class']
    tip = ['place %s, input %s' % (place_text(r['row']), r['input'])]
    if r.get('line'):
        tip.append('line: ' + r['line'])
    tip.append('requested prec %d' % r['prec'])
    if 'output' in r and r['output'] is not None:
        tip.append('output: ' + r['output'])
    tip.append('status: %s (class %s)' % (r['status'], cls))
    if r.get('reason'):
        tip.append('reason: ' + r['reason'])
    title = E('\n'.join(tip))
    css, glyph, _ = WELL[cls]
    if cls == 'ok':
        if r['count'] is not None:
            zero = ' zero' if r['count'] == 0 else ''
            return '<span class="w ok%s" id="%s" title="%s">%d</span>%s' % (zero, cell_id(r), title, r['count'],
                                                                           got_html(r))
        return '<span class="w ok val" id="%s" title="%s">%s</span>' % (cell_id(r), title, E(shorten(r['value'])))
    return '<span class="w %s" id="%s" title="%s">%s</span>' % (css, cell_id(r), title, E(glyph))


def in_places(rule_places, row):
    if rule_places == 'all':
        return True
    if rule_places == 'primes':
        return row != 'R'
    if rule_places == 'odd primes':
        return row != 'R' and row != 2
    if isinstance(rule_places, list):
        return row in rule_places
    raise ValueError('rule.places is all, primes, odd primes or a list: %r' % (rule_places,))


def holds(cond, r):
    if 'status' in cond:
        return r['status'] == cond['status']
    if 'count' in cond:
        return r['status'] == 'OK' and r['count'] == cond['count']
    if 'count_min' in cond:
        return r['status'] == 'OK' and r['count'] is not None and r['count'] >= cond['count_min']
    raise ValueError('a condition is status, count or count_min: %r' % (cond,))


def check_rule(finding, e, records):
    """Check a finding's rule against the wells above the stop line: (number of wells checked, exceptions).

    The rule is {places, when?: {mod, residues} or {places}, then, else?}: at a checked place where `when` holds
    (a residue of p, or a list of places; without `when`, always) the well satisfies `then`, elsewhere `else` (no
    `else`: nothing is asked there). The exceptions are the rows where it fails, and `check <id>` for a check
    record that does not satisfy rule.checks[id]."""
    rule = finding.get('rule')
    if not rule:
        return None
    col = e['labels'].index(finding['column']) if 'then' in rule else None
    n, exceptions = 0, []
    swept = sweep.stop_line(records)['swept']
    for r in wells(records):
        if col is None or r['column'] != col or r['row'] not in swept or not in_places(rule['places'], r['row']):
            continue
        if r['status'] == sweep.NO_COMMAND:
            continue
        n += 1
        if 'when' in rule:
            w = rule['when']
            if 'places' in w:
                inside = r['row'] in w['places']
            elif r['row'] == 'R':
                raise ValueError('a rule with `when` (a residue of p) cannot cover R')
            else:
                inside = r['row'] % w['mod'] in w['residues']
            ok = holds(rule['then'], r) if inside else ('else' not in rule or holds(rule['else'], r))
        else:
            ok = holds(rule['then'], r)
        if not ok:
            exceptions.append(r['row'])
    checks = {r['id']: r for r in records if r['kind'] == 'check'}
    for cid, cond in rule.get('checks', {}).items():
        rec = checks.get(cid)
        good = rec is not None and (rec['status'] == cond['status'] if 'status' in cond
                                    else rec['output'] == cond['output'])
        if not good:
            exceptions.append('check ' + cid)
    return n, exceptions


def check_source(src):
    """A source is a file under the repository with a line range that exists in it; else ValueError."""
    path = ROOT / src['file']
    if not path.is_file():
        raise ValueError('source not on disk: %s' % src['file'])
    m = re.fullmatch(r'(\d+)(?:-(\d+))?', str(src['lines']))
    if not m:
        raise ValueError('lines are N or N-M: %r' % src['lines'])
    lo, hi = int(m.group(1)), int(m.group(2) or m.group(1))
    with open(path, encoding='utf-8', errors='replace') as f:
        count = sum(1 for _ in f)
    if not (1 <= lo <= hi <= count):
        raise ValueError('%s has %d lines, not %s' % (src['file'], count, src['lines']))


def next_prime(n):
    k = n + 1
    while not sweep.is_prime(k):
        k += 1
    return k


def stop_text(e, records):
    s = sweep.stop_line(records)
    swept_primes = [p for p in s['swept'] if p != 'R']
    if s['row'] is None:
        top = max(swept_primes) if swept_primes else 1
        head = 'p &gt;= %d: not swept.' % next_prime(top)
    elif s['row'] == 'R' or not swept_primes:
        head = 'Nothing swept.'
        top = 1
    else:
        top = max(swept_primes)
        head = 'p &gt;= %d: not swept.' % s['row']
    gaps = [p for p in sweep.primes_below(top) if p not in swept_primes]
    parts = ['<b>%s</b>' % head]
    if s['below']:
        reasons = sorted({r['reason'] for r in wells(records) if r['status'] == sweep.BELOW})
        parts.append('<span>%d places of the experiment not run (%s).</span>'
                     % (s['below'], E('; '.join(reasons))))
    if gaps:
        parts.append('<span>Not swept either: %s.</span>' % ', '.join(map(str, gaps)))
    parts.append('<span>Nothing below this line is computed. A finding may cover it only by an argument.</span>')
    return '<div class="stop">%s</div>' % ''.join(parts)


def footprint(inp):
    if 'poly' in inp:
        return 'polynomial, coefficients from the constant: ' + inp['poly']
    if sweep.RAT_RE.match(inp['value']):
        return 'a rational: exact at every place'
    return 'footprint not given (the driver gives no places of a value)'


def joined(v):
    """A text of a finding: a string, or a list of strings joined by spaces (so that a JSON line stays short)."""
    return ' '.join(v) if isinstance(v, list) else v


def finding_html(f, e, records):
    f = {k: joined(v) if k in ('text', 'evidence', 'argument') else v for k, v in f.items()}
    f['sources'] = [dict(src, says=joined(src.get('says', ''))) for src in f.get('sources', [])]
    if f.get('state') not in STATES:
        raise ValueError('a finding state is one of %s: %r' % (STATES, f.get('state')))
    for src in f.get('sources', []):
        check_source(src)
    if f['state'] == 'sourced' and not f.get('sources'):
        raise ValueError('a sourced finding cites a file under refs/ by line')
    col = None
    if f.get('column') is not None:
        col = e['labels'].index(f['column'])
    swept = sweep.stop_line(records)['swept']
    covered = [r for r in wells(records) if (col is None or r['column'] == col) and r['row'] in swept
               and in_places((f.get('rule') or {}).get('places', 'all'), r['row'])]
    places = [place_text(r['row']) for r in covered if r['column'] == (col if col is not None else r['column'])]
    places = list(dict.fromkeys(places))
    result = check_rule(f, e, records)
    exc = result[1] if result is not None else []
    state = 'refuted' if exc else f['state']        # the state drawn: the records overrule the file's state
    lit = covered
    if exc and col is not None:                      # a refuted card lights its witnesses
        lit = [r for r in covered if r['row'] in exc]
    out = ['<div class="find" tabindex="0" data-state="%s" data-cells="%s">'
           % (state, ' '.join(cell_id(r) for r in lit))]
    out.append('<span class="id">%s · %s</span>' % (E(f['id']), E('column ' + f['column'] if col is not None
                                                                     else 'whole wall')))
    out.append('<span class="t">%s</span>' % E(f['text']))
    if places:
        out.append('<span class="ev">Computed at %d places: %s.</span>'
                   % (len(places), E(', '.join(places))))
    if result is not None:
        n = result[0]
        word = 'place' if n == 1 else 'places'
        if exc:
            out.append('<span class="bad">Counterexample: the records contradict this finding at %s.</span>'
                       % E(', '.join(map(str, exc))))
        else:
            nchk = len(f['rule'].get('checks', {}))
            out.append('<span class="ev">Rule checked by the renderer on %d %s%s: no exception.</span>'
                       % (n, word, ' and %d check%s' % (nchk, '' if nchk == 1 else 's') if nchk else ''))
    if f.get('evidence'):
        out.append('<span class="ev">%s</span>' % E(f['evidence']))
    checks = {r['id']: r for r in records if r['kind'] == 'check'}
    for cid in f.get('checks_shown', []):
        r = checks[cid]
        out.append('<span class="ev"><code>%s</code> gives <code>%s</code></span>' % (E(r['line']), E(r['output'])))
    for src in f.get('sources', []):
        out.append('<span class="src"><code>%s:%s</code> %s</span>'
                   % (E(src['file']), E(str(src['lines'])), E(src.get('says', ''))))
    if f.get('argument'):
        out.append('<span class="ev"><b>Argument</b> (words, not computed): %s</span>' % E(f['argument']))
    below = 'Below the stop line: ' + ('nothing: the records refute it.' if state == 'refuted' else
                                      'covered by the argument and its source.' if state == 'sourced' else
                                      'argued in words only.' if f.get('argument') else 'nothing said.')
    out.append('<span class="ev">%s</span>' % below)
    out.append('<span class="badge st-%s">%s</span>' % (state, E(STATE_WORDS[state])))
    if state != f['state']:
        out.append('<span class="bad">State in the findings file: written as %s, refuted by the records at %s.'
                   '</span>' % (E(f['state']), E(', '.join(map(str, exc)))))
    out.append('</div>')
    return '\n'.join(out)


_DEFAULT = object()


def shown_path(path):
    """A path as the footer names it: relative to the repository root when it is inside it, else absolute."""
    full = Path(path).resolve()
    try:
        return str(full.relative_to(ROOT))
    except ValueError:
        return str(full)


def render(e, records, findings, records_path=_DEFAULT, findings_path=_DEFAULT):
    """The page. records_path and findings_path are the files read (named in the footer); by default those of
    ux/bench/out/ and ux/bench/experiments/; findings_path None: no findings file was read."""
    if records_path is _DEFAULT:
        records_path = 'ux/bench/out/%s.records.jsonl' % e['name']
    if findings_path is _DEFAULT:
        findings_path = 'ux/bench/experiments/%s.findings.json' % e['name']
    ws = wells(records)
    s = sweep.stop_line(records)
    calls = sum(1 for r in ws if r['line'] is not None)
    done = sum(1 for r in ws if r['status'] not in (sweep.BELOW, sweep.NO_COMMAND))
    below = sum(1 for r in ws if r['status'] == sweep.BELOW)
    h = ['<!doctype html>', '<html lang="en"><head><meta charset="utf-8">',
         '<meta name="viewport" content="width=device-width, initial-scale=1">',
         '<title>Bench %s</title>' % E(e['name']), '<style>%s</style></head><body>' % CSS, '<div class="wrap">']
    h.append('<header class="q"><span class="lab">Question</span><h1>%s</h1>' % E(e['question']))
    h.append('<span class="cost">%d inputs × %d places = %d cells · operation %s · budget %d cells, '
             '%s s per process</span></header>'
             % (len(e['inputs']), len(e['rows']), len(ws), E(e['operation']), e['budget']['max_cells'],
                e['budget']['timeout']))
    h.append('<div class="team">sweeper: %d calls, %d done, %d below the stop line</div>' % (calls, done, below))
    h.append('<div class="main"><section class="plate" aria-label="Wall of places">')
    h.append('<div class="tools"><label>rows <select id="order"><option value="0">by place</option>')
    for m in e.get('regroup', [4, 8]):
        h.append('<option value="%d">by p mod %d</option>' % (m, m))
    h.append('</select></label><span>cell: %s</span></div>' % E(e.get('cell', e['operation'])))
    h.append('<div class="scroll"><table><thead><tr><th class="first">place</th>')
    for inp, label in zip(e['inputs'], e['labels']):
        h.append('<th><span class="in">%s</span><span class="fp">%s</span></th>' % (E(label), E(footprint(inp))))
    h.append('</tr></thead><tbody id="wall">')
    for place in s['swept']:
        row = sorted((r for r in ws if r['row'] == place), key=lambda r: r['column'])
        p = 0 if place == 'R' else place
        h.append('<tr data-p="%d"%s><th scope="row">%s<span class="m8">%s</span></th>%s</tr>'
                 % (p, ' class="arch"' if place == 'R' else '', place_text(place),
                    '' if place == 'R' else 'requested prec %d' % row[0]['prec'] if row else '',
                    ''.join('<td>%s</td>' % well_html(r) for r in row)))
    h.append('</tbody></table></div>')
    h.append(stop_text(e, records))
    for f in findings:
        result = check_rule(f, e, records)
        if result is not None and result[1]:
            continue                                 # a refuted finding covers nothing below the stop line
        if f.get('argument') or f['state'] == 'sourced':
            what = joined(f.get('argument') or f['text'])
            h.append('<div class="argband"><b>%s %s, below the stop line:</b> %s</div>'
                     % (E(f['id']), 'sourced' if f['state'] == 'sourced' else 'argued, not computed', E(what)))
    h.append('<div class="legend" aria-label="Legend">')
    for cls in ('ok', 'zero', 'no', 'open', 'budget', 'plain', 'na'):
        css, glyph, words = WELL[cls]
        h.append('<span><span class="w %s">%s</span> %s</span>' % (css, E(glyph if glyph is not None else '2'),
                                                                     E(words)))
    h.append('<span><span class="got">O(5^6)</span> next to a count: the printed precision, below the requested '
             'prec of the row</span>')
    h.append('</div>')
    checks = [r for r in records if r['kind'] == 'check']
    if checks:
        h.append('<div class="checks"><b>Checks</b> (driver lines outside the wall):<ul>')
        for r in checks:
            h.append('<li><code>%s</code> gives <code>%s</code> (%s)</li>' % (E(r['line']), E(r['output']),
                                                                              E(r['status'])))
        h.append('</ul></div>')
    h.append('</section><aside class="rail" aria-label="Findings"><h2>Findings</h2>')
    if not findings:
        h.append('<div class="find"><span class="ev">No findings.</span></div>')
    for f in findings:
        h.append(finding_html(f, e, records))
    h.append('</aside></div>')
    if findings_path is not None:
        fnd = 'Findings are written by hand in <code>%s</code>.' % E(str(findings_path))
    else:
        fnd = 'There is no findings file (%s.findings.json next to the experiment is read).' % E(e['name'])
    h.append('<p class="foot">Bench slice (ux/bench). Every well is one record of <code>%s</code>; hover a well '
             'for its record, a finding for the wells it rests on. %s</p>' % (E(str(records_path)), fnd))
    h.append('</div><script>%s</script></body></html>' % JS)
    return '\n'.join(h) + '\n'


def read_jsonl(path):
    with open(path, encoding='utf-8') as f:
        return [json.loads(line) for line in f if line.strip()]


def main(argv=None):
    ap = argparse.ArgumentParser(description='Draw the wall of one experiment of the Bench.')
    ap.add_argument('experiment', help='ux/bench/experiments/<name>.json')
    ap.add_argument('--out', default=str(HERE / 'out'))
    a = ap.parse_args(argv)
    e = sweep.load_experiment(a.experiment)
    rpath = os.path.join(a.out, e['name'] + '.records.jsonl')
    records = read_jsonl(rpath)
    fpath = Path(a.experiment).with_name(e['name'] + '.findings.json')
    findings = json.loads(fpath.read_text(encoding='utf-8')) if fpath.exists() else []
    page = render(e, records, findings, records_path=shown_path(rpath),
                  findings_path=shown_path(fpath) if fpath.exists() else None)
    target = os.path.join(a.out, e['name'] + '.html')
    with open(target, 'w', encoding='utf-8') as f:
        f.write(page)
    print('%s: %d wells, %d findings -> %s' % (e['name'], len(wells(records)), len(findings), target))


if __name__ == '__main__':
    main()
