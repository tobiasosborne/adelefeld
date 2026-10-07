#!exit 1
# P4, F4. Delta_1 at (4,1) gives [1,-i,-1,i] at layout (1,4), weight 1.
ffun_fourier ffun(D=4, M=1; (0) + (0)*i, (1) + (0)*i, (0) + (0)*i, (0) + (0)*i)
# Delta_0 coefficient 3 at (2,3) has six exact transform entries 1, weight 1/3.
ffun_fourier ffun(D=2, M=3; (3) + (0)*i, (0) + (0)*i, (0) + (0)*i, (0) + (0)*i, (0) + (0)*i, (0) + (0)*i)
ffun_fourier ffun(D=1, M=1; (0) + (0)*i)
ffun_fourier 1
