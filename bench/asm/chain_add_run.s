
bench_word:     file format elf64-x86-64


Disassembly of section .init:

Disassembly of section .plt:

Disassembly of section .plt.got:

Disassembly of section .plt.sec:

Disassembly of section .text:

00000000000026b0 <chain_add_run>:
   own result back as the next left operand; the dependency is visible in
   bench/asm (see bench/README.md). */

static __attribute__((noinline))
void chain_add_run(void *v, unsigned long long calls)
{
    26b0:	f3 0f 1e fa          	endbr64
    word_ctx *c = (word_ctx *) v;
    ulong x = c->x0;
    26b4:	48 8b 47 30          	mov    rax,QWORD PTR [rdi+0x30]
    const ulong b = c->b;
    26b8:	48 8b 4f 28          	mov    rcx,QWORD PTR [rdi+0x28]
    const nmod_t mod = c->mod;
    26bc:	4c 8b 17             	mov    r10,QWORD PTR [rdi]
    unsigned long long i;
    for (i = 0; i < calls; i++)
    26bf:	48 85 f6             	test   rsi,rsi
    26c2:	74 29                	je     26ed <chain_add_run+0x3d>
NMOD_INLINE
mp_limb_t nmod_add(mp_limb_t a, mp_limb_t b, nmod_t mod)
{
   const mp_limb_t neg = mod.n - a;
   if (neg > b)
      return a + b;
    26c4:	49 89 cb             	mov    r11,rcx
    26c7:	31 d2                	xor    edx,edx
    26c9:	4d 29 d3             	sub    r11,r10
    26cc:	0f 1f 40 00          	nop    DWORD PTR [rax+0x0]
   const mp_limb_t neg = mod.n - a;
    26d0:	4d 89 d1             	mov    r9,r10
      return a + b;
    26d3:	4c 8d 04 08          	lea    r8,[rax+rcx*1]
   const mp_limb_t neg = mod.n - a;
    26d7:	49 29 c1             	sub    r9,rax
      return a + b;
    26da:	4c 01 d8             	add    rax,r11
    26dd:	4c 39 c9             	cmp    rcx,r9
    26e0:	49 0f 42 c0          	cmovb  rax,r8
    26e4:	48 83 c2 01          	add    rdx,0x1
    26e8:	48 39 d6             	cmp    rsi,rdx
    26eb:	75 e3                	jne    26d0 <chain_add_run+0x20>
        x = nmod_add(x, b, mod);
    c->acc = x;
    26ed:	48 89 47 38          	mov    QWORD PTR [rdi+0x38],rax
}
    26f1:	c3                   	ret

Disassembly of section .fini:
