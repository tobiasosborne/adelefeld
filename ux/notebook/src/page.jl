# The static page of a notebook (D-UX1: one HTML file, inline CSS and JavaScript, opens from file://).
# The CSS is that of docs/ux/mock-1-adelica-notebook.html:5-126 (place strip, branch chips, status cards),
# without the web fonts (no network) and with the four card shapes of D-UX4.

const CSS = raw"""
:root {
  --paper: #f4f6f8; --sheet: #ffffff; --ink: #18212d; --ink-2: #4a5566; --ink-3: #6b7584; --rule: #dde2e8;
  --accent: #245a8c; --accent-soft: #e4edf6; --open: #9a5d08; --open-soft: #fbf2e3;
  --proved: #3f5a52; --proved-soft: #e8f0ed; --budget: #6a4c9c; --budget-soft: #f1ecf8;
  --fog-a: #d9dee5; --fog-b: #f6f7f9; --stale: #9a5d08;
  --f-ui: "Public Sans", "Segoe UI", system-ui, sans-serif;
  --f-code: "JetBrains Mono", ui-monospace, "SFMono-Regular", Menlo, monospace;
  --f-math: "STIX Two Text", Cambria, Georgia, serif;
}
@media (prefers-color-scheme: dark) {
  :root:not([data-theme="light"]) {
    --paper: #11151b; --sheet: #171c24; --ink: #e4e8ee; --ink-2: #aab3c0; --ink-3: #8a93a2; --rule: #2a313c;
    --accent: #8cb5df; --accent-soft: #1d2a38; --open: #e2a64e; --open-soft: #2c2214;
    --proved: #9cc3b6; --proved-soft: #1a2824; --budget: #b8a0e4; --budget-soft: #241e33;
    --fog-a: #2e3642; --fog-b: #1a1f27; --stale: #e2a64e; color-scheme: dark;
  }
}
:root[data-theme="dark"] {
  --paper: #11151b; --sheet: #171c24; --ink: #e4e8ee; --ink-2: #aab3c0; --ink-3: #8a93a2; --rule: #2a313c;
  --accent: #8cb5df; --accent-soft: #1d2a38; --open: #e2a64e; --open-soft: #2c2214;
  --proved: #9cc3b6; --proved-soft: #1a2824; --budget: #b8a0e4; --budget-soft: #241e33;
  --fog-a: #2e3642; --fog-b: #1a1f27; --stale: #e2a64e; color-scheme: dark;
}
* { box-sizing: border-box; }
body { background: var(--paper); color: var(--ink); font-family: var(--f-ui); font-size: 15px; line-height: 1.5;
  margin: 0; padding: 0 16px 48px; }
.top { max-width: 1180px; margin: 0 auto; display: flex; flex-wrap: wrap; align-items: baseline; gap: 8px 20px;
  padding: 18px 0 14px; border-bottom: 1px solid var(--rule); }
.top h1 { font-size: 17px; font-weight: 700; margin: 0; }
.top .file { font-family: var(--f-code); font-size: 13px; color: var(--ink-2); }
.top .kernel { margin-left: auto; font-size: 12px; color: var(--ink-3); font-family: var(--f-code); }
.wrap { max-width: 1180px; margin: 0 auto; display: grid; grid-template-columns: minmax(0, 1fr) 300px; gap: 28px;
  padding-top: 22px; }
@media (max-width: 900px) { .wrap { grid-template-columns: minmax(0, 1fr); } }
.col { min-width: 0; display: flex; flex-direction: column; gap: 18px; }
.cell { background: var(--sheet); border: 1px solid var(--rule); border-radius: 6px; padding: 12px 14px 14px;
  display: grid; grid-template-columns: 64px minmax(0, 1fr); gap: 6px 10px; }
.cell.stale { border: 1.5px dashed var(--stale); }
.step { grid-column: 1 / -1; margin: 0; font-size: 13px; color: var(--ink-2); }
.lbl { font-family: var(--f-code); font-size: 12px; color: var(--ink-3); padding-top: 2px; }
.lbl.out { color: var(--accent); }
.in { font-family: var(--f-code); font-size: 14px; overflow-x: auto; white-space: pre; min-width: 0; }
.stalemark { grid-column: 2; font-size: 12.5px; color: var(--stale); }
.stalemark button { font: inherit; font-size: 12px; border: 1px solid var(--rule); background: var(--sheet);
  color: var(--ink); border-radius: 4px; padding: 0 6px; cursor: pointer; }
.out { min-width: 0; }
.scroll { overflow-x: auto; }
.strip { display: grid; grid-auto-flow: column; grid-auto-columns: minmax(104px, auto);
  border: 1px solid var(--rule);
  border-radius: 4px; width: max-content; min-width: 100%; }
.pl { padding: 8px 12px; border-right: 1px solid var(--rule); display: flex; flex-direction: column; gap: 2px;
  max-width: 240px; }
.pl:last-child { border-right: 0; }
.pl .name { font-size: 11px; text-transform: uppercase; letter-spacing: 0.08em; color: var(--ink-3);
  font-weight: 600; }
.pl .val { font-family: var(--f-math); font-size: 17px; }
.pl .sub { font-family: var(--f-code); font-size: 12px; color: var(--ink-2); }
.pl.rest, .pl.nofo, .pl.cof {
  background: repeating-linear-gradient(135deg, var(--fog-a) 0 6px, var(--fog-b) 6px 12px); }
.pl.rest .val, .pl.cof .val { font-family: var(--f-ui); font-size: 14px; color: var(--ink-2); }
.dig { font-family: var(--f-code); font-size: 15px; letter-spacing: 0.12em; white-space: nowrap; }
.dig .per { text-decoration: overline; text-decoration-thickness: 1.5px; }
.dig .fog { display: inline-block; width: 74px; height: 1.05em; vertical-align: -2px; margin-right: 4px;
  border-radius: 2px;
  background: repeating-linear-gradient(135deg, var(--fog-a) 0 4px,
      var(--fog-b) 4px 8px); border: 1px solid var(--rule); }
.dig .fog[data-rewrite] { cursor: pointer; }
.dig .fog[data-rewrite]:hover, .dig .fog[data-rewrite]:focus-visible { outline: 2px solid var(--accent); }
.dig .ell { color: var(--ink-3); letter-spacing: 0; }
.refine { font: inherit; font-size: 12px; border: 1px solid var(--rule); background: var(--sheet);
  color: var(--ink); border-radius: 4px; padding: 1px 8px; cursor: pointer; justify-self: start; }
.refine:hover, .refine:focus-visible { border-color: var(--accent); outline: 2px solid var(--accent-soft); }
.fan { display: flex; flex-wrap: wrap; gap: 10px; }
.chip { border: 1px solid var(--rule); border-radius: 6px; padding: 8px 12px; display: grid; gap: 2px; min-width: 0;
  background: var(--sheet); text-align: left; color: var(--ink); font: inherit; }
button.chip { cursor: pointer; }
button.chip:hover, button.chip:focus-visible { border-color: var(--accent); outline: none;
  box-shadow: 0 0 0 2px var(--accent-soft); }
.chip .seed { font-family: var(--f-code); font-size: 12px; color: var(--accent); }
.chip .v { font-family: var(--f-math); font-size: 18px; overflow-wrap: anywhere; }
.chip.picked { border-color: var(--accent); background: var(--accent-soft); }
.fanhead { font-size: 12px; color: var(--ink-3); margin-bottom: 8px; }
.fanhead b { color: var(--ink-2); font-weight: 600; }
.status { border-radius: 6px; padding: 10px 14px; display: grid; gap: 6px; }
.status .nm { font-size: 12px; font-weight: 700; letter-spacing: 0.1em; }
.status .meta { font-family: var(--f-code); font-size: 12px; color: var(--ink-2); }
.status .why { font-family: var(--f-math); font-size: 15px; }
.s-dashed { border: 1.5px dashed var(--open); background: var(--open-soft); }
.s-dashed .nm { color: var(--open); }
.s-solid { border: 1.5px solid var(--proved); background: var(--proved-soft); }
.s-solid .nm { color: var(--proved); }
.s-dotted { border: 2px dotted var(--budget); background: var(--budget-soft); }
.s-dotted .nm { color: var(--budget); }
.s-plain { border: 1px solid var(--rule); background: var(--sheet); }
.acts { display: flex; flex-wrap: wrap; gap: 8px; }
.act { font-family: var(--f-code); font-size: 12.5px; border: 1px solid var(--rule); background: var(--sheet);
  border-radius: 4px; padding: 3px 8px; }
.note { font-size: 13px; color: var(--ink-2); margin: 8px 0 0; }
.dump { margin-top: 6px; font-size: 12px; color: var(--ink-3); }
.dump code { font-family: var(--f-code); overflow-wrap: anywhere; }
.foot { max-width: 1180px; margin: 28px auto 0; padding-top: 10px; border-top: 1px solid var(--rule);
  font-size: 12px; color: var(--ink-3); }
pre.plain, pre.jerr { margin: 0; font-family: var(--f-code); font-size: 13px; white-space: pre-wrap; }
pre.jerr { color: var(--open); }
.margin { min-width: 0; display: flex; flex-direction: column; gap: 14px; font-size: 13px; color: var(--ink-2); }
.margin h2 { font-size: 12px; text-transform: uppercase; letter-spacing: 0.1em; margin: 0; color: var(--ink-3); }
.margin p { margin: 0; }
.legend { display: grid; gap: 6px; }
.sw { display: inline-block; width: 26px; height: 12px; vertical-align: -1px; margin-right: 8px;
  border-radius: 2px; }
.sw.fogsw { background: repeating-linear-gradient(135deg, var(--fog-a) 0 4px, var(--fog-b) 4px 8px);
  border: 1px solid var(--rule); }
.sw.d { border: 1.5px dashed var(--open); } .sw.s { border: 1.5px solid var(--proved); }
.sw.o { border: 2px dotted var(--budget); } .sw.p { border: 1px solid var(--rule); }
code { font-family: var(--f-code); font-size: 12.5px; }
@media (max-width: 520px) { .cell { grid-template-columns: minmax(0, 1fr); } .stalemark { grid-column: 1; } }
"""

