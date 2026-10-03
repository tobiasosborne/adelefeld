from pathlib import Path
import subprocess
P=Path('lanes/f-review8')
s=Path('src/lroot.c').read_text()
s='unsigned long review_enum_calls;\n'+s
assert s.count('    fmpz_t a,h;')==1
s=s.replace('    fmpz_t a,h;','    review_enum_calls++;\n    fmpz_t a,h;')
for name in ('original','fault'):
    text=s
    if name=='fault': text=text.replace('ulong general=d-(ulong)sh->nrat, plain=0;',
                                       'return ADF_OK; ulong general=d-(ulong)sh->nrat, plain=0;')
    src=P/'build'/f'early-{name}.c'; src.write_text(text)
    cmd=['timeout','60','cc','-std=gnu11','-O2','-Iinclude','-Isrc',str(src),str(P/'early_probe.c'),
         str(P/'build/libadelefeld.a'),'-lflint','-lgmp','-lm','-o',str(P/'build'/f'early-{name}')]
    subprocess.run(cmd,check=True)
    subprocess.run(['timeout','30',str(P/'build'/f'early-{name}')],check=True)
