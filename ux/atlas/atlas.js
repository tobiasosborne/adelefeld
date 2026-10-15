/* Shared pure geometry. Value arithmetic and hover-ball text belong to scene.py.
   This classic script works from file://; CommonJS exports let Node test it without a DOM. */
(function (root) {
  'use strict';
  const classes = {
    OK: 'ok', NOT_DETERMINED: 'open', UNIT_NOT_CERTIFIED: 'open', NEEDS_SPLIT: 'open', NOT_UNIQUE: 'open',
    DOMAIN: 'proved no', NO_SOLUTION: 'proved no', NOT_UNIT: 'proved no', LIMIT: 'budget',
    PARSE: 'not mathematics', UNSUPPORTED: 'not mathematics'
  };
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
        const children = i < 6 ? Array.from({ length: prime }, (_, d) => d) : [path[i]];
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
  function layoutPaths(paths, prime, width, height) {
    const nodes = chooseNodes(paths, prime);
    const maxDepth = Math.max(1, ...nodes.map(n => n.depth));
    const rows = new Map();
    nodes.forEach(node => {
      if (!rows.has(node.depth)) rows.set(node.depth, []);
      rows.get(node.depth).push(node);
    });
    for (const [depth, row] of rows) row.forEach((node, index) => {
      node.x = width * (index + 1) / (row.length + 1);
      node.y = height * depth / maxDepth;
    });
    return nodes;
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
      if (!Number.isInteger(tree.prime) || tree.prime < 2 || tree.prime >= 1000 || primes.has(tree.prime)) {
        throw new Error('Invalid or repeated tree prime');
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
      }
    }
    statusClass(scene.ruler.status);
    return scene;
  }
  const api = { chooseNodes, layoutPaths, statusClass, selectDepth, validateScene };
  if (typeof module !== 'undefined' && module.exports) module.exports = api;
  else root.AtlasGeometry = api;
})(typeof globalThis !== 'undefined' ? globalThis : this);
