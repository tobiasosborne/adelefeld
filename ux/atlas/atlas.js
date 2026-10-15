/* Shared pure geometry. Value arithmetic and hover-ball text belong to scene.py.
   This classic script works from file://; CommonJS exports let Node test it without a DOM. */
(function (root) {
  'use strict';
  const classes = {
    OK: 'ok', NOT_DETERMINED: 'open', UNIT_NOT_CERTIFIED: 'open', NEEDS_SPLIT: 'open', NOT_UNIQUE: 'open',
    DOMAIN: 'proved no', NO_SOLUTION: 'proved no', NOT_UNIT: 'proved no', LIMIT: 'budget',
    PARSE: 'not mathematics', UNSUPPORTED: 'not mathematics'
  };
  // One tree panel of the page. Every drawing size used by atlas.html is read from here.
  const GEOMETRY = Object.freeze({
    pitch: 350, panelWidth: 330, treeLeft: 42, treeTop: 66, treeWidth: 280, treeHeight: 350,
    litRadius: 5, unlitRadius: 2.2, siblingDepth: 6
  });
  // A row of n circles has centre spacing treeWidth / (n + 1). Two lit neighbours need spacing >= 2 litRadius,
  // so a row holds at most floor(treeWidth / (2 litRadius)) - 1 = 27 circles.
  function rowCapacity() {
    return Math.floor(GEOMETRY.treeWidth / (2 * GEOMETRY.litRadius)) - 1;
  }
  // Two certified branches that differ at their first digit put 2p siblings in the next row (a quadratic,
  // the idempotents). The limit is the largest prime p with 2p <= rowCapacity(): 13. Scenes with more lit
  // nodes in one row are refused by rowSizes below, in the builder and in the viewer.
  const MAX_PRIME = (() => {
    let best = 2;
    for (let p = 2; 2 * p <= rowCapacity(); p++) {
      let prime = true;
      for (let d = 2; d * d <= p; d++) if (p % d === 0) prime = false;
      if (prime) best = p;
    }
    return best;
  })();
  function statusClass(status) {
    if (!(status in classes)) throw new Error('Unknown status: ' + status);
    return classes[status];
  }
  function chooseNodes(paths, prime) {
    const map = new Map();
    const add = (digits) => {
      const key = digits.join('.');
      if (!map.has(key)) map.set(key, {
        key, digits, depth: digits.length, parent: digits.slice(0, -1).join('.'), branches: [], lit: false
      });
      return map.get(key);
    };
    add([]);
    paths.forEach((path, branch) => {
      add([]).branches.push(branch);
      for (let i = 0; i < path.length; i++) {
        const prefix = path.slice(0, i);
        const children = i < GEOMETRY.siblingDepth ? Array.from({ length: prime }, (_, d) => d) : [path[i]];
        children.forEach(d => add([...prefix, d]));
        add(path.slice(0, i + 1)).branches.push(branch);
      }
    });
    for (const node of map.values()) node.lit = node.branches.length > 0;
    return [...map.values()].sort((a, b) => {
      if (a.depth !== b.depth) return a.depth - b.depth;
      for (let i = 0; i < a.depth; i++) if (a.digits[i] !== b.digits[i]) return a.digits[i] - b.digits[i];
      return 0;
    });
  }
  function rowSizes(paths, prime) {
    const sizes = [];
    chooseNodes(paths, prime).forEach(node => { sizes[node.depth] = (sizes[node.depth] || 0) + 1; });
    return sizes;
  }
  // A node at depth d of a tree whose root has exponent s is the ball a + p^(s + d) Z_p (scene.py digit_path).
  // The y axis is the exponent: levels from topExponent (the coarsest drawn) to bottomExponent (the finest).
  // The level of exponent 0, the balls a + Z_p, is the baseline; a negative valuation puts levels above it.
  function layoutTree(paths, prime, startExponent, width, height, options = {}) {
    const nodes = chooseNodes(paths, prime);
    const maxDepth = Math.max(1, ...nodes.map(n => n.depth));
    const top = options.topExponent ?? startExponent;
    const bottom = options.bottomExponent ?? startExponent + maxDepth;
    const step = height / Math.max(1, bottom - top);
    const rows = new Map();
    nodes.forEach(node => {
      if (!rows.has(node.depth)) rows.set(node.depth, []);
      rows.get(node.depth).push(node);
    });
    for (const row of rows.values()) row.forEach((node, index) => {
      node.exponent = startExponent + node.depth;
      node.x = width * (index + 1) / (row.length + 1);
      node.y = (node.exponent - top) * step;
    });
    const levels = [];
    for (let e = top; e <= bottom; e++) levels.push({ exponent: e, y: (e - top) * step, label: `${prime}^${e}` });
    const baseline = top <= 0 && 0 <= bottom ? { y: (0 - top) * step, label: `Z_${prime}` } : null;
    return { nodes, levels, baseline, step };
  }
  function layoutPaths(paths, prime, width, height) {
    return layoutTree(paths, prime, 0, width, height).nodes;
  }
  function sceneTitle(scene) {
    if (scene.solved_polynomial) return `${scene.lens} · ${scene.solved_polynomial} = 0`;
    if (/^roots \d+$/.test(scene.lens)) return `${scene.lens} · X^${scene.lens.split(' ')[1]} = x`;
    return scene.lens;
  }
  function mapLabel(scene) {
    const places = scene.places.join(', ');
    if (scene.solved_polynomial) {
      return `Roots of ${scene.solved_polynomial} = 0 at ${places}; input ${scene.value}`;
    }
    if (/^roots \d+$/.test(scene.lens)) {
      return `Roots of X^${scene.lens.split(' ')[1]} = x at ${places}; input x = ${scene.value}`;
    }
    return `${scene.lens} of ${scene.value} at ${places}`;
  }
  function gateLines(result) {
    return [...(result.reason ? [result.reason] : []),
      'Reported failing place: ' + (result.failing_place ?? 'not given')];
  }
  function selectDepth(tree, depth) {
    if (!Object.hasOwn(tree.depths, String(depth))) throw new Error('Depth is not precomputed: ' + depth);
    return tree.depths[String(depth)];
  }
  function validateScene(scene) {
    if (!scene || scene.schema !== 'adelefeld-place-atlas-1') throw new Error('Unknown scene schema');
    if (!Array.isArray(scene.trees) || scene.trees.length < 1 || scene.trees.length > 4) {
      throw new Error('A scene needs one to four trees');
    }
    if (!Array.isArray(scene.depth_range) || scene.depth_range.length !== 2) throw new Error('Missing depths');
    const [lo, hi] = scene.depth_range;
    if (!Number.isInteger(lo) || !Number.isInteger(hi) || lo < 1 || hi > 5 || lo > hi) {
      throw new Error('Depth range must lie in 1..5');
    }
    if (!scene.ruler || !Array.isArray(scene.threads) || !scene.provenance || !scene.fog) {
      throw new Error('Missing scene components');
    }
    const primes = new Set();
    for (const tree of scene.trees) {
      if (!Number.isInteger(tree.prime) || tree.prime < 2 || primes.has(tree.prime)) {
        throw new Error('Invalid or repeated tree prime');
      }
      if (tree.prime > MAX_PRIME) {
        throw new Error(`Prime ${tree.prime} is above the drawing limit ${MAX_PRIME} of this slice`);
      }
      primes.add(tree.prime);
      for (let d = lo; d <= hi; d++) {
        const result = selectDepth(tree, d);
        statusClass(result.status);
        if (!Array.isArray(result.paths) || !result.node_balls) throw new Error('Missing paths or hover balls');
        for (const path of result.paths) {
          if (!Array.isArray(path.digits) || path.digits.length > 1000 || path.digits.some(
            digit => !Number.isInteger(digit) || digit < 0 || digit >= tree.prime
          )) throw new Error('Invalid digit path');
        }
        const widest = Math.max(...rowSizes(result.paths.map(path => path.digits), tree.prime));
        if (widest > rowCapacity()) {
          throw new Error(`A row of ${widest} circles at prime ${tree.prime}, depth ${d}, exceeds the capacity `
            + `${rowCapacity()} of a tree row`);
        }
      }
    }
    statusClass(scene.ruler.status);
    return scene;
  }
  const api = {
    GEOMETRY, MAX_PRIME, rowCapacity, rowSizes, chooseNodes, layoutTree, layoutPaths, sceneTitle, mapLabel,
    gateLines, statusClass, selectDepth, validateScene
  };
  if (typeof module !== 'undefined' && module.exports) module.exports = api;
  else root.AtlasGeometry = api;
})(typeof globalThis !== 'undefined' ? globalThis : this);
