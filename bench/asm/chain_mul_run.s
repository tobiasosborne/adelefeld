
bench_word:     file format elf64-x86-64


Disassembly of section .init:

Disassembly of section .plt:

Disassembly of section .plt.got:

Disassembly of section .plt.sec:

Disassembly of section .text:

0000000000002700 <chain_mul_run>:
    c->acc = x;
}

static __attribute__((noinline))
void chain_mul_run(void *v, unsigned long long calls)
{
    2700:	f3 0f 1e fa          	endbr64
    2704:	55                   	push   rbp
    2705:	49 89 f2             	mov    r10,rsi
    2708:	53                   	push   rbx
    2709:	48 89 fb             	mov    rbx,rdi
    word_ctx *c = (word_ctx *) v;
    ulong x = c->x0;
    270c:	48 8b 77 30          	mov    rsi,QWORD PTR [rdi+0x30]
    const ulong b = c->b;
    2710:	4c 8b 4f 28          	mov    r9,QWORD PTR [rdi+0x28]
    const nmod_t mod = c->mod;
    2714:	4c 8b 5b 08          	mov    r11,QWORD PTR [rbx+0x8]
    2718:	48 8b 3f             	mov    rdi,QWORD PTR [rdi]
    271b:	48 8b 4b 10          	mov    rcx,QWORD PTR [rbx+0x10]
    unsigned long long i;
    for (i = 0; i < calls; i++)
    271f:	4d 85 d2             	test   r10,r10
    2722:	74 55                	je     2779 <chain_mul_run+0x79>

NMOD_INLINE
mp_limb_t nmod_mul(mp_limb_t a, mp_limb_t b, nmod_t mod)
{
    mp_limb_t res;
    NMOD_MUL_PRENORM(res, a, b << mod.norm, mod);
    2724:	48 d3 e7             	shl    rdi,cl
    2727:	89 cd                	mov    ebp,ecx
    2729:	49 d3 e1             	shl    r9,cl
    272c:	45 31 c0             	xor    r8d,r8d
    272f:	90                   	nop
    2730:	48 89 f0             	mov    rax,rsi
    2733:	49 f7 e1             	mul    r9
    2736:	48 89 d1             	mov    rcx,rdx
    2739:	48 89 c6             	mov    rsi,rax
    273c:	4c 89 d8             	mov    rax,r11
    273f:	48 f7 e2             	mul    rdx
    2742:	48 01 f0             	add    rax,rsi
    2745:	48 11 ca             	adc    rdx,rcx
    2748:	48 83 c2 01          	add    rdx,0x1
    274c:	89 e9                	mov    ecx,ebp
    274e:	48 0f af d7          	imul   rdx,rdi
    2752:	48 29 d6             	sub    rsi,rdx
    2755:	48 39 f0             	cmp    rax,rsi
    2758:	48 8d 14 3e          	lea    rdx,[rsi+rdi*1]
    275c:	48 0f 42 f2          	cmovb  rsi,rdx
    2760:	48 89 f0             	mov    rax,rsi
    2763:	48 29 f8             	sub    rax,rdi
    2766:	48 39 fe             	cmp    rsi,rdi
    2769:	48 0f 43 f0          	cmovae rsi,rax
    276d:	49 83 c0 01          	add    r8,0x1
    2771:	48 d3 ee             	shr    rsi,cl
    2774:	4d 39 c2             	cmp    r10,r8
    2777:	75 b7                	jne    2730 <chain_mul_run+0x30>
        x = nmod_mul(x, b, mod);
    c->acc = x;
    2779:	48 89 73 38          	mov    QWORD PTR [rbx+0x38],rsi
}
    277d:	5b                   	pop    rbx
    277e:	5d                   	pop    rbp
    277f:	c3                   	ret

Disassembly of section .fini:
