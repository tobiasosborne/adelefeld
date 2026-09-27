
bench_word:     file format elf64-x86-64


Disassembly of section .init:

Disassembly of section .plt:

Disassembly of section .plt.got:

Disassembly of section .plt.sec:

Disassembly of section .text:

0000000000002950 <chain_mul2_run>:
    c->acc = x;
}

static __attribute__((noinline))
void chain_mul2_run(void *v, unsigned long long calls)
{
    2950:	f3 0f 1e fa          	endbr64
    2954:	41 57                	push   r15
    2956:	41 56                	push   r14
    2958:	49 89 fe             	mov    r14,rdi
    295b:	41 55                	push   r13
    295d:	41 54                	push   r12
    295f:	55                   	push   rbp
    2960:	53                   	push   rbx
    2961:	48 83 ec 08          	sub    rsp,0x8
    word_ctx *c = (word_ctx *) v;
    ulong x = c->x0;
    2965:	48 8b 47 30          	mov    rax,QWORD PTR [rdi+0x30]
    const ulong b = c->b;
    2969:	48 8b 5f 28          	mov    rbx,QWORD PTR [rdi+0x28]
    const ulong mod_n = c->mod_n;
    296d:	4c 8b 67 18          	mov    r12,QWORD PTR [rdi+0x18]
    const ulong ninv = c->ninv;
    2971:	4c 8b 6f 20          	mov    r13,QWORD PTR [rdi+0x20]
    unsigned long long i;
    for (i = 0; i < calls; i++)
    2975:	48 85 f6             	test   rsi,rsi
    2978:	74 23                	je     299d <chain_mul2_run+0x4d>
    297a:	48 89 f5             	mov    rbp,rsi
    297d:	45 31 ff             	xor    r15d,r15d
{
    ulong p1, p2;

    FLINT_ASSERT(n != 0);

    umul_ppmm(p1, p2, a, b);
    2980:	48 f7 e3             	mul    rbx
    return n_ll_mod_preinv(p1, p2, n, ninv);
    2983:	4c 89 e9             	mov    rcx,r13
    umul_ppmm(p1, p2, a, b);
    2986:	48 89 d7             	mov    rdi,rdx
    return n_ll_mod_preinv(p1, p2, n, ninv);
    2989:	48 89 c6             	mov    rsi,rax
    298c:	4c 89 e2             	mov    rdx,r12
    298f:	e8 ec e9 ff ff       	call   1380 <n_ll_mod_preinv@plt>
    2994:	49 83 c7 01          	add    r15,0x1
    2998:	4c 39 fd             	cmp    rbp,r15
    299b:	75 e3                	jne    2980 <chain_mul2_run+0x30>
        x = n_mulmod2_preinv(x, b, mod_n, ninv);
    c->acc = x;
    299d:	49 89 46 38          	mov    QWORD PTR [r14+0x38],rax
}
    29a1:	48 83 c4 08          	add    rsp,0x8
    29a5:	5b                   	pop    rbx
    29a6:	5d                   	pop    rbp
    29a7:	41 5c                	pop    r12
    29a9:	41 5d                	pop    r13
    29ab:	41 5e                	pop    r14
    29ad:	41 5f                	pop    r15
    29af:	c3                   	ret

Disassembly of section .fini:
