#!/usr/bin/env python3
"""Write the SVG assets of README.md from live driver runs (lane docs-readme).

Run from the repository root after `make -j2 && make -C tools/adf`:

    python3 lanes/docs-readme/make_assets.py

Writes docs/assets/terminal.svg, docs/assets/places.svg and docs/assets/badges/*.svg. Every driver output shown
in an SVG is produced here by running build/adf (D-DOC3); nothing is typed in by hand except the commands. The
script stops if an output differs from the one README.md quotes (EXPECT below), so a change in the driver is seen.
"""

import os
import subprocess
import sys
from xml.sax.saxutils import escape

ROOT = os.getcwd()
ASSETS = os.path.join(ROOT, "docs", "assets")


def sh(cmd):
    """Run one shell line from the repository root and return its standard output without the last newline."""
    r = subprocess.run(["sh", "-c", cmd], cwd=ROOT, capture_output=True, text=True, timeout=60)
    return r.stdout.rstrip("\n")


def adf(lines):
    """Run the driver on a script of lines and return its output lines."""
    r = subprocess.run(["build/adf"], cwd=ROOT, input="".join(l + "\n" for l in lines), capture_output=True,
                       text=True, timeout=60)
    return r.stdout.rstrip("\n").split("\n")


# ---------------------------------------------------------------------------------------------------------------
# The terminal session (the same commands as lanes/docs-readme/terminal_session.sh)

SESSION = [
    "echo 'add (0 ; 0) with 22/7' | build/adf",
    "echo 'project 22/7 with real 2 3 5 7' | build/adf",
    "printf 'prec 6\\nroots_at -1 with 5 with 2\\n' | build/adf",
    "echo 'roots_at -1 with 3 with 2' | build/adf",
    "echo 'mul (3.0 +/- 0.1 ; 146 mod 1000) with 7' | build/adf",
    "echo 'sin_at (* ; 0 mod 2) with 2' | build/adf",
    "echo 'recover 146 mod 1000 with 30 with 15' | build/adf",
]

EXPECT = [
    "(3.142857142857142857 +/- 2.4e-19 ; 22/7)",
    "real: 3.142857142857142857 +/- 2.4e-19; 2: 22/7; 3: 22/7; 5: 22/7; 7: 22/7",
    "5 [2]: 14557 + O(5^6); 5 [3]: 1068 + O(5^6)",
    "error: DOMAIN",
    "(21 +/- 0.71 ; 1022 mod 7000)",
    "error: NOT_DETERMINED",
    "22/7",
]

# the status class of docs/ux/README.md, "What could be combined": shape and colour of the pill
PILL = {
    "error: DOMAIN": ("proved: no", "#ff7b72", None),
    "error: NOT_DETERMINED": ("open", "#e3b341", "4 3"),
}


