// ux/notebook/test/page.test.js: the inline script of a generated page, run against a minimal DOM stub.
// Run from the repository root: node --test ux/notebook/test/
// It checks the two edits a page can make: a chip click writes its root(...) call into the cell and marks
// the cell; restore puts the original text back. The attributes are read from ux/notebook/out/a_roots.html.
'use strict';
const test = require('node:test');
const assert = require('node:assert');
const fs = require('node:fs');
const path = require('node:path');

const page = fs.readFileSync(path.join(__dirname, '..', 'out', 'a_roots.html'), 'utf8');
const script = page.match(/<script>([\s\S]*)<\/script>/)[1];

function el(tag, attrs, parent) {
  const e = { tagName: tag, attrs: Object.assign({}, attrs), parent, children: [], hidden: false,
    textContent: '', classes: new Set(),
    getAttribute(k) { return k in this.attrs ? this.attrs[k] : null; },
    setAttribute(k, v) { this.attrs[k] = String(v); },
    hasAttribute(k) { return k in this.attrs; },
    closest(sel) {
      for (let x = this; x; x = x.parent) {
        if (sel === '.cell' && x.classes.has('cell')) return x;
        const m = sel.match(/^\[([a-z-]+)\]$/);
        if (m && x.hasAttribute(m[1])) return x;
      }
      return null;
    },
    querySelector(sel) { return this.querySelectorAll(sel)[0] || null; },
    querySelectorAll(sel) {
      const out = [];
      const walk = (x) => { for (const c of x.children) {
        if ((sel === '.in' && c.classes.has('in')) || (sel === '.stalemark' && c.classes.has('stalemark')) ||
            (sel === 'button.chip' && c.tagName === 'BUTTON' && c.classes.has('chip'))) out.push(c);
        walk(c); } };
      walk(this);
      return out;
    } };
  e.classList = { add: (c) => e.classes.add(c), remove: (c) => e.classes.delete(c),
    toggle: (c, on) => (on ? e.classes.add(c) : e.classes.delete(c)) };
  if (parent) parent.children.push(e);
  return e;
}

// Cell 1 of the page: its input text and its two chips, as the page holds them.
const cellText = page.match(/<section class="cell" id="c1"[\s\S]*?<div class="in">([^<]*)<\/div>/)[1];
const rewrites = [...page.matchAll(/<button class="chip" type="button" data-cell="c1" data-rewrite="([^"]*)"/g)]
  .map((m) => m[1]);

test('the page has two chips in cell 1 with root(...) rewrites', () => {
  assert.strictEqual(cellText, 'roots(9, 2; at = 5)');
  assert.deepStrictEqual(rewrites, ['root(9, 2; at = 5, seed = 2)', 'root(9, 2; at = 5, seed = 3)']);
});

test('a chip click rewrites the cell text and marks it; restore undoes it', () => {
  const handlers = {};
  const cell = el('SECTION', { id: 'c1' }, null); cell.classes.add('cell');
  const inp = el('DIV', {}, cell); inp.classes.add('in'); inp.textContent = cellText;
  const mark = el('DIV', {}, cell); mark.classes.add('stalemark'); mark.hidden = true;
  const restore = el('BUTTON', { 'data-restore': '' }, mark);
  const chips = rewrites.map((r) => { const c = el('BUTTON', { 'data-cell': 'c1', 'data-rewrite': r }, cell);
    c.classes.add('chip'); return c; });
  const seed = el('SPAN', {}, chips[1]);   // a click lands on the seed label inside the chip
  const document = { addEventListener: (k, f) => { handlers[k] = f; },
      getElementById: (id) => (id === 'c1' ? cell : null) };
  new Function('document', script)(document);
  handlers.click({ target: seed });
  assert.strictEqual(inp.textContent, 'root(9, 2; at = 5, seed = 3)');
  assert.ok(cell.classes.has('stale'));
  assert.strictEqual(mark.hidden, false);
  assert.ok(chips[1].classes.has('picked') && !chips[0].classes.has('picked'));
  handlers.click({ target: restore });
  assert.strictEqual(inp.textContent, 'roots(9, 2; at = 5)');
  assert.ok(!cell.classes.has('stale'));
  assert.strictEqual(mark.hidden, true);
});

test('the fog of the single root in cell 4 asks for prec = 32', () => {
  const m = page.match(/<span class="fog" data-cell="c4" data-rewrite="([^"]*)"/);
  assert.strictEqual(m[1], 'root(-1, 2; at = 5, seed = 2, prec = 32)');
});
