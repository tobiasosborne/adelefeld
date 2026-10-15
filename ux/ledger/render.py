#!/usr/bin/env python3
"""Render a ledger directory (claims/ and seals/) as one static page, ux/ledger/ledger.html.

    python3 ux/ledger/render.py                   writes ux/ledger/ledger.html
    options: --ledger DIR, --out FILE

The page is a function of the claim and seal files alone (no date of its own), so a second run
writes the same bytes. Inline CSS and JavaScript; it opens from file:// (D-UX1). The look follows
docs/ux/mock-3-claim-ledger.html. Standard library only.
"""
import argparse
import html
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import ledger  # noqa: E402

SHOW_MAX = 160          # longer lines are shown cut, with their length
SCRIPT_OPEN_MAX = 40    # a longer script is folded

LOCAL_RE = re.compile(r'^(\d+)(?: \[[^\]]*\])?: (\d+) \+ O\((\d+)\^(\d+)\)$')


def esc(s):
    return html.escape(str(s), quote=True)


def cut(s):
    if len(s) <= SHOW_MAX:
        return esc(s)
    return '%s<span class="cut"> ... (%d bytes in all)</span>' % (esc(s[:120]), len(s))


def code_spans(escaped):
    """`text` in an escaped argument becomes <code>text</code>."""
    return re.sub(r'`([^`]+)`', r'<code>\1</code>', escaped)


def digits(n, p, k):
    ds = []
    for _ in range(k):
        ds.append(n % p)
        n //= p
    ds.reverse()
    return ''.join(str(d) for d in ds) if p <= 10 else ','.join(str(d) for d in ds)


def digit_difference(expected, actual):
    """For two local lines `p: c + O(p^N)` with the same p and N and integer centres: the base-p
    digits of both (most significant first), the power of p of the lowest digit that differs, and p.
    None when the lines are not of that form."""
    a, b = LOCAL_RE.match(expected), LOCAL_RE.match(actual)
    if not a or not b:
        return None
    p, n = int(a.group(1)), int(a.group(4))
    if int(b.group(1)) != p or int(b.group(3)) != p or int(a.group(3)) != p or int(b.group(4)) != n:
        return None
    x, y = int(a.group(2)), int(b.group(2))
    if x == y or n > 64:
        return None
    k, d = 0, x - y
    while d % p == 0:
        d //= p
        k += 1
    return digits(x, p, n), digits(y, p, n), k, p


# ---- shapes

SVG = {
    'proposed': '<circle cx="8" cy="8" r="6.5" fill="none" stroke="currentColor" stroke-width="1.4"/>',
    'sealed': '<circle cx="8" cy="8" r="6.5" fill="currentColor"/>',
    'refuted': ('<circle cx="8" cy="8" r="6.5" fill="none" stroke="currentColor" stroke-width="1.6"/>'
                '<line x1="2.5" y1="13.5" x2="13.5" y2="2.5" stroke="currentColor" stroke-width="1.8"/>'),
    'open': ('<circle cx="8" cy="8" r="6.5" fill="none" stroke="currentColor" stroke-width="1.8" '
             'stroke-dasharray="3 2"/>'),
    'ratified': ('<circle cx="8" cy="8" r="6.5" fill="currentColor"/>'
                 '<path d="M4.8 8.3 7 10.4 11.3 5.6" fill="none" stroke="var(--leaf)" stroke-width="1.8"/>'),
}


def badge(state, extra=''):
    label = state.upper() + (' &middot; ' + esc(extra) if extra else '')
    return ('<span class="state s-%s"><svg viewBox="0 0 16 16" aria-hidden="true">%s</svg>%s</span>'
            % (state, SVG[state], label))


def cls_tag(line):
    st = ledger.line_status(line)
    c = ledger.line_class(line)
    if c == 'ok':
        return ''
    return '<span class="cls cls-%s" title="%s">%s</span>' % (c, esc(ledger.CLASS_INFO[c][0]), esc(st))


