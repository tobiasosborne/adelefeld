"""Three planted faults in private copies; run only the stored-result comparison."""
from pathlib import Path
import subprocess
lane = Path('lanes/f-slice5')
s = Path('src/lfunc.c').read_text()
changes = [
    ('term_precision', 'fmpz_mod(term, zk, Qt);',
     'fmpz_divexact_ui(Qt, Qt, p); fmpz_mod(term, zk, Qt);'),
    ('wrong_integer', 'H = K - k * vz + e;',
     'if (p == 5) { kp += 5; } H = K - k * vz + e;'),
    ('sum_precision', 'if (k % 2 == 1) fmpz_add(S, S, term); else fmpz_sub(S, S, term);',
     'if (k % 2 == 1) fmpz_add(S, S, term); else fmpz_sub(S, S, term); fmpz_mod(S, S, Q);'),
]
(lane/'stored_only.c').write_text('''#define ADF_TEST_NO_MAIN
#include "../../tests/test_lfunc.c"
int main(void) {
    adf_test_current = "stored_before_optimisation";
    adf_test_fn_stored_before_optimisation();
    printf("stored checks=%lu failures=%lu\\n",adf_test_checks,adf_test_failures);
    flint_cleanup();
    return adf_test_failures ? 1 : 0;
}
''')
for name, old, new in changes:
    assert s.count(old) == 1
    path = lane/f'fault-{name}.c'
    path.write_text(s.replace(old,new))
    binary = lane/'build'/f'fault-{name}'
    subprocess.run(['timeout','60','cc','-std=c11','-O2','-g','-Wall','-Wextra','-Werror',
        '-Iinclude','-Itests','-Isrc',str(path),str(lane/'stored_only.c'),
        str(lane/'build/libadelefeld.a'),str(lane/'build/support/jsonl.o'),
        str(lane/'build/support/golden.o'),'-lflint','-lgmp','-lm','-o',str(binary)],check=True)
    r = subprocess.run(['timeout','60',str(binary)],capture_output=True,text=True)
    (lane/f'fault-{name}.log').write_text(r.stdout+r.stderr)
    assert r.returncode == 1, (name,r.returncode)
    assert 'stored row' in r.stdout
    print(name,r.returncode,r.stdout.splitlines()[-1],flush=True)