def terminal_svg(outputs):
    W = 780
    fs = 13.0
    cw = 7.8                       # 0.6 em: every text line gets textLength = len * cw, so widths are exact
    x0 = 30
    y_first = 72
    lh = 20.5
    nlines = 2 * len(SESSION) + 1
    H = int(y_first + (nlines - 1) * lh + 30)
    rate = 32.0                    # characters per second
    # timeline in seconds
    t = 0.4
    rows = []
    for cmd, out in zip(SESSION, outputs):
        t_act = t
        t0 = t_act + 0.35
        t1 = t0 + len(cmd) / rate
        t_out = t1 + 0.45
        rows.append((cmd, out, t_act, t0, t1, t_out))
        t = t_out + 0.95
    t_fin = t
    T = t_fin + 4.5
    fade = 0.5

    def pc(s):
        return "%.3f%%" % (100.0 * s / T)

    def pc2(s):                    # the keyframe just after s
        return "%.3f%%" % (100.0 * s / T + 0.001)

    css = []
    css.append("text{font-family:ui-monospace,SFMono-Regular,'SF Mono',Menlo,Consolas,'DejaVu Sans Mono',"
               "'Liberation Mono',monospace;font-size:%gpx;white-space:pre}" % fs)
    css.append(".ttl{font-family:-apple-system,'Segoe UI',Helvetica,Arial,sans-serif;font-size:12px;fill:#8b949e}")
    css.append(".pill{font-family:-apple-system,'Segoe UI',Helvetica,Arial,sans-serif;font-size:10.5px}")
    css.append(".pr{fill:#7ee787}.cm{fill:#e6edf3}.out{fill:#a5d6ff}")
    css.append(".cur{fill:#c9d1d9}")
    css.append(".blink{animation:blink 1.06s steps(1,end) infinite}")
    css.append("@keyframes blink{0%{opacity:1}50%{opacity:0}100%{opacity:1}}")
    body = []
    for i, (cmd, out, t_act, t0, t1, t_out) in enumerate(rows):
        y = y_first + 2 * i * lh
        L = len(cmd)
        # appearance of the prompt (at t_act) and of the output (at t_out); all fade at the end of the loop
        for name, ts in (("p%d" % i, t_act), ("o%d" % i, t_out)):
            css.append("@keyframes %s{0%%{opacity:0}%s{opacity:0}%s{opacity:1}%s{opacity:1}100%%{opacity:0}}"
                       % (name, pc(ts), pc2(ts), pc(T - fade)))
            css.append(".%s{animation:%s %.3fs linear infinite}" % (name, name, T))
        # the cover: hides the command, moves right one character per step while typing, then leaves
        css.append("@keyframes c%d{0%%{transform:translateX(0px)}%s{transform:translateX(0px);"
                   "animation-timing-function:steps(%d,end)}%s{transform:translateX(%.1fpx)}"
                   "%s{transform:translateX(%.1fpx)}%s{transform:translateX(%dpx)}100%%{transform:translateX(%dpx)}}"
                   % (i, pc(t0), L, pc(t1), L * cw, pc(t_out), L * cw, pc2(t_out), W, W))
        css.append(".c%d{animation:c%d %.3fs linear infinite}" % (i, i, T))
        # the cursor of this line: visible from t_act until the output appears
        css.append("@keyframes k%d{0%%{opacity:0}%s{opacity:0}%s{opacity:1}%s{opacity:1}%s{opacity:0}100%%{opacity:0}}"
                   % (i, pc(t_act), pc2(t_act), pc(t_out), pc2(t_out)))
        css.append(".k%d{animation:k%d %.3fs linear infinite}" % (i, i, T))
        xc = x0 + 2 * cw
        body.append('<g class="p%d"><text x="%g" y="%.1f" class="pr" textLength="%.1f">$</text></g>'
                    % (i, x0, y, cw))
        body.append('<g class="fd"><text x="%.1f" y="%.1f" class="cm" textLength="%.1f" lengthAdjust="spacing">'
                    '%s</text></g>' % (xc, y, L * cw, escape(cmd)))
        # base transform: off to the right, so that the picture without animation shows every line
        body.append('<g class="c%d" transform="translate(%d,0)">' % (i, W))
        body.append('<rect x="%.1f" y="%.1f" width="%.1f" height="%.1f" fill="#0d1117"/>'
                    % (xc, y - fs - 1, W - xc + 4, lh))
        body.append('<g class="k%d" opacity="0"><rect class="cur blink" x="%.1f" y="%.1f" width="%.1f" '
                    'height="%.1f"/></g>' % (i, xc, y - fs + 1.5, cw, fs + 3))
        body.append('</g>')
        yo = y + lh
        style = "out"
        pill = PILL.get(out)
        fill = pill[1] if pill else None
        body.append('<g class="o%d">' % i)
        body.append('<text x="%g" y="%.1f" class="%s"%s textLength="%.1f" lengthAdjust="spacing">%s</text>'
                    % (x0, yo, style, (' style="fill:%s"' % fill) if fill else "", len(out) * cw, escape(out)))
        if pill:
            label, colour, dash = pill
            px = x0 + len(out) * cw + 16
            pw = 9 + 6.0 * len(label)
            body.append('<rect x="%.1f" y="%.1f" width="%.1f" height="16" rx="8" fill="none" stroke="%s" '
                        'stroke-width="1.2"%s/>' % (px, yo - 12.5, pw, colour,
                                                    (' stroke-dasharray="%s"' % dash) if dash else ""))
            body.append('<text x="%.1f" y="%.1f" class="pill" fill="%s" text-anchor="middle">%s</text>'
                        % (px + pw / 2, yo - 1, colour, escape(label)))
        body.append('</g>')
    # the final prompt with a blinking cursor
    yf = y_first + 2 * len(rows) * lh
    css.append("@keyframes pf{0%%{opacity:0}%s{opacity:0}%s{opacity:1}%s{opacity:1}100%%{opacity:0}}"
               % (pc(t_fin), pc2(t_fin), pc(T - fade)))
    css.append(".pf{animation:pf %.3fs linear infinite}" % T)
    body.append('<g class="pf"><text x="%g" y="%.1f" class="pr" textLength="%.1f">$</text>'
                '<rect class="cur blink" x="%.1f" y="%.1f" width="%.1f" height="%.1f"/></g>'
                % (x0, yf, cw, x0 + 2 * cw, yf - fs + 1.5, cw, fs + 3))
    css.append("@keyframes fd{0%%{opacity:1}%s{opacity:1}100%%{opacity:0}}" % pc(T - fade))
    css.append(".fd{animation:fd %.3fs linear infinite}" % T)
    css.append("@media (prefers-reduced-motion: reduce){*{animation:none !important}}")

    svg = []
    svg.append('<svg xmlns="http://www.w3.org/2000/svg" width="%d" height="%d" viewBox="0 0 %d %d" role="img" '
               'aria-labelledby="title desc">' % (W, H, W, H))
    svg.append('<title id="title">A session of the adelefeld driver build/adf</title>')
    desc = "; ".join("%s gives %s" % (c, o) for c, o in zip(SESSION, outputs))
    svg.append('<desc id="desc">%s</desc>' % escape(desc))
    svg.append('<style>%s</style>' % "".join(css))
    svg.append('<defs><filter id="sh" x="-5%%" y="-5%%" width="110%%" height="120%%"><feDropShadow dx="0" dy="4" '
               'stdDeviation="6" flood-color="#000" flood-opacity="0.35"/></filter>'
               '<clipPath id="body"><rect x="11" y="43" width="%d" height="%d"/></clipPath></defs>' % (W - 22, H - 56))
    svg.append('<rect x="10" y="10" width="%d" height="%d" rx="10" fill="#0d1117" filter="url(#sh)"/>'
               % (W - 20, H - 24))
    svg.append('<rect x="10" y="10" width="%d" height="%d" rx="10" fill="#0d1117" stroke="#30363d"/>'
               % (W - 20, H - 24))
    svg.append('<path d="M10 42 V20 a10 10 0 0 1 10 -10 H%d a10 10 0 0 1 10 10 V42 Z" fill="#161b22"/>' % (W - 20))
    svg.append('<line x1="10" y1="42.5" x2="%d" y2="42.5" stroke="#30363d"/>' % (W - 10))
    for k, c in enumerate(("#ff5f57", "#febc2e", "#28c840")):
        svg.append('<circle cx="%d" cy="26" r="6" fill="%s"/>' % (32 + 20 * k, c))
    svg.append('<text x="%d" y="30" class="ttl" text-anchor="middle">adelefeld: build/adf</text>' % (W // 2))
    svg.append('<g clip-path="url(#body)">')
    svg.extend(body)
    svg.append('</g></svg>')
    return "\n".join(svg) + "\n"


# ---------------------------------------------------------------------------------------------------------------
# The places picture: 22/7 on the real line and in the trees of Z_2, Z_3, Z_5 to depth 4

PRIMES = (2, 3, 5)
DEPTH = 4


def places_numbers():
    lines = []
    for p in PRIMES:
        for k in range(1, DEPTH + 1):
            lines.append("project (* ; 22/7 mod %d) with %d" % (p ** k, p))
    out = adf(lines)
    res = {}
    i = 0
    for p in PRIMES:
        res[p] = []
        for k in range(1, DEPTH + 1):
            want_prefix = "%d: " % p
            want_suffix = " + O(%d^%d)" % (p, k)
            s = out[i]
            i += 1
            if not (s.startswith(want_prefix) and s.endswith(want_suffix)):
                sys.exit("unexpected driver line: %r" % s)
            res[p].append((int(s[len(want_prefix):-len(want_suffix)]), s))
    real = adf(["digits 7", "project 22/7 with real"])[0]
    vals = adf(["valuation (3.1 +/- 0.1 ; 22/7 * [1]) with %d" % q for q in (2, 3, 5, 7, 11)])
    return res, real, vals


def places_svg(res, real, vals):
    W, H = 800, 560
    gold = "#f2cc60"
    css = []
    css.append("text{font-family:-apple-system,'Segoe UI',Helvetica,Arial,sans-serif}")
    css.append(".mono{font-family:ui-monospace,SFMono-Regular,'SF Mono',Menlo,Consolas,'DejaVu Sans Mono',monospace}")
    css.append(".h{font-size:17px;fill:#e6edf3;font-weight:600}.s{font-size:12.5px;fill:#8b949e}")
    css.append(".tick{font-size:11px;fill:#8b949e}.dig{font-size:9.5px;fill:#8b949e}.digl{font-size:9.5px;"
               "fill:#0d1117;font-weight:700}.res{font-size:11px;fill:%s}.drv{font-size:12px;fill:#a5d6ff}"
               ".sum{font-size:11.5px;fill:#c9d1d9}.foot{font-size:12px;fill:#8b949e}" % gold)
    css.append(".spark{animation:run 5s linear infinite}@keyframes run{from{stroke-dashoffset:100}"
               "to{stroke-dashoffset:0}}")
    css.append(".halo{animation:pulse 2.4s ease-in-out infinite}@keyframes pulse{0%{opacity:.15}50%{opacity:.7}"
               "100%{opacity:.15}}")
    css.append("@media (prefers-reduced-motion: reduce){.spark{display:none}*{animation:none !important}}")
    s = []
    s.append('<svg xmlns="http://www.w3.org/2000/svg" width="%d" height="%d" viewBox="0 0 %d %d" role="img" '
             'aria-labelledby="title desc">' % (W, H, W, H))
    s.append('<title id="title">The rational 22/7 at the real place and at the primes 2, 3 and 5</title>')
    s.append('<desc id="desc">%s</desc>' % escape(
        "%s. " % real + "; ".join(" then ".join(line for _, line in res[p]) for p in PRIMES)))
    s.append('<style>%s</style>' % "".join(css))
    s.append('<defs><filter id="glow" x="-50%" y="-50%" width="200%" height="200%"><feGaussianBlur '
             'stdDeviation="2.2" result="b"/><feMerge><feMergeNode in="b"/><feMergeNode in="SourceGraphic"/>'
             '</feMerge></filter></defs>')
    s.append('<rect x="0.5" y="0.5" width="%d" height="%d" rx="12" fill="#0d1117" stroke="#30363d"/>' % (W - 1, H - 1))
    s.append('<text x="28" y="40" class="h">One rational, every place at once: 22/7</text>')
    s.append('<text x="28" y="62" class="s">Top: the real line. Below: the p-adic integers for p = 2, 3, 5 as trees. '
             'A node at depth k is a ball a + p^k Z_p,</text>')
    s.append('<text x="28" y="79" class="s">and its p children fix the next base-p digit. The gold thread is 22/7: '
             'one point of the real line, one path in every tree.</text>')
    # the ruler [0, 5]
    yr = 140
    xa, xb = 70, 730
    sc = (xb - xa) / 5.0
    s.append('<text x="28" y="%d" class="tick">R</text>' % (yr + 4))
    s.append('<line x1="%d" y1="%d" x2="%d" y2="%d" stroke="#8b949e" stroke-width="1.5"/>' % (xa - 10, yr, xb + 10, yr))
    for j in range(0, 21):
        x = xa + j * sc / 4
        big = j % 4 == 0
        s.append('<line x1="%.1f" y1="%d" x2="%.1f" y2="%d" stroke="#8b949e" stroke-width="%s"/>'
                 % (x, yr - (6 if big else 3), x, yr + (6 if big else 3), "1.2" if big else "0.8"))
        if big:
            s.append('<text x="%.1f" y="%d" class="tick" text-anchor="middle">%d</text>' % (x, yr + 21, j // 4))
    xq = xa + sc * 22.0 / 7.0
    s.append('<text x="%.1f" y="%d" class="mono res" text-anchor="middle">%s</text>' % (xq, yr - 16, escape(real)))
    # the trees
    centres = (140, 390, 640)
    y0 = 222
    dy = 54
    span = 150
    for p, cx in zip(PRIMES, centres):
        left = cx - 100
        # lit positions: root, then the lit child of each level
        lit = [(cx - 25, y0)]
        prev = 0
        levels = []
        for k in range(1, DEPTH + 1):
            c = res[p][k - 1][0]
            digit = (c - prev) // p ** (k - 1)
            assert 0 <= digit < p and (c - prev) % p ** (k - 1) == 0
            prev = c
            y = y0 + k * dy
            xs = [left + (j + 0.5) * span / p for j in range(p)]
            levels.append((k, y, xs, digit, c))
            lit.append((xs[digit], y))
        r = 8 if p < 5 else 6.5
        # edges first
        for (k, y, xs, digit, c), (px, py) in zip(levels, lit[:-1]):
            for j, x in enumerate(xs):
                if j != digit:
                    s.append('<line x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f" stroke="#30363d" stroke-width="1"/>'
                             % (px, py, x, y))
        # the thread: from the point on the ruler to the root and down the lit path
        d = "M%.1f %d C%.1f %d %.1f %d %.1f %d" % (xq, yr, xq, yr + 50, lit[0][0], y0 - 50, lit[0][0], y0)
        d += "".join(" L%.1f %.1f" % pt for pt in lit[1:])
        s.append('<path d="%s" fill="none" stroke="%s" stroke-width="2.2" opacity="0.9" filter="url(#glow)"/>'
                 % (d, gold))
        s.append('<path class="spark" d="%s" fill="none" stroke="#fff4c2" stroke-width="3.2" stroke-linecap="round" '
                 'pathLength="100" stroke-dasharray="3 140" stroke-dashoffset="100"/>' % d)
        # nodes
        s.append('<rect x="%.1f" y="%d" width="50" height="22" rx="11" fill="#161b22" stroke="%s"/>'
                 % (lit[0][0] - 25, y0 - 11, gold))
        s.append('<text x="%.1f" y="%d" text-anchor="middle" style="font-size:13px;fill:#e6edf3">Z'
                 '<tspan dy="4" style="font-size:9.5px">%d</tspan></text>' % (lit[0][0], y0 + 4, p))
        for (k, y, xs, digit, c) in levels:
            for j, x in enumerate(xs):
                if j == digit:
                    s.append('<circle class="halo" cx="%.1f" cy="%.1f" r="%.1f" fill="%s" opacity="0.3"/>'
                             % (x, y, r + 5, gold))
                    s.append('<circle cx="%.1f" cy="%.1f" r="%.1f" fill="%s"/>' % (x, y, r, gold))
                    s.append('<text x="%.1f" y="%.1f" class="digl" text-anchor="middle">%d</text>' % (x, y + 3.3, j))
                else:
                    s.append('<circle cx="%.1f" cy="%.1f" r="%.1f" fill="#161b22" stroke="#484f58"/>' % (x, y, r))
                    s.append('<text x="%.1f" y="%.1f" class="dig" text-anchor="middle">%d</text>' % (x, y + 3.3, j))
            s.append('<text x="%.1f" y="%.1f" class="res">%d mod %d</text>' % (left + span + 12, y + 4, c, p ** k))
        # under the tree: the driver line of the deepest level, and the digit sum
        c4, line4 = res[p][-1]
        s.append('<text x="%.1f" y="%d" class="mono drv" text-anchor="middle">%s</text>'
                 % (cx, y0 + DEPTH * dy + 42, escape(line4)))
        digits = [(c4 // p ** k) % p for k in range(DEPTH)]
        assert sum(dg * p ** k for k, dg in enumerate(digits)) == c4
        sup = {1: "", 2: "\u00b2", 3: "\u00b3"}
        terms = ["%d" % digits[0]] + ["%d·%d%s" % (dg, p, sup[k]) for k, dg in enumerate(digits) if k >= 1]
        s.append('<text x="%.1f" y="%d" class="sum" text-anchor="middle">22/7 = %s + …</text>'
                 % (cx, y0 + DEPTH * dy + 62, " + ".join(terms)))
    v = dict(zip((2, 3, 5, 7, 11), vals))
    s.append('<text x="28" y="%d" class="foot">Valuations from the driver: v_2 = %s, v_3 = %s, v_5 = %s, '
             'v_7 = %s (22/7 is not in Z_7), v_11 = %s (22 = 2·11); at every other prime 22/7 is a unit.</text>'
             % (H - 22, v[2], v[3], v[5], v[7], v[11]))
    s.append('</svg>')
    return "\n".join(s) + "\n"


# ---------------------------------------------------------------------------------------------------------------
# Badges: two-part flat badges drawn here (no external service)

def badge(label, value, colour):
    cw = 6.6
    lw = int(len(label) * cw + 12)
    vw = int(len(value) * cw + 12)
    w = lw + vw
    return ('<svg xmlns="http://www.w3.org/2000/svg" width="%d" height="20" viewBox="0 0 %d 20" role="img" '
            'aria-label="%s: %s"><title>%s: %s</title>'
            '<clipPath id="r"><rect width="%d" height="20" rx="3"/></clipPath><g clip-path="url(#r)">'
            '<rect width="%d" height="20" fill="#30363d"/><rect x="%d" width="%d" height="20" fill="%s"/></g>'
            '<g fill="#fff" font-family="Verdana,\'DejaVu Sans\',Geneva,sans-serif" font-size="11">'
            '<text x="6" y="14" textLength="%.1f" lengthAdjust="spacingAndGlyphs">%s</text>'
            '<text x="%d" y="14" textLength="%.1f" lengthAdjust="spacingAndGlyphs">%s</text></g></svg>\n'
            % (w, w, escape(label), escape(value), escape(label), escape(value), w, lw, lw, vw, colour,
               len(label) * cw, escape(label), lw + 6, len(value) * cw, escape(value)))


def status_icons():
    """The four status classes of docs/ux/README.md:58-60, one 22 x 22 icon each, as the bench draws its wells
    (ux/bench/README.md, "What the page shows"): open a dashed ring with ?, proved no a solid ring with a slash,
    budget a dotted square with L, not mathematics a plain !. Colours read on light and dark backgrounds."""
    head = ('<svg xmlns="http://www.w3.org/2000/svg" width="22" height="22" viewBox="0 0 22 22" role="img" '
            'aria-label="%s"><title>%s</title>')
    font = 'font-family="-apple-system,\'Segoe UI\',Helvetica,Arial,sans-serif" font-weight="700"'
    icons = {
        "open": ("open: dashed ring",
                 '<circle cx="11" cy="11" r="8.5" fill="none" stroke="#d29922" stroke-width="2" '
                 'stroke-dasharray="3.2 2.4"/><text x="11" y="15" text-anchor="middle" font-size="11" %s '
                 'fill="#d29922">?</text>' % font),
        "proved-no": ("proved no: solid ring with a slash",
                      '<circle cx="11" cy="11" r="8.5" fill="none" stroke="#e5534b" stroke-width="2"/>'
                      '<line x1="5" y1="17" x2="17" y2="5" stroke="#e5534b" stroke-width="2"/>'),
        "budget": ("budget: dotted square",
                   '<rect x="2.5" y="2.5" width="17" height="17" rx="2" fill="none" stroke="#539bf5" '
                   'stroke-width="2" stroke-dasharray="0.1 3.4" stroke-linecap="round"/><text x="11" y="15" '
                   'text-anchor="middle" font-size="11" %s fill="#539bf5">L</text>' % font),
        "not-math": ("not mathematics: plain",
                     '<text x="11" y="16.5" text-anchor="middle" font-size="15" %s fill="#768390">!</text>' % font),
    }
    os.makedirs(os.path.join(ASSETS, "status"), exist_ok=True)
    for name, (label, body) in icons.items():
        with open(os.path.join(ASSETS, "status", name + ".svg"), "w") as f:
            f.write(head % (label, label) + body + "</svg>\n")


def main():
    if not os.path.exists(os.path.join(ROOT, "build", "adf")):
        sys.exit("build/adf is missing: run `make -j2 && make -C tools/adf` from the repository root first")
    outputs = [sh(c) for c in SESSION]
    if outputs != EXPECT:
        for c, o, e in zip(SESSION, outputs, EXPECT):
            if o != e:
                print("differs: %s\n  got  %r\n  want %r" % (c, o, e), file=sys.stderr)
        sys.exit(1)
    res, real, vals = places_numbers()
    os.makedirs(os.path.join(ASSETS, "badges"), exist_ok=True)
    with open(os.path.join(ASSETS, "terminal.svg"), "w") as f:
        f.write(terminal_svg(outputs))
    with open(os.path.join(ASSETS, "places.svg"), "w") as f:
        f.write(places_svg(res, real, vals))
    ntests = len([n for n in os.listdir(os.path.join(ROOT, "tests")) if n.startswith("test_") and n.endswith(".c")])
    badges = {
        "language": ("language", "C11", "#1f6feb"),
        "flint": ("FLINT", "3.0.1", "#8957e5"),
        "licence": ("licence", "AGPL-3.0", "#2ea043"),
        "tests": ("C tests", "%d programs" % ntests, "#2ea043"),
        "julia": ("Julia tests", "5369 (Pkg.test)", "#2ea043"),
        "suites": ("suites", "5 of 5 green, a9a3bf4", "#2ea043"),
    }
    for name, (lab, val, col) in badges.items():
        with open(os.path.join(ASSETS, "badges", name + ".svg"), "w") as f:
            f.write(badge(lab, val, col))
    status_icons()
    print("terminal.svg: %d commands; places.svg: %s; badges: %d; C test programs: %d"
          % (len(SESSION), ", ".join("%d: %d" % (p, res[p][-1][0]) for p in PRIMES), len(badges), ntests))


if __name__ == "__main__":
    main()
