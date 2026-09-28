# reconstruct_probe.cmd: reconstruction through the driver, hostile sizes (review surface)
# build/adf docs/reviews/m1/surface/checks/reconstruct_probe.cmd
reconstruct (0 +/- 1e100000 ; 1/3 mod 1)
reconstruct (0 +/- 1e-100000 ; 0 mod 1)
reconstruct (* ; 1/3 mod 1) with 0 with 1
reconstruct (* ; 1/3 mod 1) with -1/2 with 1/2
reconstruct (* ; 7 mod 100000000000000000000000000000000000000000000000000) with 0 with 10
reconstruct (* ; 0 mod 1/1000000000000000000000000000000) with 1/7 with 1/7
reconstruct (* ; 5 mod 1/2) with 1/3 with 1/3
reconstruct (0.333333333333333333333333 +/- 1e-20 ; 0 mod 1/3)
reconstruct (* ; 1/3 mod 1) with 1/3 with 1/3
reconstruct (* ; 1 mod 2) with -1000000000000000000000000000000000000 with 1000000000000000000000000000000000000
