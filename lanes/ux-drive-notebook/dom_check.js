const fs = require('node:fs');
const vm = require('node:vm');
const assert = require('node:assert/strict');
const path = require('node:path');
const lane = __dirname;
let checks = 0;
const check = (ok, text) => { assert.ok(ok, text); checks++; console.log('PASS', text); };
const decode = s => s.replaceAll('&quot;', '"').replaceAll('&#39;', "'")
  .replaceAll('&lt;', '<').replaceAll('&gt;', '>').replaceAll('&amp;', '&');
const file = process.argv[2] || 'work/ux/notebook/out/a_roots.html';
const html = fs.readFileSync(path.join(lane, file), 'utf8');
const sections = [...html.matchAll(/<section class="cell"[^>]*id="([^"]+)"[^>]*>([\s\S]*?)<\/section>/g)];
const cells = {};
for (const [,id,body] of sections) {
  const input = decode(body.match(/<div class="in">([\s\S]*?)<\/div>/)[1]);
  const a = {};
  const inp = {textContent: input, hasAttribute: k => k in a, getAttribute: k => a[k],
    setAttribute: (k,v) => a[k] = v};
  const states = new Set(); const mark = {hidden: true};
  const cell = {inp, mark, states, controls: [], querySelector: q => q === '.in' ? inp : mark,
    querySelectorAll: () => cell.controls.filter(c => c.tagName === 'BUTTON'),
    classList: {add: k => states.add(k), remove: k => states.delete(k)}};
  for (const m of body.matchAll(/<(button|span)[^>]*data-cell="([^"]+)"[^>]*data-rewrite="([^"]+)"[^>]*>/g)) {
    const attrs = {'data-cell': m[2], 'data-rewrite': decode(m[3])};
    const control = {tagName: m[1].toUpperCase(), getAttribute: k => attrs[k],
      closest: q => q === '[data-rewrite]' ? control : null,
      classList: {toggle: () => {}, remove: () => {}}};
    cell.controls.push(control);
  }
  cells[id] = cell;
  console.log(id, input, 'controls', cell.controls.length);
  console.log(decode(body.replace(/<[^>]*>/g, ' ')).replace(/\s+/g, ' ').trim());
}
check(sections.length > 0, `${sections.length} cells are present in HTML`);
const scripts = [...html.matchAll(/<script>([\s\S]*?)<\/script>/g)];
const events = {};
const document = {getElementById: id => cells[id], addEventListener: (kind, handler) => events[kind] = handler};
for (const [,s] of scripts) vm.runInNewContext(s, {document});
check(!/<(?:script|link)[^>]*(?:src|href)="https?:/.test(html), 'No remote script or stylesheet');
for (const name of ['OK','NOT_DETERMINED','UNIT_NOT_CERTIFIED','NEEDS_SPLIT','NOT_UNIQUE',
                    'NO_SOLUTION','NOT_UNIT','DOMAIN','UNSUPPORTED','PARSE','LIMIT']) {
  check(html.includes(name), 'Legend contains ' + name);
}
if (file.endsWith('a_roots.html')) {
  const cell = cells.c1; const original = cell.inp.textContent;
  events.click({target: cell.controls[1]});
  check(cell.inp.textContent === 'root(9, 2; at = 5, seed = 3)', 'Chip [3] writes seed into code');
  check(cell.states.has('stale') && !cell.mark.hidden, 'Edited output marked stale');
  const restore = {closest: q => q === '.cell' ? cell : null};
  events.click({target: {closest: q => q === '[data-restore]' ? restore : null}});
  check(cell.inp.textContent === original && cell.mark.hidden, 'Restore returns original code');
  const fog = cells.c4.controls[0];
  events.keydown({key: 'Enter', target: fog, preventDefault: () => {}});
  check(cells.c4.inp.textContent.endsWith('prec = 32)'), 'Fog requests 32 from 20');
  events.keydown({key: 'ArrowUp', altKey: true, target: fog, preventDefault: () => {}});
  check(cells.c4.inp.textContent.endsWith('prec = 32)'), 'Alt-Up has no precision action');
  check(cells.c3.controls.length === 2, 'Fan has branch controls only; no fog refinement before rerun');
  check(!/teichmuller\(/i.test(html), 'Scenario A page omits Teichmueller comparison');
}
console.log('PASS', checks, 'checks; this is HTML and mock DOM inspection, not a rendered screenshot.');
