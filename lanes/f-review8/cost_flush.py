from pathlib import Path
p=Path('lanes/f-review8/cost.c')
s=p.read_text().replace('total[i]=now()-t0;',
                       'total[i]=now()-t0;\n        printf("completed_total trial=%d seconds=%.9f status=%d\\n",i,total[i],st); fflush(stdout);')
p.write_text(s)
