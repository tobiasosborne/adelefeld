"""Show the inert NOT_UNIQUE block in the author's vector test."""

from pathlib import Path


source = Path('tests/test_resid_rest.c').read_text()
anchor = '/* the status of adf_resid_reconstruct, which is what the checker is asked about */'
start = source.index('if (st == ADF_NOT_UNIQUE)', source.index(anchor))
brace = source.index('{', start)
depth = 0
end = None
for pos in range(brace, len(source)):
    if source[pos] == '{':
        depth += 1
    elif source[pos] == '}':
        depth -= 1
        if depth == 0:
            end = pos + 1
            break
assert end is not None
block = source[start:end]
print('line', source.count('\n', 0, start) + 1)
print('ADF_CHECK calls', block.count('ADF_CHECK'))
print('adf_resid calls', block.count('adf_resid_'))
print('lines', block.count('\n') + 1)
