// Lane ux-atlas-r1: run the real page script in a minimal DOM stand-in (pattern of
// lanes/ux-drive-atlas/check_dom.cjs) and assert what items 2 to 5 put on the page. Not a browser check.
// Usage, from the repository root: node lanes/ux-atlas-r1/page_check.cjs SCENE.json KIND
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
const walk = e => [e, ...e.children.flatMap(walk)];
const html = fs.readFileSync('ux/atlas/atlas.html', 'utf8');
const ids = new Map([...html.matchAll(/id="([^"]+)"/g)].map(m => [m[1], new Element('div')]));
const document = { title: '', getElementById: id => ids.get(id), createElement: tag => new Element(tag),
  createElementNS: (_, tag) => new Element(tag), querySelectorAll: () => [] };
const context = vm.createContext({ document, location: { protocol: 'file:', search: '' }, URLSearchParams });
context.window = context;
vm.runInContext(fs.readFileSync('ux/atlas/atlas.js', 'utf8'), context);
vm.runInContext([...html.matchAll(/<script>([\s\S]*?)<\/script>/g)].map(m => m[1]).join('\n'), context);
const [file, kind] = process.argv.slice(2);
const scene = JSON.parse(fs.readFileSync(file, 'utf8'));
context.PlaceAtlas.loadScene(scene);
const texts = walk(ids.get('map')).filter(e => e.tag === 'text').map(e => ({ t: e.text, a: e.attributes }));
const has = t => texts.some(x => x.t === t);
const lens = ids.get('lens').textContent, label = ids.get('map').attributes['aria-label'];
const out = { file, lens, title: document.title, label,
  scale: texts.filter(x => x.a.class === 'scale').map(x => x.t),
  baselines: texts.filter(x => x.a.class === 'zlab').map(x => x.t) };
let assertions = 0;
const ok = (cond, msg) => { assert.ok(cond, msg); assertions++; };
if (kind === 'idem') {
  ok(lens === 'roots 2 · X^2 - X = 0', lens);
  ok(document.title.startsWith('roots 2 · X^2 - X = 0'), document.title);
  ok(label.startsWith('Roots of X^2 - X = 0 at real, 2, 3, 5'), label);
}
if (kind === 'sqrt9') {
  ok(lens === 'roots 2 · X^2 - 9 = 0', lens);
  ok(label.startsWith('Roots of X^2 - 9 = 0'), label);
}
if (kind === 'onefifth') {
  // Trees at 2 and 5 at depth 5: one exponent axis from 5^-1 (the root of the 5-tree) down to the finest level.
  const tree5 = scene.trees.find(t => t.prime === 5).depths['5'];
  ok(tree5.paths[0].start_exponent === -1, 'start exponent');
  ok(out.scale.includes('5^-1') && out.scale.includes('5^0') && out.scale.includes('2^-1'), out.scale.join());
  ok(out.baselines.join() === 'Z_2,Z_5', out.baselines.join());
  const y = t => Number(texts.find(x => x.t === t).a.y);
  ok(y('5^-1') < y('5^0'), 'level above the baseline');
  ok(y('2^0') === y('5^0'), 'baselines aligned across trees');
  ok(tree5.node_balls[''] === '[p=5: 0 + O(5^-1)]' && tree5.node_balls['1'] === '[p=5: 1/5 + O(5^0)]', 'hover');
}
if (kind === 'exp') {
  ok(scene.ruler.reason === 'no real coordinate in the input', 'scene reason');
  ok(has('DOMAIN') && has('no real coordinate in the input') && has('Reported failing place: not given'), 'gate');
  const r = texts.findIndex(x => x.t === 'no real coordinate in the input');
  ok(texts[r + 1].t === 'Reported failing place: not given', 'order');
}
if (kind === 'idem' || kind === 'sqrt9') {
  ok(!out.scale.some(s => s.includes('^-')), 'no negative levels');
  ok(out.baselines.length === scene.trees.length, 'one baseline per tree');
}
const cards = walk(ids.get('controls')).filter(e => /^digits derived from record \d+: /.test(e.text || ''));
const paths = scene.trees.reduce((n, t) => n + t.depths[String(scene.depth_range[1])].paths.length, 0);
ok(cards.length === paths, `path cards ${cards.length} of ${paths}`);
for (const card of cards) {
  const [, index, command] = card.text.match(/^digits derived from record (\d+): (.*)$/);
  ok(scene.provenance.records[Number(index)].command === command, card.text);
}
ok(/D-UX5 as amended/.test(html.match(/<footer>([\s\S]*?)<\/footer>/)[1]), 'footer');
out.assertions = assertions;
console.log(JSON.stringify(out));