# A click on an element with data-rewrite (a branch chip, a fog band) puts that text into the cell's input
# and marks the cell: the page cannot compute, the human's kernel recomputes before the cell is kept.
const JS = raw"""
(function () {
  function rewrite(el) {
    var cell = document.getElementById(el.getAttribute('data-cell'));
    if (!cell) return;
    var inp = cell.querySelector('.in');
    if (!inp.hasAttribute('data-original')) inp.setAttribute('data-original', inp.textContent);
    inp.textContent = el.getAttribute('data-rewrite');
    cell.classList.add('stale');
    cell.querySelector('.stalemark').hidden = false;
    cell.querySelectorAll('button.chip').forEach(function (c) { c.classList.toggle('picked', c === el); });
  }
  function restore(cell) {
    var inp = cell.querySelector('.in');
    if (inp.hasAttribute('data-original')) inp.textContent = inp.getAttribute('data-original');
    cell.classList.remove('stale');
    cell.querySelector('.stalemark').hidden = true;
    cell.querySelectorAll('button.chip').forEach(function (c) { c.classList.remove('picked'); });
  }
  document.addEventListener('click', function (ev) {
    var r = ev.target.closest('[data-restore]');
    if (r) { restore(r.closest('.cell')); return; }
    var el = ev.target.closest('[data-rewrite]');
    if (el) rewrite(el);
  });
  document.addEventListener('keydown', function (ev) {
    if (ev.key !== 'Enter' && ev.key !== ' ') return;
    var el = ev.target.closest('[data-rewrite]');
    if (el && el.tagName !== 'BUTTON') { ev.preventDefault(); rewrite(el); }
  });
})();
"""