def open_reason(seal):
    if seal is None:
        return ''
    if seal.get('reopened'):
        return 'reopened'
    if seal.get('exit') is None:
        return 'timeout'
    for line in seal['actual']:
        if ledger.line_class(line) in ('open', 'budget'):
            return ledger.line_status(line)
    return ''


def footprint(claim, seal):
    """The strip: R, 2, 3, 5, 7, the other primes, the cofactors, then the tail. A filled box is a place at
    which a script line that printed ran (ledger.computed_places; `seal` is None unless the seal is valid);
    an outlined box is named in `places` without such a line; a pale box is neither."""
    named = set(claim['places'])
    computed = set(ledger.computed_places(claim, seal))
    every = named | computed

    def kind(x):
        return 'c' if x in computed else ('n' if x in named else '')

    def box(label, k, title=None, wide=False):
        c = (k + (' wide' if wide else '')).strip()
        t = ' title="%s"' % esc(title) if title else ''
        return '<span%s%s>%s</span>' % (' class="%s"' % c if c else '', t, esc(label))

    parts = [box(x, kind(x)) for x in ('R', 2, 3, 5, 7)]
    for k in ('c', 'n'):
        extra = sorted(p for p in every if isinstance(p, int) and p > 7 and kind(p) == k)
        if len(extra) > 4:
            parts.append(box('%d ... %d' % (extra[0], extra[-1]), k,
                             '%d primes: %s' % (len(extra), ' '.join(map(str, extra))), wide=True))
        else:
            parts.extend(box(x, k) for x in extra)
        cof = sorted(p for p in every if isinstance(p, str) and p.startswith('cofactor ') and kind(p) == k)
        if len(cof) == 1:
            parts.append(box('cof', k, cof[0]))
        elif cof:
            parts.append(box('cof x%d' % len(cof), k, '; '.join(cof), wide=True))
    if claim['tail'] == 'argued':
        parts.append('<span class="tail arg" title="argued for every other place">all</span>')
    else:
        parts.append('<span class="tail" title="nothing said about any other place">...</span>')
    return '<span class="fp">%s</span>' % ''.join(parts)


# ---- the card

def comparison_rows(claim, seal):
    expect = claim['expect']
    actual = seal['actual'] if seal else []
    if seal:
        _, marks = ledger.compare(expect, actual, claim.get('expect_exit'), seal.get('exit'))
    else:
        marks = ['not run'] * len(expect)
    lines = seal.get('script_line') if seal else None
    rows = []
    for i in range(max(len(expect), len(actual))):
        n = lines[i] if lines and i < len(lines) else None
        src = claim['script'][n - 1] if n and n <= len(claim['script']) else ''
        e = expect[i] if i < len(expect) else ''
        a = actual[i] if i < len(actual) else ''
        m = marks[i]
        sym = {'agree': '=', 'refutes': '&ne;', 'open': '?', 'missing': '&ne;', 'extra': '&ne;'}.get(m, '')
        rows.append('<tr class="m-%s"><td class="ln">%s</td><td class="src">%s</td><td>%s</td>'
                    '<td>%s %s</td><td class="mk" title="%s">%s</td></tr>'
                    % (m.replace(' ', '-'), n or '', cut(src), cut(e) if i < len(expect) else '<i>none</i>',
                       cut(a) if i < len(actual) else '<i>none</i>', cls_tag(a) if i < len(actual) else '',
                       esc(m), sym))
        if m == 'refutes':
            dd = digit_difference(e, a)
            if dd:
                rows.append('<tr class="dd"><td></td><td colspan="4">base-%d digits, lowest %d: expected '
                            '<code>...%s</code>, library <code>...%s</code>; the digit of %d^%d is the '
                            'first that differs</td></tr>' % (dd[3], len(dd[0].split(',')) if dd[3] > 10
                                                              else len(dd[0]), dd[0], dd[1], dd[3], dd[2]))
    return rows


