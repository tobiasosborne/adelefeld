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

// Repair ux-atlas-r1. Item 1 (D1): the prime limit follows from the tree geometry.
const atlas = require('./atlas.js');
const primesUpTo = n => Array.from({ length: n - 1 }, (_, i) => i + 2)
  .filter(p => Array.from({ length: p - 2 }, (_, i) => i + 2).every(d => p % d));
function assertRowsApart(nodes, label) {
  const { litRadius, unlitRadius } = atlas.GEOMETRY;
  const rows = new Map();
  for (const node of nodes) rows.set(node.depth, [...(rows.get(node.depth) || []), node]);
  for (const row of rows.values()) {
    for (let i = 1; i < row.length; i++) {
      const r = n => n.lit ? litRadius : unlitRadius;
      assert.ok(row[i].x - row[i - 1].x >= r(row[i]) + r(row[i - 1]) - 1e-9,
        `${label}: depth ${row[i].depth} centres ${row[i - 1].x}, ${row[i].x}`);
    }
  }
}

test('item 1: the limits are computed from the tree geometry', () => {
  const g = atlas.GEOMETRY;
  assert.equal(atlas.rowCapacity(), Math.floor(g.treeWidth / (2 * g.litRadius)) - 1);
  assert.equal(atlas.rowCapacity(), 27);
  assert.equal(atlas.MAX_PRIME, 13);
  assert.ok(2 * atlas.MAX_PRIME <= atlas.rowCapacity() && 2 * 17 > atlas.rowCapacity());
});

test('item 1: primes 2 to the limit, depth 1, all siblings lit: centre distance >= 2 r', () => {
  const primes = primesUpTo(atlas.MAX_PRIME);
  assert.deepEqual(primes, [2, 3, 5, 7, 11, 13]);
  for (const p of primes) {
    const all = Array.from({ length: p }, (_, d) => [d]);
    const nodes = atlas.layoutPaths(all, p, atlas.GEOMETRY.treeWidth, atlas.GEOMETRY.treeHeight);
    const row = nodes.filter(n => n.depth === 1);
    assert.equal(row.length, p);
    for (let i = 1; i < row.length; i++) {
      assert.ok(row[i].x - row[i - 1].x >= 2 * atlas.GEOMETRY.litRadius, `p=${p}`);
    }
    const two = atlas.layoutPaths([[0, 0], [p - 1, 0]], p, atlas.GEOMETRY.treeWidth, atlas.GEOMETRY.treeHeight);
    assert.equal(two.filter(n => n.depth === 2).length, 2 * p);
    assertRowsApart(two, `p=${p}, two branches`);
  }
});

function tinyScene(prime, paths) {
  const result = { status: 'OK', paths: paths.map(digits => ({ digits })), node_balls: {} };
  return { schema: 'adelefeld-place-atlas-1', depth_range: [1, 1], ruler: { status: 'OK' }, threads: [],
    provenance: {}, fog: {}, trees: [{ prime, depths: { '1': result } }] };
}

test('item 1: the viewer refuses a prime above the limit and a row above the capacity', () => {
  assert.equal(validateScene(tinyScene(13, [[0], [1]])).trees[0].prime, 13);
  assert.throws(() => validateScene(tinyScene(17, [[0]])), /17.*13|13.*17/);
  assert.throws(() => validateScene(tinyScene(101, [[0]])), /101/);
  assert.throws(() => validateScene(tinyScene(13, [[0, 0], [1, 0], [2, 0]])), /39.*27|27.*39/);
  const nine = [0, 1, 2].flatMap(a => [0, 1, 2].map(b => [a, b, 0]));
  assert.equal(Math.max(...atlas.rowSizes(nine, 3)), 27);
  assert.equal(validateScene(tinyScene(3, nine)).trees[0].prime, 3);
  assertRowsApart(atlas.layoutPaths(nine, 3, atlas.GEOMETRY.treeWidth, atlas.GEOMETRY.treeHeight), 'row of 27');
});

