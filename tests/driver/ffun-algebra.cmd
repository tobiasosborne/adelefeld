#!exit 1
# Hand-derived before adding driver handlers. F1 product, F2 shifts, F3 holes.
ffun_mul ffun(D=2, M=1; (1) + (0)*i, (2) + (0)*i) with ffun(D=1, M=2; (3) + (0)*i, (4) + (0)*i)
ffun_translate ffun(D=2, M=1; (0) + (0)*i, (1) + (0)*i) with 1/2
ffun_translate ffun(D=1, M=1; (1) + (0)*i) with 1/3
ffun_reflect ffun(D=4, M=1; (0) + (0)*i, (1) + (0)*i, (0) + (0)*i, (0) + (0)*i)
ffun_dilate ffun(D=2, M=1; (0) + (0)*i, (1) + (0)*i) with 2/3
ffun_dilate ffun(D=2, M=1; (0) + (0)*i, (1) + (0)*i) with -1
ffun_dilate ffun(D=1, M=1; (1) + (0)*i) with 0
ffun_dilate_idele ffun(D=1, M=3; (0) + (0)*i, (1) + (0)*i, (0) + (0)*i) with (7 ; 1 * [1 mod 1])
ffun_dilate_idele ffun(D=1, M=2; (1) + (0)*i, (2) + (0)*i) with (-7 ; 1 * [1 mod 1])
ffun_dilate_idele ffun(D=1, M=1; (1) + (0)*i) with (9 ; 1/3 * [1])
ffun_conj ffun(D=2, M=1; (1) + (2)*i, (-3) + (4)*i)
ffun_mul ffun(D=1, M=1; (1) + (0)*i) with 2
ffun_translate ffun(D=1, M=1; (1) + (0)*i) with (2 ; 2)
ffun_dilate_idele ffun(D=1, M=1; (1) + (0)*i) with 2/3