def card(claim, seal, history, state, note, children, qkey):
    cid = claim['id']
    out = ['<article class="card" id="claim-%s" aria-label="Claim %s">' % (esc(cid), esc(cid))]
    reason = open_reason(seal) if state == 'open' else ''
    out.append('<div class="top"><span class="id">%s &middot; by %s</span>%s</div>'
               % (esc(cid), esc(claim['proposer']), badge(state, reason)))
    out.append('<h2>%s</h2>' % esc(claim['statement']))
    if claim.get('question'):
        out.append('<p class="q2">%s</p>' % esc(claim['question']))
    if note:
        out.append('<p class="note">%s</p>' % esc(note))
    if seal and seal.get('reopened') and state == 'open':
        r = seal['reopened']
        out.append('<div class="loud"><b>REOPENED.</b> %s. Earlier: adf %s, FLINT %s, %s, verdict %s. '
                   'Later: adf %s, FLINT %s, %s, verdict %s.</div>'
                   % (esc(r['why'].capitalize()), esc(r['earlier']['adf_sha256'][:12]),
                      esc(r['earlier'].get('flint_version')), esc(r['earlier'].get('date')),
                      esc(r['earlier'].get('verdict')), esc(r['later']['adf_sha256'][:12]),
                      esc(r['later'].get('flint_version')), esc(r['later'].get('date')),
                      esc(r['later'].get('verdict'))))
        old, new = r['earlier'].get('actual') or [], seal['actual']
        k = next((i for i in range(max(len(old), len(new))) if i >= len(old) or i >= len(new) or old[i] != new[i]),
                 None)
        if k is not None:
            out.append('<p class="note">First line that differs, output line %d: earlier <code>%s</code>, later '
                       '<code>%s</code>.</p>' % (k + 1, cut(old[k]) if k < len(old) else 'none',
                                                  cut(new[k]) if k < len(new) else 'none'))
    if claim.get('parent'):
        out.append('<p class="rel">Split from or derived from <a href="#claim-%s">%s</a>.</p>'
                   % (esc(claim['parent']), esc(claim['parent'])))
    if children:
        out.append('<p class="rel">Children: %s.</p>' % ', '.join(
            '<a href="#claim-%s">%s</a> (%s)' % (esc(c), esc(c), esc(s)) for c, s in children))
    # script
    script = claim['script']
    if script:
        numbered = '\n'.join('%3d  %s' % (i + 1, cut(s)) for i, s in enumerate(script))
        pre = '<pre class="script">%s</pre>' % numbered
        if len(script) > SCRIPT_OPEN_MAX:
            pre = '<details><summary>the whole script, %d lines</summary>%s</details>' % (len(script), pre)
        out.append('<div class="sec"><span class="k">Script (driver lines)</span>%s</div>' % pre)
    else:
        out.append('<div class="sec"><span class="k">Script</span><p class="note">none: the driver cannot '
                   'express this claim; the sealer leaves it proposed.</p></div>')
    if claim['expect'] or seal:
        ex = (' &middot; exit status expected %d' % claim['expect_exit']) if 'expect_exit' in claim else ''
        got = (' &middot; written %s' % seal['exit']) if seal else ''
        out.append('<div class="sec"><span class="k">Expected and actual, line by line%s%s</span>'
                   '<div class="tablewrap"><table class="cmp"><thead><tr><th>#</th><th>script line</th>'
                   '<th>expected</th><th>library</th><th></th></tr></thead><tbody>%s</tbody></table></div>'
                   '</div>' % (ex, got, ''.join(comparison_rows(claim, seal))))
    if seal and any(ledger.line_status(x) != 'OK' for x in seal['actual']):
        out.append('<p class="small">Failing place of a status line: <b>not given</b> (the driver prints the '
                   'status only, tools/adf/README.md:253). Dump of a result: <b>not given</b> (the driver has no '
                   'record mode).</p>')
    # footprint
    valid = seal if seal is not None and state != 'proposed' else None
    done = ledger.computed_places(claim, valid)
    out.append('<div class="sec"><span class="k">Footprint</span><div>%s</div><p class="small">%s; %s; %s.</p>'
               '</div>'
               % (footprint(claim, valid),
                  ('places of the claim: ' + ', '.join(map(str, claim['places'])) if claim['places']
                   else 'no place named'),
                  ('computed by a script line: ' + ', '.join(map(str, done)) if done
                   else 'no place computed by a script line'),
                  ('every other place: argued' if claim['tail'] == 'argued'
                   else 'nothing said about any other place')))
    out.append('<div class="sec"><span class="k">Argument (argued, not computed)</span><p class="arg">%s</p></div>'
               % (code_spans(esc(claim['argument'])) if claim['argument'] else '<i>none</i>'))
    # seal
    if seal:
        b = seal['build']
        out.append('<div class="seal"><b>seal</b> %s &middot; run verdict %s &middot; %s<br>'
                   'commit %s &middot; adf sha256 %s<br>FLINT %s (FLINT_VERSION of %s) &middot; %s<br>'
                   'claim sha256 %s%s</div>'
                   % (esc(seal['verdict']), esc(seal.get('run_verdict', seal['verdict'])), esc(seal['date']),
                      esc(b['commit']), esc(b['adf_sha256']), esc(b['flint_version']),
                      esc(b.get('flint_header', 'not given')), esc(b['flint_library']),
                      esc(seal['claim_sha256']),
                      ('<br>note: ' + esc(seal['note'])) if seal.get('note') else ''))
    else:
        out.append('<div class="seal">no seal</div>')
    if history:
        items = ''.join('<li>%s &middot; %s &middot; adf %s &middot; lines written: %d</li>'
                        % (esc(h.get('date')), esc(h.get('verdict')), esc(h['build']['adf_sha256'][:12]),
                           len(h.get('actual', []))) for h in history)
        out.append('<div class="sec"><span class="k">Earlier seals</span><ul class="hist">%s</ul></div>' % items)
    out.append('<p class="why"><a href="#list">back to the list</a></p>')
    out.append('<p class="why">A seal says "this build of the library computed this", not "this is true". '
               'Only the sealer seals; only a person ratifies (no command of this slice does).</p>')
    out.append('</article>')
    return '\n'.join(out)


