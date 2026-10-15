/* Structural browser smoke check. This minimal DOM is not a visual or browser-engine test.
   Run: timeout 120 node --test --test-isolation=none lanes/ux-atlas/check_viewer.mjs */
import test from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { createRequire } from 'node:module';
import vm from 'node:vm';
const require = createRequire(import.meta.url);
const geometry = require('../../ux/atlas/atlas.js');
const root = new URL('../../ux/atlas/', import.meta.url);
const source = readFileSync(new URL('atlas.html', root), 'utf8');
const script = source.match(/<script>\s*([\s\S]*?)<\/script>/)[1];
const fixture = name => JSON.parse(readFileSync(new URL('scenes/' + name + '.json', root)));
class Element {
  constructor(tag = 'div') { this.tag = tag; this.children = []; this.attributes = {}; this.style = {}; }
  setAttribute(name, value) { this.attributes[name] = value; }
  appendChild(child) { this.children.push(child); return child; }
  replaceChildren(...children) { this.children = children; }
  addEventListener(name, handler) { (this.events ??= {})[name] = handler; }
}
function page(protocol = 'file:', search = '') {
  const elements = new Map();
  const samples = [new Element('a'), new Element('a')];
  const document = {
    getElementById(id) { if (!elements.has(id)) elements.set(id, new Element()); return elements.get(id); },
    createElement(tag) { return new Element(tag); },
    createElementNS(ns, tag) { return new Element(tag); },
    querySelectorAll() { return samples; }
  };
  let fetches = 0;
  const window = { AtlasGeometry: geometry };
  vm.runInNewContext(script, { document, window, location: { protocol, search }, URLSearchParams,
    fetch: async () => { fetches++; return { ok: true, json: async () => fixture('sqrt9') }; }
  }, { timeout: 1000 });
  return { window, elements, samples, fetches: () => fetches };
}
const descendants = element => [element, ...element.children.flatMap(descendants)];

test('Scenario A loads four trees, two real points and changes stored depth', () => {
  const { window, elements } = page();
  window.PlaceAtlas.loadScene(fixture('sqrt9'));
  assert.equal(elements.get('controls').children.length, 4);
  assert.equal(elements.get('value').textContent, '9');
  const card = elements.get('controls').children[0];
  const slider = descendants(card).find(e => e.tag === 'input');
  slider.value = '1'; slider.events.input();
  assert.match(card.children.find(e => e.className === 'count').textContent, /2 branches · 1 lit paths/);
  slider.value = '5'; slider.events.input();
  assert.match(card.children.find(e => e.className === 'count').textContent, /2 branches · 2 lit paths/);
  const nodes = descendants(elements.get('map')).filter(e => e.attributes.tabindex === 0);
  assert.ok(nodes.length > 0);
  nodes.at(-1).events.focus();
  assert.match(elements.get('hover').textContent, /^\[p=7: .+ O\(7\^5\)\]$/);
});
test('Scenario B has three controls and all eight checked rows', () => {
  const { window, elements } = page();
  window.PlaceAtlas.loadScene(fixture('idempotents30'));
  assert.equal(elements.get('controls').children.length, 3);
  const table = descendants(elements.get('reading')).find(e => e.tag === 'tbody');
  assert.equal(table.children.length, 8);
  for (const row of table.children) assert.equal(row.children.at(-1).textContent, 'true');
  assert.ok(descendants(elements.get('map')).some(e => e.textContent === 'every other p'));
});
test('file protocol uses the picker and never fetches a query', async () => {
  const result = page('file:', '?scene=scenes/sqrt9.json');
  assert.equal(result.fetches(), 0);
  assert.ok(result.samples.every(e => e.hidden));
  await result.elements.get('picker').events.change({ target: {
    files: [{ text: async () => JSON.stringify(fixture('sqrt9')) }]
  } });
  assert.equal(result.elements.get('value').textContent, '9');
});
test('served query fetches and loads one scene', async () => {
  const result = page('http:', '?scene=scenes/sqrt9.json');
  await new Promise(setImmediate);
  assert.equal(result.fetches(), 1);
  assert.equal(result.elements.get('value').textContent, '9');
});
test('a driver status becomes a gate at the named call', () => {
  const { window, elements } = page();
  const scene = fixture('sqrt9');
  const status = scene.trees[0].depths['5'];
  status.status = 'NOT_DETERMINED'; status.paths = [];
  window.PlaceAtlas.loadScene(scene);
  assert.ok(descendants(elements.get('map')).some(e => e.attributes['stroke-dasharray'] === '5 4'));
  assert.ok(descendants(elements.get('map')).some(e => e.textContent === 'NOT_DETERMINED'));
});
