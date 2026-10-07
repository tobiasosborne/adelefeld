"""Check the lane's document constraints and required oracle entry points."""
import ast
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
paths = ('docs/api-5.md', 'proto/tate_checks.py', 'lanes/d-tate/progress.md')
for name in paths:
    lines = (ROOT/name).read_text().splitlines()
    assert all(len(line) <= 116 for line in lines), name
    print(f'{name}: lines={len(lines)} max_columns={max(map(len, lines))} overlong=0')
assert len((ROOT/'docs/api-5.md').read_text().splitlines()) <= 500
tree = ast.parse((ROOT/'proto/tate_checks.py').read_text())
names = {node.name for node in tree.body if isinstance(node, ast.FunctionDef)}
required = {'local_integral', 'local_gamma', 'local_epsilon', 'real_integral',
            'global_value', 'continuation', 'functional_equation_sides'}
assert required <= names
print('design line budget <=500; oracle AST parsed; required entry points=7/7')
