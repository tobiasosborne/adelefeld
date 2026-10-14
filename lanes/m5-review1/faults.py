"""Twelve source faults, planted only in lane scratch copies; baseline archive is never changed."""
import subprocess,sys,time,json,shutil,os
from pathlib import Path
ROOT=Path(__file__).resolve().parent;REPO=ROOT.parent.parent
TESTS=['test_tate_local','test_tate_gamma','test_tate','test_tate_continue','test_tate_fe']
FL=REPO/'src/localfactor.c';TT=REPO/'src/tate.c';DR=REPO/'tools/adf/adf.c'
FAULTS=[
('negative_kernel','localfactor.c','fmpq_add(theta, theta, add);','fmpq_sub(theta, theta, add);'),
('inverse_character','localfactor.c','fmpq_add(theta, theta, add);\n        fmpq_neg(theta, theta);','fmpq_sub(theta, theta, add);\n        /* eta0 instead of its inverse */'),
('integral_poles_for_gamma','localfactor.c','if (odd != e && arf_cmp_si(x, 1 + e) >= 0) return ADF_DOMAIN;','if (odd == e && arf_cmp_si(x, -e) <= 0) return ADF_DOMAIN;'),
('W_conjugated','tate.c','acb_mul(bn + n, t, g->f + n % C, w);','acb_mul(bn + n, t, g->f + n % C, w);\n        { acb_t W,ratio; acb_init(W); acb_init(ratio); adf_char_root_number(W,chi,w); acb_conj(ratio,W); acb_div(ratio,ratio,W,w); acb_mul(bn+n,bn+n,ratio,w); acb_clear(W); acb_clear(ratio); }'),
('dual_z_for_zprime','tate.c','acb_add_si(zd, zd, 1 + e, w);','acb_set(zd,zz); acb_mul_2exp_si(zd,zd,1);'),
('wrong_pole_sign','tate.c','acb_sub(lam, lam, t, w);','acb_add(lam, lam, t, w);'),
('tail_without_two','tate.c','arb_mul_2exp_si(J, J, 1);','/* omit L14 factor 2 */'),
('T3_remainder_dropped','tate.c','arb_add(En, En, el, TT_TP);\n    arb_add(En, En, er, TT_TP);','/* omit both T3 remainders */'),
('conductor_twice','tate.c','acb_mul(lam, lam, cz, w);','{ acb_mul(lam, lam, cz, w); acb_mul(lam,lam,cz,w); }'),
('width_before_error','tate.c','acb_add_error_arf(lam, lo);','acb_set(y,lam); if(C>1 && !lmode) acb_mul(y,y,cz,w);\n    tt_pre_narrow = acb_is_finite(y) && mag_cmp_2exp_si(arb_radref(acb_realref(y)),-bits-1)<=0 && mag_cmp_2exp_si(arb_radref(acb_imagref(y)),-bits-1)<=0;\n    acb_add_error_arf(lam,lo);'),
('Gamma_division','localfactor.c','rgamma_shifted(den, den, w);','acb_gamma(den,den,w); acb_inv(den,den,w);'),
('FE_side_copied','adf.c','acb_mul(R, W, B, wp);','acb_set(R,A);'),
('known_L14_branch_survivor','tate.c','arb_mul(u, bb, R0, TT_TP);','arb_sub(u, R0, RR, TT_TP);'),
('ramified_integral_zero','localfactor.c','if (eta0->q > 1)\n        {\n            acb_one(t);','if (eta0->q > 1)\n        {\n            acb_zero(t);')]
def cmd(args,timeout=160):
    st=time.monotonic();p=subprocess.run(['timeout',str(timeout),*map(str,args)],cwd=REPO,text=True,capture_output=True)
    return p.returncode,time.monotonic()-st,p.stdout,p.stderr

def one(index):
    name,file,old,new=FAULTS[index];scratch=ROOT/'fault-scratch'/name;scratch.mkdir(parents=True,exist_ok=True)
    source={'localfactor.c':FL,'tate.c':TT,'adf.c':DR}[file];text=source.read_text();assert text.count(old)==1,(name,text.count(old))
    text=text.replace(old,new)
    if name=='width_before_error':
        text=text.replace('static int\ntt_attempt(', 'static int tt_pre_narrow;\nstatic int\ntt_attempt(')
        text=text.replace('st == ADF_OK && tt_narrow(res, bits)', 'st == ADF_OK && tt_pre_narrow')
    copy=scratch/file;copy.write_text(text);obj=scratch/(file+'.o')
    c=['cc','-O2','-g','-Iinclude','-Isrc','-Itests','-c',copy,'-o',obj]
    rc,elapsed,out,err=cmd(c,60);assert rc==0,err
    results={};support=sorted((ROOT/'build/support').glob('*.o'));lib=ROOT/'build/libadelefeld.a'
    for test in TESTS:
        exe=scratch/test
        # The driver fault is deliberately planted in production alone. These C tests do not call it.
        objs=[] if file=='adf.c' else [obj]
        rc,ct,out,err=cmd(['cc','-O2','-g','-Iinclude','-Itests',REPO/'tests'/f'{test}.c',*support,*objs,lib,'-lflint','-lgmp','-lm','-o',exe],60)
        assert rc==0,(test,err)
        rc,rt,out,err=cmd([exe],100)
        results[test]={'exit':rc,'seconds':rt,'last_stdout':out.splitlines()[-1:] ,'first_stderr':err.splitlines()[:2]}
    if file=='adf.c':
        exe=scratch/'adf';rc,ct,out,err=cmd(['cc',obj,lib,'-lflint','-lgmp','-lm','-o',exe],60);assert rc==0,err
        line='tate_fe char(q=1, n=1, s=(0) + (0)*i) with ((0.5) + (0)*i ; 0) with 80\n'
        p=subprocess.run(['timeout','30',str(exe)],input=line,text=True,capture_output=True,cwd=REPO)
        results['driver']={'exit':p.returncode,'line':p.stdout.strip()}
        p=subprocess.run(['timeout','30',str(ROOT/'adf')],input=line,text=True,capture_output=True,cwd=REPO)
        results['baseline_driver']={'exit':p.returncode,'line':p.stdout.strip()}
    if name=='known_L14_branch_survivor':
        probe=scratch/'certprobe.c'
        probe.write_text((ROOT/'certprobe.c').read_text().replace('../../src/tate.c',str(copy)))
        exe=scratch/'certprobe'
        rc,ct,out,err=cmd(['cc','-O2','-Iinclude','-Isrc',probe,lib,'-lflint','-lgmp','-lm','-o',exe],60)
        assert rc==0,err
        p=subprocess.run(['timeout','20',str(exe)],input='0.25 0.125 1\n',text=True,capture_output=True,cwd=REPO)
        import mpmath as mp
        mp.mp.dps=60
        truth=mp.quad(lambda t:t**mp.mpf('.25')*mp.exp(-mp.mpf('.125')*t),[1,mp.inf])
        results['bound_probe']={'exit':p.returncode,'returned_arb_dump':p.stdout.strip(),'true_integral':mp.nstr(truth,60)}
    record={'index':index,'name':name,'source':str(source.relative_to(REPO)),'compile_exit':0,'results':results}
    (ROOT/f'fault-{index}.json').write_text(json.dumps(record,indent=2)+'\n');print(json.dumps(record),flush=True)
    shutil.rmtree(scratch)
if __name__=='__main__':one(int(sys.argv[1]))
