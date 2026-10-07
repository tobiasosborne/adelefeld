#!exit 1
# F1: refine (2,1) by holes and (1,2) by repetition to (2,2), then add each cell.
ffun_add ffun(D=2, M=1; (1) + (0)*i, (2) + (0)*i) with ffun(D=1, M=2; (3) + (0)*i, (4) + (0)*i)
ffun_add ffun(D=1, M=1; (0) + (0)*i) with ffun(D=1, M=1; (0.5 +/- 0.25) + (-1)*i)
ffun_add ffun(D=1, M=1; (0) + (0)*i) with 1
ffun_add ffun(D=1, M=1; (0) + (0)*i)