function legend_html()
    io = IOBuffer()
    print(io, "<div class=\"legend\" aria-label=\"Status classes\">")
    for (shape, sw, cls) in (("dashed", "d", "open: more work may help"), ("solid", "s",
        "proved: the answer is no"),
                             ("dotted", "o", "budget: a limit was hit"), ("plain", "p",
                                 "not a question of mathematics"))
        names = [n for n in STATUS_NAMES if n != "OK" && status_class(n)[2] == shape]
        print(io, "<span><span class=\"sw ", sw, "\"></span>", cls, ": <code>", join(names, " "), "</code></span>")
    end
    print(io, "<span>OK: a value, a fan or a strip; no card</span>")
    print(io, "<span><span class=\"sw fogsw\"></span>fog: digits not known, or no information at a place</span>")
    print(io, "</div>")
    return String(take!(io))
end

function page_html(cells, outs, name; notes = nothing, source = nothing, stamp = stamp_text())
    io = IOBuffer()
    print(io, "<!doctype html>\n<html lang=\"en\">\n<head>\n<meta charset=\"utf-8\">\n",
          "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n",
          "<title>Adelica ", esc(name), "</title>\n<style>", CSS, "</style>\n</head>\n<body>\n")
    print(io, "<header class=\"top\"><h1>Adelica</h1><span class=\"file\">",
        esc(source === nothing ? name : source),
          "</span><span class=\"kernel\">values: julia/Adelefeld · roots, root, recover, project: build/adf · ",
          "a static page: it cannot compute</span></header>\n")
    print(io, "<div class=\"wrap\">\n<main class=\"col\" aria-label=\"Notebook cells\">\n")
    for (i, (c, o)) in enumerate(zip(cells, outs))
        id = "c$(i)"
        print(io, "<section class=\"cell\" id=\"", id, "\" aria-label=\"Cell ", i, "\">\n")
        if notes !== nothing && !isempty(notes[i])
            print(io, "<p class=\"step\">", esc(notes[i]), "</p>\n")
        end
        print(io, "<div class=\"lbl\">In [", i, "]</div><div class=\"in\">", esc(c), "</div>\n")
        print(io, "<div class=\"stalemark\" hidden>edited on this page: re-evaluate to keep. The page cannot ",
              "compute; put this text into the notebook file and run <code>sh ux/notebook/run.sh</code>. ",
              "<button type=\"button\" data-restore>restore</button></div>\n")
        print(io, "<div class=\"lbl out\">Out[", i, "]</div><div class=\"out\">", o.html, "</div>\n</section>\n")
    end
    print(io, "</main>\n<aside class=\"margin\" aria-label=\"How to read this page\">\n")
    print(io,
        "<h2>How to read</h2>\n<p>A value is a strip: the real place R, one column per prime below 1000 that ",
          "divides its modulus or denominator (valuation v, |x|<sub>p</sub>, the p-adic digits with the low ",
          "digit on ",
          "the right, the period overlined), and one column for every other prime.</p>\n",
          "<p>A fan shows every branch of a root; a click on a chip writes its seed into the cell. ",
          "A click on the fog of a single root (above 97: on <i>12 more digits</i>) asks for 12 more digits. ",
          "Both edit the cell text only. Hover a strip or a chip for its set in words.</p>\n",
          "<h2>Statuses</h2>\n", legend_html(), "\n",
          "<h2>Agent window</h2>\n<p><code>", esc(name), ".records.jsonl</code>: one record per output, with the ",
          "value text, the status, the dump (or why there is none), what computed it, and the driver line that ",
          "reproduces it (or why there is none).</p>\n</aside>\n</div>\n")
    print(io, "<footer class=\"foot\">Made by ", esc(stamp), ". The package's load gate checks that the ",
          "run-time FLINT agrees in major and minor version with the one the library was compiled against ",
          "(julia/Adelefeld/src/load.jl:159-166).</footer>\n")
    print(io, "<script>", JS, "</script>\n</body>\n</html>\n")
    return String(take!(io))
end