CSS = r"""
:root { --paper: #eef1ec; --leaf: #fbfcfa; --ink: #1c2420; --ink-2: #4d5852; --ink-3: #6f7a74; --rule: #cdd5cd;
  --sealed: #2d6a4c; --refuted: #a2402e; --open: #93650f; --argued: #44598a; --hl: #e3eae2;
  --f-text: Georgia, "IBM Plex Serif", serif; --f-code: ui-monospace, Menlo, "DejaVu Sans Mono", monospace;
  --f-label: "Arial Narrow", "Helvetica Neue", Arial, sans-serif; }
@media (prefers-color-scheme: dark) { :root:not([data-theme="light"]) {
  --paper: #141816; --leaf: #1b201d; --ink: #e2e8e3; --ink-2: #a9b4ad; --ink-3: #8a968f; --rule: #2e3631;
  --sealed: #7cc39d; --refuted: #e88c78; --open: #e0b25a; --argued: #9db1e0; --hl: #232a26; color-scheme: dark; } }
:root[data-theme="dark"] { --paper: #141816; --leaf: #1b201d; --ink: #e2e8e3; --ink-2: #a9b4ad; --ink-3: #8a968f;
  --rule: #2e3631; --sealed: #7cc39d; --refuted: #e88c78; --open: #e0b25a; --argued: #9db1e0; --hl: #232a26;
  color-scheme: dark; }
* { box-sizing: border-box; }
body { background: var(--paper); color: var(--ink); font-family: var(--f-text); font-size: 15px; line-height: 1.5;
  margin: 0; padding: 0 16px 40px; }
a { color: inherit; }
.head { max-width: 1400px; margin: 0 auto; padding: 18px 0 12px; display: flex; flex-wrap: wrap; gap: 6px 22px;
  align-items: baseline; border-bottom: 2px solid var(--ink); }
.head h1 { margin: 0; font-family: var(--f-label); font-weight: 600; font-size: 20px; letter-spacing: .06em;
  text-transform: uppercase; }
.head .q { font-style: italic; color: var(--ink-2); }
.counts { margin-left: auto; font-family: var(--f-label); font-size: 13px; color: var(--ink-2); display: flex;
  flex-wrap: wrap; gap: 4px 14px; }
.counts b { color: var(--ink); }
.bar { max-width: 1400px; margin: 10px auto 0; display: flex; flex-wrap: wrap; gap: 8px; align-items: center;
  font-family: var(--f-label); font-size: 13px; }
.bar select { max-width: min(420px, 100%); }
.bar button, .bar select { font: inherit; background: var(--leaf); color: var(--ink);
  border: 1px solid var(--ink-2);
  border-radius: 3px; padding: 3px 10px; cursor: pointer; }
.bar button[aria-pressed="true"] { background: var(--ink); color: var(--leaf); }
.grid { max-width: 1400px; margin: 14px auto 0; display: grid;
  grid-template-columns: minmax(0, 1.15fr) minmax(0, 1fr);
  gap: 22px; align-items: start; }
@media (max-width: 980px) { .grid { grid-template-columns: minmax(0, 1fr); }
  .cards { position: static; max-height: none; overflow: visible; } }
@media (max-width: 700px) {
  table.list, table.list tbody { display: block; }
  table.list thead { display: none; }
  table.list tr { display: grid; grid-template-columns: auto 1fr; gap: 4px 10px; padding: 8px 0;
    border-bottom: 1px solid var(--rule); }
  table.list tbody td { border: 0; padding: 0; }
  table.list tbody td.st, table.list tbody td.by { grid-column: 1 / -1; max-width: none; }
  table.list tbody td.fit { width: auto; } }
table.list tr[hidden] { display: none; }
.bar label { display: flex; gap: 4px; align-items: center; max-width: 100%; min-width: 0; }
.bar select { min-width: 0; }
.tablewrap { overflow-x: auto; }
table.list { border-collapse: collapse; width: 100%; }
table.list th { font-family: var(--f-label); font-size: 12px; letter-spacing: .08em; text-transform: uppercase;
  color: var(--ink-3); text-align: left; padding: 6px; border-bottom: 1px solid var(--rule); }
table.list td { padding: 7px 6px; border-bottom: 1px solid var(--rule); vertical-align: middle; }
table.list .cid { display: block; font-family: var(--f-code); font-size: 11px; color: var(--ink-3); }
table.list td.st { font-size: 14px; line-height: 1.35; }
table.list td.fit { width: 1%; white-space: nowrap; }
table.list td.by { max-width: 110px; }
table.list .rs { display: block; font-family: var(--f-label); font-size: 11px; color: var(--open);
  padding-left: 22px; }
table.list td.by { font-family: var(--f-label); font-size: 12px; color: var(--ink-2); }
table.list tr.sel td { background: var(--hl); }
table.list td.st a { text-decoration: none; }
table.list td.st a:hover { text-decoration: underline; }
.state { display: inline-flex; align-items: center; gap: 6px; font-family: var(--f-label); font-size: 12px;
  font-weight: 600; letter-spacing: .05em; white-space: nowrap; }
.state svg { width: 16px; height: 16px; flex: none; }
.s-sealed, .s-ratified { color: var(--sealed); } .s-refuted { color: var(--refuted); }
.s-open { color: var(--open); } .s-proposed { color: var(--ink-3); }
.fp { display: inline-flex; flex-wrap: nowrap; align-items: center; gap: 2px; font-family: var(--f-code);
  font-size: 10.5px;
  white-space: nowrap; }
.fp span { display: inline-block; min-width: 18px; height: 18px; line-height: 16px; text-align: center;
  border: 1px solid var(--rule); color: var(--ink-3); border-radius: 2px; padding: 0 2px; }
.fp span.c { background: var(--ink); color: var(--leaf); border-color: var(--ink); }
.fp span.n { border: 1.5px solid var(--ink); color: var(--ink); }
.fp span.wide { min-width: 52px; }
.fp span.tail { min-width: 30px; border-style: dashed; }
.fp span.tail.arg { border: 1px solid var(--argued); color: var(--argued);
  background: repeating-linear-gradient(135deg, transparent 0 3px, var(--argued) 3px 4px); }
.legend { display: flex; flex-wrap: wrap; gap: 6px 14px; margin-top: 10px; font-family: var(--f-label);
  font-size: 12px;
  color: var(--ink-2); align-items: center; }
.cards { position: sticky; top: 8px; max-height: calc(100vh - 16px); overflow-y: auto; min-width: 0; }
.card { display: none; background: var(--leaf); border: 1px solid var(--rule); border-radius: 4px;
  padding: 16px 18px;
  gap: 12px; align-content: start; min-width: 0; }
.card:target { display: grid; }
.cards:has(.card:target) .intro { display: none; }
.intro { background: var(--leaf); border: 1px solid var(--rule); border-radius: 4px; padding: 16px 18px; }
.card .top { display: flex; flex-wrap: wrap; gap: 8px 14px; align-items: center; justify-content: space-between; }
.card .id { font-family: var(--f-code); font-size: 12px; color: var(--ink-3); }
.card h2 { margin: 0; font-size: 19px; font-weight: 600; line-height: 1.35; }
.q2, .rel, .small { margin: 0; font-size: 13px; color: var(--ink-2); }
.note { margin: 0; font-family: var(--f-label); font-size: 13px; color: var(--open); }
.loud { border: 2px solid var(--open); border-radius: 3px; padding: 8px 10px; font-size: 14px; }
.sec { display: grid; gap: 4px; min-width: 0; }
.sec .k { font-family: var(--f-label); font-size: 11.5px; font-weight: 600; letter-spacing: .1em;
  text-transform: uppercase;
  color: var(--ink-3); }
pre { margin: 0; font-family: var(--f-code); font-size: 12.5px; background: var(--paper);
  border: 1px solid var(--rule);
  border-radius: 3px; padding: 8px 10px; overflow-x: auto; }
summary { cursor: pointer; font-family: var(--f-label); font-size: 13px; }
table.cmp { border-collapse: collapse; width: 100%; font-family: var(--f-code); font-size: 12px; }
table.cmp th { font-family: var(--f-label); font-size: 11px; text-transform: uppercase; letter-spacing: .06em;
  color: var(--ink-3); text-align: left; padding: 3px 5px; border-bottom: 1px solid var(--rule); }
table.cmp td { padding: 3px 5px; border-bottom: 1px solid var(--rule); vertical-align: top;
  overflow-wrap: anywhere; }
table.cmp td.ln { color: var(--ink-3); text-align: right; white-space: nowrap; overflow-wrap: normal; }
table.cmp td.src { color: var(--ink-2); }
table.cmp td.mk { font-weight: 700; text-align: center; }
tr.m-refutes td, tr.m-missing td, tr.m-extra td { background: color-mix(in srgb, var(--refuted) 14%, transparent); }
tr.m-refutes td.mk, tr.m-missing td.mk, tr.m-extra td.mk { color: var(--refuted); }
tr.m-open td { background: color-mix(in srgb, var(--open) 14%, transparent); }
tr.m-open td.mk { color: var(--open); }
tr.m-agree td.mk { color: var(--sealed); }
tr.dd td { font-family: var(--f-text); font-size: 12.5px; color: var(--refuted); }
.cut { color: var(--ink-3); font-style: italic; }
.cls { display: inline-block; font-family: var(--f-label); font-size: 11px; letter-spacing: .04em; padding: 0 5px;
  border-radius: 3px; margin-left: 4px; white-space: nowrap; }
.cls-open { border: 1.5px dashed var(--open); color: var(--open); }
.cls-no { border: 1.5px solid var(--refuted); color: var(--refuted); }
.cls-budget { border: 1.5px dotted var(--argued); color: var(--argued); }
.cls-plain { border: 1.5px solid transparent; color: var(--ink-2); text-decoration: underline; }
.arg { margin: 0; font-style: italic; color: var(--argued); font-size: 14px; white-space: pre-wrap; }
.seal { font-family: var(--f-code); font-size: 11.5px; color: var(--ink-2); border-top: 1px solid var(--rule);
  padding-top: 8px; overflow-wrap: anywhere; }
.hist { margin: 0; padding-left: 18px; font-family: var(--f-code); font-size: 12px; }
.why { margin: 0; font-size: 12px; color: var(--ink-3); font-family: var(--f-label); }
.foot { max-width: 1400px; margin: 22px auto 0; font-size: 12px; color: var(--ink-2); font-family: var(--f-label); }
.foot table { border-collapse: collapse; margin-top: 6px; }
.foot td { padding: 2px 10px 2px 0; }
"""

