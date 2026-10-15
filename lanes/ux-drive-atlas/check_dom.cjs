// Exercise the actual page's load and depth handlers with a minimal, local DOM stand-in.
// This checks emitted elements and text. It does not check browser pixels or layout engines.
const fs = require('node:fs');
const vm = require('node:vm');
const assert = require('node:assert/strict');
class Element {
  constructor(tag) { this.tag = tag; this.children = []; this.attributes = {}; this.style = {}; this.events = {}; }
  set textContent(value) { this.text = String(value); this.children = []; }
  get textContent() { return (this.text || '') + this.children.map(c => c.textContent).join(' '); }
  setAttribute(k, v) { this.attributes[k] = String(v); }
  appendChild(node) { this.children.push(node); return node; }
  replaceChildren(...children) { this.children = children; this.text = ''; }
  addEventListener(name, handler) { this.events[name] = handler; }
}
function walk(e) { return [e, ...e.children.flatMap(walk)]; }
const html = fs.readFileSync('ux/atlas/atlas.html', 'utf8');
const ids = new Map([...html.matchAll(/id="([^"]+)"/g)].map(m => [m[1], new Element('div')]));
const samples = [new Element('a'), new Element('a')];
const document = {
  getElementById: id => ids.get(id), createElement: tag => new Element(tag),
  createElementNS: (_, tag) => new Element(tag), querySelectorAll: selector => {
    assert.equal(selector, '.sample'); return samples;
  }
};
const context = vm.createContext({document, location: {protocol: 'file:', search: ''}, URLSearchParams});
context.window = context;
vm.runInContext(fs.readFileSync('ux/atlas/atlas.js', 'utf8'), context);
const inline = [...html.matchAll(/<script>([\s\S]*?)<\/script>/g)].map(m => m[1]).join('\n');
vm.runInContext(inline, context);
assert(samples.every(a => a.hidden));
const scene = JSON.parse(fs.readFileSync(process.argv[2], 'utf8'));
context.PlaceAtlas.loadScene(scene);
assert.equal(ids.get('map').hidden, false);
assert.equal(ids.get('stdin').textContent, scene.provenance.stdin);
assert.equal(ids.get('stdout').textContent, scene.provenance.stdout);
const sliders = walk(ids.get('controls')).filter(e => e.tag === 'input');
assert.equal(sliders.length, scene.trees.length);
let changes = 0;
for (let i = 0; i < sliders.length; i++) {
  for (let depth = scene.depth_range[0]; depth <= scene.depth_range[1]; depth++) {
    sliders[i].value = String(depth);
    sliders[i].events.input(); changes++;
    assert(ids.get('map').textContent.includes(`depth ${depth}`));
  }
}
const nodes = walk(ids.get('map'));
const focusable = nodes.filter(n => n.attributes.tabindex === '0');
for (const n of focusable) {
  n.events.focus();
  assert.equal(ids.get('hover').textContent, n.attributes['aria-label']);
}
const summary = {
  scene: process.argv[2], sliders: sliders.length, depth_changes: changes,
  svg_elements: nodes.length, focus_checks: focusable.length,
  finite_coordinates: nodes.every(n => Object.entries(n.attributes).every(([k,v]) =>
    !['x','y','cx','cy','x1','x2','y1','y2'].includes(k) || Number.isFinite(Number(v)))),
  text: Object.fromEntries([...ids].map(([key,e]) => [key, e.textContent])),
  elements: nodes.map(n => ({tag:n.tag, ...n.attributes, text:n.text || ''}))
};
assert(summary.finite_coordinates);
fs.writeFileSync(process.argv[3], JSON.stringify(summary, null, 2) + '\n');
console.log(JSON.stringify({...summary, text: undefined, elements: undefined}));
