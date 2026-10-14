from checks import *
import resource
cases=[(1,1,2097152,128),(1021,2,53,128),(1,1,2097152,2097152),(1021,2,2097152,2097152),(1,1,20,128)]
for q,n,bits,prec in cases:
 s=mp.mpc(2**130,2**-40) if bits==20 else mp.mpc('.5',2**-40)
 start=time.monotonic();p=subprocess.run(['timeout','60',str(ROOT/'runner')],input=row(0,4,q,n,0,s,bits=bits,prec=prec),text=True,capture_output=True)
 print(json.dumps({'q':q,'n':n,'s':str(s),'bits':bits,'prec':prec,'exit':p.returncode,'seconds':time.monotonic()-start,'stdout':p.stdout.strip(),'stderr':p.stderr.strip()}),flush=True)
# One CPU. No other long C run is intentionally started during this timing probe.
before=resource.getrusage(resource.RUSAGE_CHILDREN);start=time.monotonic()
p=subprocess.run(['taskset','-c','0','timeout','60',str(ROOT/'runner')],input=row(0,4,1,1,0,mp.mpc('.5',2**-40),bits=53,prec=2097152),text=True,capture_output=True)
after=resource.getrusage(resource.RUSAGE_CHILDREN)
print(json.dumps({'q':1,'n':1,'s':'1/2 + 2^-40 i','bits':53,'prec':2097152,'exit':p.returncode,'seconds':time.monotonic()-start,'user_cpu':after.ru_utime-before.ru_utime,'sys_cpu':after.ru_stime-before.ru_stime,'stdout':p.stdout.strip(),'stderr':p.stderr.strip()}),flush=True)
