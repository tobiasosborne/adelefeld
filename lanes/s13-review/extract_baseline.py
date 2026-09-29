"""Read one loose commit and its tree without invoking Git."""

from pathlib import Path
import sys
import zlib


OBJECTS = Path('/home/tobias/Projects/adelefeld/.git/objects')


def obj(oid):
    data = zlib.decompress((OBJECTS / oid[:2] / oid[2:]).read_bytes())
    return data.split(b'\0', 1)[1]


def child(tree, name):
    data = obj(tree)
    pos = 0
    while pos < len(data):
        end = data.index(b'\0', pos)
        mode_name = data[pos:end]
        oid = data[end + 1:end + 21].hex()
        if mode_name.split(b' ', 1)[1] == name.encode():
            return oid
        pos = end + 21
    raise KeyError(name)


commit = obj('29845ccd2cc441718f5c51011623b8c1b9c2848e')
tree = commit.split(b'\n', 1)[0].split(b' ')[1].decode()
src = child(tree, 'src')
resid = child(src, 'resid.c')
Path(sys.argv[1]).write_bytes(obj(resid))
print(f'commit tree {tree}, src/resid.c {resid}, {len(obj(resid))} bytes')