for (const name of ['sqrt9', 'idempotents30']) {
  test(name + ': item 1, no two circles of one row overlap at any stored depth', () => {
    const scene = JSON.parse(readFileSync(new URL('./scenes/' + name + '.json', import.meta.url)));
    for (const tree of scene.trees) for (const [depth, result] of Object.entries(tree.depths)) {
      const nodes = atlas.layoutPaths(result.paths.map(p => p.digits), tree.prime,
        atlas.GEOMETRY.treeWidth, atlas.GEOMETRY.treeHeight);
      assertRowsApart(nodes, `${name} p=${tree.prime} depth ${depth}`);
    }
  });
}

// Item 2 (U4): the solved polynomial beside the lens.
test('item 2: the lens title and the map label name the polynomial actually solved', () => {
  const load = name => JSON.parse(readFileSync(new URL('./scenes/' + name + '.json', import.meta.url)));
  const idem = load('idempotents30'), sqrt9 = load('sqrt9');
  assert.equal(atlas.sceneTitle(idem), 'roots 2 · X^2 - X = 0');
  assert.equal(atlas.sceneTitle(sqrt9), 'roots 2 · X^2 - 9 = 0');
  assert.match(atlas.mapLabel(idem), /^Roots of X\^2 - X = 0 at real, 2, 3, 5; input \(\* ; 0 mod 30\)/);
  assert.match(atlas.mapLabel(sqrt9), /^Roots of X\^2 - 9 = 0 at real, 2, 3, 5, 7; input 9/);
  assert.doesNotMatch(atlas.mapLabel(idem), /roots 2 of/);
  const exp = { lens: 'exp', value: '(* ; 5 mod 25)', places: ['real', 2], solved_polynomial: null };
  assert.equal(atlas.sceneTitle(exp), 'exp');
  assert.equal(atlas.mapLabel(exp), 'exp of (* ; 5 mod 25) at real, 2');
  const ball = { lens: 'roots 2', value: '(* ; 2 mod 7)', places: ['real', 7], solved_polynomial: null };
  assert.equal(atlas.sceneTitle(ball), 'roots 2 · X^2 = x');
  assert.equal(atlas.mapLabel(ball), 'Roots of X^2 = x at real, 7; input x = (* ; 2 mod 7)');
});

// Item 3 (U3): a gate caused by a missing coordinate says so; the raw status stays.
test('item 3: gate lines carry the reason and keep the failing place not given', () => {
  const missing = { status: 'DOMAIN', failing_place: 'not given', reason: 'no real coordinate in the input' };
  assert.deepEqual(atlas.gateLines(missing),
    ['no real coordinate in the input', 'Reported failing place: not given']);
  assert.deepEqual(atlas.gateLines({ status: 'DOMAIN', failing_place: 'not given' }),
    ['Reported failing place: not given']);
});

// Item 4 (U2): levels above Z_p for a negative valuation.
test('item 4: 1/5 at 5 has its root one level above the Z_5 baseline, with an exponent scale', () => {
  const tree = atlas.layoutTree([[1, 0, 0, 0, 0, 0]], 5, -1, 280, 350);
  const root = tree.nodes.find(n => n.key === '');
  assert.equal(root.exponent, -1);
  assert.deepEqual(tree.levels.map(l => l.label), ['5^-1', '5^0', '5^1', '5^2', '5^3', '5^4', '5^5']);
  assert.equal(tree.baseline.label, 'Z_5');
  const step = 350 / 6;
  assert.ok(Math.abs(tree.baseline.y - (root.y + step)) < 1e-9);
  assert.ok(root.y < tree.baseline.y);
  assert.equal(tree.nodes.filter(n => n.y < tree.baseline.y - 1e-9).length, 1);
  const first = tree.nodes.find(n => n.key === '1');
  assert.equal(first.exponent, 0);
  assert.ok(Math.abs(first.y - tree.baseline.y) < 1e-9);
  const integral = atlas.layoutTree([[1, 0]], 5, 0, 280, 350);
  assert.equal(integral.baseline.y, integral.nodes[0].y);
  assert.deepEqual(integral.levels.map(l => l.label), ['5^0', '5^1', '5^2']);
  const aligned = atlas.layoutTree([[1, 0]], 5, 0, 280, 350, { topExponent: -1, bottomExponent: 2 });
  assert.ok(Math.abs(aligned.baseline.y - 350 / 3) < 1e-9);
});
