"""Build the HEAD Log archive and compare every output line with the repaired archive."""
import subprocess
import shutil
import time
import json
from pathlib import Path

lane = Path('lanes/f-repair8')
build = lane / 'build/plain'
scratch = lane / 'build/differential'
scratch.mkdir(exist_ok=True)
commands = []


def run(args, stdout=None):
    start = time.monotonic()
    result = subprocess.run(['timeout', '170'] + args, stdout=stdout or subprocess.PIPE,
                            stderr=subprocess.PIPE, text=True)
    commands.append(dict(command=['timeout', '170'] + args, exit=result.returncode,
                         wall=round(time.monotonic() - start, 6), stderr=result.stderr))
    assert result.returncode == 0, commands[-1]
    return result


def archive(tag, source, injected=False):
    text = Path(source).read_text()
    if injected:
        old = '    adf_sball_init(s); adf_sball_init(t);'
        assert text.count(old) == 1
        text = text.replace(old, '    if (prec == 97) return ADF_NOT_DETERMINED;\n' + old)
    src = scratch / tag / 'gfunc_log.c'
    src.parent.mkdir(exist_ok=True)
    src.write_text(text)
    obj = src.with_suffix('.o')
    lib = src.parent / 'libadelefeld.a'
    exe = src.parent / 'compare'
    run(['cc', '-Iinclude', '-Isrc', '-std=c11', '-O2', '-g', '-Wall', '-Wextra', '-Werror',
         '-c', str(src), '-o', str(obj)])
    shutil.copyfile(build / 'libadelefeld.a', lib)
    run(['ar', 'r', str(lib), str(obj)])
    run(['cc', '-Iinclude', '-std=c11', '-O2', '-g', '-Wall', '-Wextra', '-Werror',
         str(lane / 'differential.c'), str(lib), '-lflint', '-lgmp', '-lm', '-o', str(exe)])
    return exe


if __name__ == '__main__':
    import sys
    injected = len(sys.argv) > 1 and sys.argv[1] == 'injected'
    phase = 'injected' if injected else 'native'
    count = 20000 if not injected else 6000
    results = []
    for tag, source in (('old', lane / 'old_gfunc_log.c'), ('new', Path('src/gfunc_log.c'))):
        exe = archive(phase + '-' + tag, source, injected)
        out = exe.parent / 'results.txt'
        with out.open('w') as f:
            rec = run([str(exe), str(count)], stdout=f)
        results.append((out.read_text().splitlines(), rec.stderr))
    differences = [(i, a, b) for i, (a, b) in enumerate(zip(results[0][0], results[1][0])) if a != b]
    assert len(results[0][0]) == len(results[1][0]) == count
    record = dict(phase=phase, requests=count, differences=len(differences),
                  first_differences=differences[:10], old=results[0][1], new=results[1][1], commands=commands)
    (lane / ('comparison-' + phase + '.json')).write_text(json.dumps(record, indent=2) + '\n')
    print(json.dumps({k: v for k, v in record.items() if k != 'commands'}))
    assert not differences, differences[:10]
