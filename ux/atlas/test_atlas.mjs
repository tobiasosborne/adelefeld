import test from 'node:test';
import assert from 'node:assert/strict';
import { createRequire } from 'node:module';
import { readFileSync } from 'node:fs';
const require = createRequire(import.meta.url);
const { chooseNodes, layoutPaths, statusClass, selectDepth, validateScene } = require('./atlas.js');

test('only the lit parents grow p children', () => {
  const nodes = chooseNodes([[1, 0, 1]], 2);
  assert.deepEqual(nodes.map(n => n.key), ['', '0', '1', '1.0', '1.1', '1.0.0', '1.0.1']);
  assert.equal(nodes.filter(n => n.lit).length, 4);
  assert.ok(!nodes.some(n => n.key === '0.0'));
});

test('two branches share their prefixes', () => {
  const nodes = chooseNodes([[1, 0], [1, 1]], 2);
  assert.equal(nodes.length, 5);
  assert.equal(nodes.find(n => n.key === '1').branches.length, 2);
});

test('siblings stop above depth six', () => {
  const nodes = chooseNodes([Array(8).fill(0)], 7);
  assert.equal(nodes.filter(n => n.depth === 6).length, 7);
  assert.equal(nodes.filter(n => n.depth === 7).length, 1);
  assert.equal(nodes.filter(n => n.depth === 8).length, 1);
  assert.equal(nodes.length, 45);
});

test('layout puts root at the top, keeps row order, and supplies parent edges', () => {
  const nodes = layoutPaths([[0, 1], [2, 0]], 3, 300, 200);
  assert.equal(nodes[0].x, 150);
  assert.equal(nodes[0].y, 0);
  for (const node of nodes.slice(1)) {
    assert.ok(node.x > 0 && node.x < 300);
    const parent = nodes.find(n => n.key === node.parent);
    assert.ok(parent.y < node.y);
  }
  const row = nodes.filter(n => n.depth === 2);
  assert.deepEqual(row.map(n => n.x), [...row.map(n => n.x)].sort((a, b) => a - b));
});

test('every named status belongs to the shared shape language', () => {
  assert.equal(statusClass('OK'), 'ok');
  for (const name of ['NOT_DETERMINED', 'UNIT_NOT_CERTIFIED', 'NEEDS_SPLIT', 'NOT_UNIQUE']) {
    assert.equal(statusClass(name), 'open');
  }
  for (const name of ['DOMAIN', 'NO_SOLUTION', 'NOT_UNIT']) assert.equal(statusClass(name), 'proved no');
  assert.equal(statusClass('LIMIT'), 'budget');
  for (const name of ['PARSE', 'UNSUPPORTED']) assert.equal(statusClass(name), 'not mathematics');
});

test('a depth handle reads the exact precomputed object', () => {
  const entry = { status: 'OK', paths: [] };
  assert.equal(selectDepth({ depths: { '2': entry } }, 2), entry);
  assert.throws(() => selectDepth({ depths: {} }, 3), /precomputed/);
});

test('scene validation rejects missing depths and too many trees', () => {
  assert.throws(() => validateScene({ schema: 'other' }), /schema/);
  assert.throws(() => validateScene({ schema: 'adelefeld-place-atlas-1', trees: Array(5).fill({}) }), /four/);
});

for (const name of ['sqrt9', 'idempotents30']) {
  test(name + ': every displayed sibling has builder-supplied ball text', () => {
    const scene = JSON.parse(readFileSync(new URL('./scenes/' + name + '.json', import.meta.url)));
    assert.equal(validateScene(scene), scene);
    for (const tree of scene.trees) {
      for (const result of Object.values(tree.depths)) {
        const nodes = layoutPaths(result.paths.map(p => p.digits), tree.prime, 280, 350);
        for (const node of nodes) assert.match(result.node_balls[node.key], /^\[p=\d+: .+ \+ O\(.+\)\]$/);
        assert.ok(nodes.length <= 1 + result.paths.length * tree.prime * 5);
      }
    }
  });
}
