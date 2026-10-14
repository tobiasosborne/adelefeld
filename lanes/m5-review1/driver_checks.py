from checks import *
C='char(q=1, n=1, s=(0) + (0)*i)';P='char(q=5, n=2, s=(0) + (0)*i)'
S='((2) + (0)*i ; 0)';A='((1) + (0)*i ; 0)'
malformed=[]
def add(args,status,why):malformed.append((args,'error: '+status,why))
# Each expected line is fixed here before any driver invocation.
for command in ('tate_local','tate_gamma','tate_epsilon'):
    for s,p,a,ch,expected,why in [
        ('2','2',A,C,'UNSUPPORTED','scalar where complex adele is required'),
        (S,'2','1',C,'UNSUPPORTED','scalar alpha where complex adele is required'),
        (S,'2',A,'1','UNSUPPORTED','scalar where character is required'),
        (S,'4',A,C,'DOMAIN','composite place'),
        (S,'0',A,C,'DOMAIN','zero place'),
        (S,'18446744073709551616',A,C,'DOMAIN','place does not fit a word'),
        (S,'2',A,'char(q=8, n=7, s=(0) + (0)*i)','DOMAIN','text must already be primitive'),
        (S,'2',A,'char(q=65537, n=3, s=(0) + (0)*i)','LIMIT','character modulus cap'),
        (S,'two',A,C,'PARSE','not a place token'),
        (S,'2',A,'char(q=5, n=0, s=(0) + (0)*i)','DOMAIN','nonunit character label'),
    ]:add([command,s,'with',p,'with',a,'with',ch],expected,why)
for command in ('tate_integral','tate_continue','tate_completed','tate_fe'):
    for ch,s,b,expected,why in [
        ('1',S,'53','DOMAIN','wrong character kind'),
        (C,'2','53','DOMAIN','wrong spectral kind'),
        (C,S,A,'DOMAIN','wrong bits kind'),
        (C,S,'1/2','DOMAIN','noninteger bits'),
        (C,S,'18446744073709551616','DOMAIN','bits do not fit slong'),
        ('char(q=8, n=7, s=(0) + (0)*i)',S,'53','DOMAIN','imprimitive character text'),
        ('char(q=65537, n=3, s=(0) + (0)*i)',S,'53','LIMIT','character cap'),
    ]:add([command,ch,'with',s,'with',b],expected,why)
add(['tate_vector','1'],'DOMAIN','wrong character kind')
add(['tate_vector','char(q=8, n=7, s=(0) + (0)*i)'],'DOMAIN','imprimitive character text')
assert len(malformed)==60
# Value text lowers; strict dump loading rejects imprimitive storage (api-3d:70-77 versus :305-314).
# Keep the imprimitive texts as normalization tests, and use a nonunit label for these malformed slots.
normalizations=[]
for i,(args,expected,why) in enumerate(malformed):
    if any('q=8, n=7' in arg for arg in args):
        canonical=[arg.replace('q=8, n=7','q=4, n=3') for arg in args]
        normalizations.append((args,canonical))
        malformed[i]=([arg.replace('q=8, n=7','q=5, n=0') for arg in args],expected,'nonunit label')
maxwidth=0
for args,expected,why in malformed:
    p=subprocess.run(['timeout','20',str(ROOT/'adf')],input=' '.join(args)+'\n',text=True,capture_output=True)
    check(p.stdout.strip()==expected,('driver malformed',args,expected,p.stdout.strip(),why));counts['driver_malformed']+=1
for original,canonical in normalizations:
    outs=[subprocess.run(['timeout','20',str(ROOT/'adf')],input=' '.join(args)+'\n',text=True,capture_output=True) for args in (original,canonical)]
    check(all(p.returncode==0 for p in outs) and outs[0].stdout==outs[1].stdout,('driver character lowering',original))
    counts['driver_normalizations']+=1
# Printed values are checked against the independent numerical models, not a golden output file.
for q,n in [(1,1),(4,3),(5,2),(7,3),(9,2),(13,2)]:
    ch=f'char(q={q}, n={n}, s=(0) + (0)*i)'
    for command,s in [('tate_integral',mp.mpc(2,3)),('tate_continue',mp.mpc(-2,1)),('tate_completed',mp.mpc('.375','.75')),('tate_fe',mp.mpc('.375','.75'))]:
        text=f'(({s.real}) + ({s.imag})*i ; 0)'
        p=subprocess.run(['timeout','30',str(ROOT/'adf')],input=' '.join([command,ch,'with',text,'with','53'])+'\n',text=True,capture_output=True)
        check(p.returncode==0,('driver numerical status',q,n,command,p.stdout,p.stderr))
        pieces=p.stdout.strip().split(' | ');value=global_value(q,n,s)[int(command in ('tate_completed','tate_fe'))]
        for printed in (pieces[:2] if command=='tate_fe' else pieces[:1]):
            # Printed coordinates use exact decimal midpoints and explicit outward decimal radii.
            v=printed.strip();re,im=v.split(' + ');re=re.strip('() ');im=im.removesuffix('*i').strip('() ')
            def rd(t):
                vals=t.split(' +/- ');return mp.mpf(vals[0]),mp.mpf(vals[1]) if len(vals)>1 else mp.mpf(0)
            check(contains((rd(re),rd(im)),value),('driver printed enclosure',q,n,command,printed,value));counts['driver_printed_balls']+=1
        if command=='tate_fe':check('overlap=yes' in p.stdout,('driver FE overlap',q,n,p.stdout))
        counts['driver_numerical_lines']+=1
print(json.dumps({'mode':'driver','counts':dict(counts),'failures':repr(failures)}));raise SystemExit(bool(failures))
