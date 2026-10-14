from certificates import *
cases=[(C,mp.mpc(zr,zi)) for C in (1,512) for zr,zi in ((.75,1.25),(-2.5,-3))]
p=subprocess.run(['timeout','60',str(ROOT/'taylorprobe')],input=''.join(f'{C} {float(z.real)} {float(z.imag)}\n' for C,z in cases),text=True,capture_output=True)
check(p.returncode==0,('taylorprobe',p.returncode,p.stderr))
for (C,z),line in zip(cases,p.stdout.splitlines()):
    head,poly,err,deg=line.split(' | ');check(int(head.split()[0])==0,('T3 status',C,z,head));degrees=list(map(int,deg.rstrip(',').split(',')))
    b=mp.pi/C;r=z.real-1;psum=mp.mpc(0);bound=mp.mpf(0)
    for k,J in enumerate(degrees):
        m=3*mp.power(2,k-1);h=mp.power(2,k-1);d=m/2
        cs=[mp.exp(-b*m)*m**(z-1)];prev=mp.mpc(0)
        for j in range(J):
            nxt=((z-1-b*m-j)*cs[-1]-b*prev)/(m*(j+1));prev=cs[-1];cs.append(nxt)
        polynomial=2*h*sum(cs[j]*h**j/(j+1) for j in range(0,J+1,2))
        truth=b**(-z)*(mp.gammainc(z,b*(m-h),mp.inf)-mp.gammainc(z,b*(m+h),mp.inf))
        B=mp.exp(-b*(m-d))*max((m-d)**r,(m+d)**r)*mp.exp(mp.pi*abs(z.imag)/2)
        remainder=2*h*B*(mp.mpf(2)/3)**(J+1)/(1-mp.mpf(2)/3)
        check(abs(truth-polynomial)<=remainder,('T3 bound below true Taylor tail',C,z,k,J,truth,polynomial,remainder))
        psum+=polynomial;bound+=remainder;counts['Taylor_panels']+=1
    check(contains(box(poly),psum),('T3 polynomial ball',C,z))
    mid,rad=dump(err);check(mid+rad>=bound,('T3 qerr below summed formula',C,z,mid+rad,bound))
    counts['Taylor_20_panel_runs']+=1
print(json.dumps({'mode':'Taylor','counts':dict(counts),'failures':repr(failures)}));raise SystemExit(bool(failures))