JS = r"""
(function () {
  var state = 'all', q = 'all';
  var rows = Array.prototype.slice.call(document.querySelectorAll('table.list tbody tr'));
  function apply() {
    var shown = 0;
    rows.forEach(function (r) {
      var ok = (state === 'all' || r.dataset.state === state) && (q === 'all' || r.dataset.q === q);
      r.hidden = !ok;
      if (ok) shown++;
    });
    document.getElementById('shown').textContent = shown;
  }
  document.querySelectorAll('button[data-filter]').forEach(function (b) {
    b.addEventListener('click', function () {
      state = b.dataset.filter;
      document.querySelectorAll('button[data-filter]').forEach(function (x) {
        x.setAttribute('aria-pressed', x === b ? 'true' : 'false');
      });
      apply();
    });
  });
  document.getElementById('qsel').addEventListener('change', function (e) { q = e.target.value; apply(); });
  function mark() {
    var id = decodeURIComponent(location.hash.replace(/^#claim-/, ''));
    rows.forEach(function (r) { r.classList.toggle('sel', r.dataset.id === id); });
  }
  window.addEventListener('hashchange', mark);
  mark();
  apply();
})();
"""


def render(ledger_dir):
    claims = ledger.load_claims(ledger_dir)
    seals = {c['id']: ledger.load_seal(ledger_dir, c['id']) for c, _ in claims}
    states = {}
    notes = {}
    for c, _ in claims:
        states[c['id']], notes[c['id']] = ledger.shown_state(c, seals[c['id']])
    questions = sorted(set(c.get('question', '') for c, _ in claims))
    qkey = {qq: 'q%d' % i for i, qq in enumerate(questions)}
    children = {}
    for c, _ in claims:
        if c.get('parent'):
            children.setdefault(c['parent'], []).append((c['id'], states[c['id']]))
    counts = {s: sum(1 for v in states.values() if v == s) for s in ledger.STATES}

    out = ['<!doctype html>', '<html lang="en">', '<head>', '<meta charset="utf-8">',
           '<meta name="viewport" content="width=device-width, initial-scale=1">',
           '<title>Claim Ledger</title>', '<style>%s</style>' % CSS, '</head>', '<body>']
    out.append('<header class="head"><h1>Ledger</h1><span class="q">claims sealed by the driver build/adf; '
               'agents propose, the sealer seals, people ratify</span><span class="counts"><span><b>%d</b> claims'
               '</span>%s</span></header>' % (len(claims), ''.join(
                   '<span><b>%d</b> %s</span>' % (counts[s], s) for s in ledger.STATES)))
    out.append('<div class="bar" role="toolbar" aria-label="Filters"><span>State:</span>'
               '<button type="button" data-filter="all" aria-pressed="true">all</button>%s'
               '<label>Question: <select id="qsel"><option value="all">all</option>%s</select></label>'
               '<span><b id="shown">%d</b> shown</span></div>' % (
                   ''.join('<button type="button" data-filter="%s" aria-pressed="false">%s (%d)</button>'
                           % (s, s, counts[s]) for s in ledger.STATES),
                   ''.join('<option value="%s">%s</option>' % (qkey[qq], esc(qq or 'no question'))
                           for qq in questions), len(claims)))
    out.append('<div class="grid"><section class="list" id="list" aria-label="Claims"><div class="tablewrap">'
               '<table class="list"><thead><tr><th>Places</th><th>State</th><th>Claim</th><th>By</th>'
               '</tr></thead><tbody>')
    for c, _ in claims:
        cid, st = c['id'], states[c['id']]
        seal = seals[cid]
        reason = open_reason(seal) if st == 'open' else ''
        out.append('<tr data-id="%s" data-state="%s" data-q="%s"><td class="fit">%s</td><td class="fit">%s</td>'
                   '<td class="st"><a href="#claim-%s"><span class="cid">%s</span>%s</a></td>'
                   '<td class="by">%s</td></tr>'
                   % (esc(cid), st, qkey[c.get('question', '')],
                      footprint(c, seal if seal is not None and st != 'proposed' else None),
                      badge(st) + ('<span class="rs">%s</span>' % esc(reason) if reason else ''),
                      esc(cid), esc(cid), esc(c['statement']), esc(c['proposer'])))
    out.append('</tbody></table></div>')
    out.append('<div class="legend"><span class="fp"><span class="c">5</span></span> '
               'computed: a script line ran at this place '
               '<span class="fp"><span class="n">5</span></span> named, no script line computed it '
               '<span class="fp"><span>5</span></span> neither '
               '<span class="fp"><span class="tail arg">all</span></span> argued for every other place '
               '<span class="fp"><span class="tail">...</span></span> nothing said about other places</div>')
    out.append('</section><section class="cards" aria-label="Claim card">')
    out.append('<div class="intro"><p><b>Choose a claim</b> in the list to open its card: the statement, the '
               'script, the expected lines beside the lines the library wrote, the footprint, the argument and '
               'the seal with its build.</p><p class="small">States: %s. A state has a shape as well as a '
               'colour.</p></div>' % ' '.join(badge(s) for s in ledger.STATES))
    for c, _ in claims:
        cid = c['id']
        out.append(card(c, seals[cid], ledger.load_history(ledger_dir, cid), states[cid], notes[cid],
                        children.get(cid, []), qkey))
    out.append('</section></div>')
    rows = ''.join('<tr><td><span class="cls cls-%s">%s</span></td><td>%s</td><td>%s</td></tr>' % (
        k, esc(ledger.CLASS_INFO[k][0]), esc(ledger.CLASS_INFO[k][1]),
        esc(', '.join(n for n, v in ledger.STATUS_CLASS.items() if v == k)))
        for k in ('ok', 'open', 'no', 'budget', 'plain'))
    out.append('<footer class="foot"><b>Status classes of the library lines</b> (D-UX4; names of '
               'include/adelefeld/status.h):<table>%s</table><p>Expected lines are the claimant\'s words; '
               'library lines were written by build/adf when the claim was sealed. Generated by '
               'ux/ledger/render.py from ux/ledger/claims and ux/ledger/seals.</p></footer>' % rows)
    out.append('<script>%s</script>' % JS)
    out.append('</body></html>')
    return '\n'.join(out) + '\n'


def main(argv=None):
    ap = argparse.ArgumentParser(description='Render a ledger as one static page.')
    ap.add_argument('--ledger', default=HERE)
    ap.add_argument('--out', default=None, help='default: ledger.html in the ledger directory')
    args = ap.parse_args(argv)
    page = render(args.ledger)
    path = args.out or os.path.join(args.ledger, 'ledger.html')
    with open(path, 'w', encoding='utf-8') as f:
        f.write(page)
    print('render: %s, %d bytes' % (os.path.relpath(path, ROOT), len(page.encode('utf-8'))))
    return 0


if __name__ == '__main__':
    sys.exit(main())
