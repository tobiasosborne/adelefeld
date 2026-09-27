	.file	"baseline.c"
	.text
	.section	.rodata.str1.1,"aMS",@progbits,1
.LC4:
	.string	"nmod_add chain (62-bit)"
	.section	.rodata.str1.8,"aMS",@progbits,1
	.align 8
.LC5:
	.string	"%-34s min %9.2f ns  median %9.2f ns  max %9.2f ns\n"
	.section	.rodata.str1.1
.LC6:
	.string	"nmod_mul chain (62-bit)"
.LC8:
	.string	"n_gcd (62-bit, fresh inputs)"
.LC10:
	.string	"fmpz_add 4096 bit"
.LC12:
	.string	"fmpz_mul 4096 x 4096 bit"
.LC13:
	.string	"fmpz_mod 8192 by 4096 bit"
.LC15:
	.string	"fmpz_gcd 4096 bit"
.LC17:
	.string	"arb_add 128 bit"
.LC18:
	.string	"arb_mul 128 bit"
.LC19:
	.string	"checksum %lu\n"
.LC20:
	.string	"3.0.1"
	.section	.rodata.str1.8
	.align 8
.LC21:
	.string	"FLINT %s, sizeof: fmpz %wu, arb %wu, acb %wu, nmod_t %wu\n"
	.section	.text.startup,"ax",@progbits
	.p2align 4
	.globl	main
	.type	main, @function
main:
.LFB434:
	.cfi_startproc
	endbr64
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	subq	$472, %rsp
	.cfi_def_cfa_offset 528
	movdqa	.LC0(%rip), %xmm0
	movq	%fs:40, %rdx
	movq	%rdx, 456(%rsp)
	xorl	%edx, %edx
	movl	$0, 304(%rsp)
	movups	%xmm0, 312(%rsp)
	cmpl	$1, %edi
	jle	.L2
	movq	8(%rsi), %rdi
	movl	$10, %edx
	xorl	%esi, %esi
	call	strtoul@PLT
.L2:
	movabsq	$4611686018427387847, %r15
	leaq	272(%rsp), %rbx
	leaq	64(%rsp), %r12
	movq	%r15, %rdi
	call	n_preinvert_limb@PLT
	movq	%rbx, %rdi
	movq	%r15, %rsi
	movq	%rax, %r14
	call	n_randint@PLT
	movq	%rbx, %rdi
	movq	%r15, %rsi
	movq	%rax, 16(%rsp)
	call	n_randint@PLT
	movq	%r12, 8(%rsp)
	movq	16(%rsp), %r8
	orq	$1, %rax
	movq	%rbx, 40(%rsp)
	movq	%rax, %r13
	leaq	336(%rsp), %rax
	movq	%rax, 32(%rsp)
	movq	%rax, %rbp
	leaq	456(%rsp), %rax
	movq	%rax, (%rsp)
	movq	%rax, %rbx
	.p2align 4,,10
	.p2align 3
.L6:
	movl	$1, %edi
	movq	%r12, %rsi
	movq	%r8, 24(%rsp)
	call	clock_gettime@PLT
	pxor	%xmm0, %xmm0
	pxor	%xmm1, %xmm1
	movq	%r13, %rdi
	cvtsi2sdq	72(%rsp), %xmm0
	mulsd	.LC1(%rip), %xmm0
	movq	24(%rsp), %r8
	subq	%r15, %rdi
	cvtsi2sdq	64(%rsp), %xmm1
	movl	$20000000, %eax
	addsd	%xmm1, %xmm0
	movsd	%xmm0, 16(%rsp)
	.p2align 4,,10
	.p2align 3
.L5:
	movq	%r15, %rsi
	leaq	(%r8,%r13), %rdx
	subq	%r8, %rsi
	addq	%rdi, %r8
	cmpq	%rsi, %r13
	cmovb	%rdx, %r8
	subq	$1, %rax
	jne	.L5
	movq	%r12, %rsi
	movl	$1, %edi
	movq	%r8, 24(%rsp)
	addq	$8, %rbp
	call	clock_gettime@PLT
	pxor	%xmm0, %xmm0
	pxor	%xmm1, %xmm1
	movq	24(%rsp), %r8
	cvtsi2sdq	72(%rsp), %xmm0
	mulsd	.LC1(%rip), %xmm0
	cvtsi2sdq	64(%rsp), %xmm1
	addsd	%xmm1, %xmm0
	subsd	16(%rsp), %xmm0
	divsd	.LC2(%rip), %xmm0
	mulsd	.LC3(%rip), %xmm0
	movsd	%xmm0, -8(%rbp)
	cmpq	%rbx, %rbp
	jne	.L6
	leaq	344(%rsp), %rax
	movq	40(%rsp), %rbx
	movl	$1, %esi
	movq	%rax, 40(%rsp)
	movq	%rax, %rdi
.L10:
	movsd	(%rdi), %xmm1
	movq	%rdi, %rax
	movl	%esi, %edx
	jmp	.L7
	.p2align 4,,10
	.p2align 3
.L9:
	movsd	%xmm0, (%rax)
	subq	$8, %rax
	testl	%edx, %edx
	je	.L64
.L7:
	movsd	-8(%rax), %xmm0
	movslq	%edx, %rcx
	leal	-1(%rcx), %edx
	comisd	%xmm1, %xmm0
	ja	.L9
.L8:
	movsd	%xmm1, 336(%rsp,%rcx,8)
	addl	$1, %esi
	addq	$8, %rdi
	cmpl	$15, %esi
	jne	.L10
	leaq	.LC4(%rip), %rdx
	movl	$2, %edi
	salq	$2, %r13
	movsd	448(%rsp), %xmm2
	movsd	392(%rsp), %xmm1
	movl	$3, %eax
	movq	%r8, 16(%rsp)
	leaq	.LC5(%rip), %rsi
	movsd	336(%rsp), %xmm0
	call	__printf_chk@PLT
	movq	16(%rsp), %r8
	movq	%rbx, %rax
	movq	(%rsp), %r15
	movq	32(%rsp), %rbp
	movq	8(%rsp), %r12
	movq	%r14, %rbx
	movq	%r13, %r14
	movq	%r8, 48(%rsp)
	movq	%r8, %rcx
	movq	%rax, %r13
	.p2align 4,,10
	.p2align 3
.L14:
	movq	%r12, %rsi
	movl	$1, %edi
	movq	%rcx, 24(%rsp)
	call	clock_gettime@PLT
	pxor	%xmm0, %xmm0
	pxor	%xmm1, %xmm1
	movq	24(%rsp), %rcx
	cvtsi2sdq	72(%rsp), %xmm0
	mulsd	.LC1(%rip), %xmm0
	movl	$20000000, %esi
	cvtsi2sdq	64(%rsp), %xmm1
	addsd	%xmm1, %xmm0
	movsd	%xmm0, 16(%rsp)
	.p2align 4,,10
	.p2align 3
.L13:
	movq	%rcx, %rax
#APP
# 164 "/usr/include/flint/nmod.h" 1
	mulq %r14
# 0 "" 2
#NO_APP
	movq	%rdx, %rdi
	movq	%rax, %rcx
	movq	%rbx, %rax
#APP
# 164 "/usr/include/flint/nmod.h" 1
	mulq %rdx
# 0 "" 2
# 164 "/usr/include/flint/nmod.h" 1
	addq %rcx,%rax
	adcq %rdi,%rdx
# 0 "" 2
#NO_APP
	addq	$1, %rdx
	imulq	$-228, %rdx, %rdx
	subq	%rdx, %rcx
	cmpq	%rcx, %rax
	leaq	-228(%rcx), %rdx
	cmovb	%rdx, %rcx
	cmpq	$-229, %rcx
	leaq	228(%rcx), %rax
	cmova	%rax, %rcx
	shrq	$2, %rcx
	subq	$1, %rsi
	jne	.L13
	movq	%r12, %rsi
	movl	$1, %edi
	movq	%rcx, 24(%rsp)
	addq	$8, %rbp
	call	clock_gettime@PLT
	pxor	%xmm0, %xmm0
	pxor	%xmm1, %xmm1
	movq	24(%rsp), %rcx
	cvtsi2sdq	72(%rsp), %xmm0
	mulsd	.LC1(%rip), %xmm0
	cvtsi2sdq	64(%rsp), %xmm1
	addsd	%xmm1, %xmm0
	subsd	16(%rsp), %xmm0
	divsd	.LC2(%rip), %xmm0
	mulsd	.LC3(%rip), %xmm0
	movsd	%xmm0, -8(%rbp)
	cmpq	%r15, %rbp
	jne	.L14
	movq	48(%rsp), %r8
	movq	40(%rsp), %r9
	movq	%r13, %rbx
	movl	$1, %edi
.L18:
	movsd	(%r9), %xmm1
	movq	%r9, %rax
	movl	%edi, %edx
	jmp	.L15
	.p2align 4,,10
	.p2align 3
.L17:
	movsd	%xmm0, (%rax)
	subq	$8, %rax
	testl	%edx, %edx
	je	.L65
.L15:
	movsd	-8(%rax), %xmm0
	movslq	%edx, %rsi
	leal	-1(%rsi), %edx
	comisd	%xmm1, %xmm0
	ja	.L17
.L16:
	movsd	%xmm1, 336(%rsp,%rsi,8)
	addl	$1, %edi
	addq	$8, %r9
	cmpl	$15, %edi
	jne	.L18
	movsd	448(%rsp), %xmm2
	leaq	.LC6(%rip), %rdx
	movsd	392(%rsp), %xmm1
	leaq	.LC5(%rip), %rsi
	movl	$2, %edi
	movl	$3, %eax
	movq	%r8, 24(%rsp)
	movsd	336(%rsp), %xmm0
	movq	%rcx, 16(%rsp)
	call	__printf_chk@PLT
	movq	16(%rsp), %rcx
	movq	24(%rsp), %r8
	movq	%rbx, %rdi
	movl	$62, %esi
	addq	%rcx, %r8
	movq	%r8, 24(%rsp)
	call	n_randbits@PLT
	movq	%rbx, %rdi
	movl	$62, %esi
	orq	$1, %rax
	movq	%rax, %r13
	call	n_randbits@PLT
	movq	%rbx, 48(%rsp)
	leaq	6000000(%rax), %rbp
	movq	32(%rsp), %rax
	movq	%rbp, %rbx
	movq	24(%rsp), %rbp
	movq	%rax, 16(%rsp)
	.p2align 4,,10
	.p2align 3
.L22:
	movq	8(%rsp), %rsi
	movl	$1, %edi
	leaq	-6000000(%rbx), %r12
	call	clock_gettime@PLT
	pxor	%xmm0, %xmm0
	pxor	%xmm1, %xmm1
	cvtsi2sdq	72(%rsp), %xmm0
	mulsd	.LC1(%rip), %xmm0
	cvtsi2sdq	64(%rsp), %xmm1
	addsd	%xmm1, %xmm0
	movsd	%xmm0, 24(%rsp)
	jmp	.L21
	.p2align 4,,10
	.p2align 3
.L112:
	call	__gmpn_gcd_11@PLT
.L20:
	cmpq	%r15, %r14
	movq	%r15, %rcx
	cmovbe	%r14, %rcx
	salq	%cl, %rax
.L19:
	addq	%rax, %rbp
	addq	$3, %r12
	movq	%rbp, %rax
	andl	$3, %eax
	leaq	2(%r13,%rax,2), %r13
	cmpq	%rbx, %r12
	je	.L111
.L21:
	movq	%r13, %rax
	testq	%r12, %r12
	je	.L19
	xorl	%ecx, %ecx
	xorl	%eax, %eax
	movq	%r13, %rdi
	movq	%r12, %rsi
	rep bsfq	%r13, %rcx
	rep bsfq	%r12, %rax
	movslq	%ecx, %r14
	shrq	%cl, %rdi
	movl	%eax, %ecx
	movslq	%eax, %r15
	shrq	%cl, %rsi
	cmpq	$1, %rdi
	je	.L67
	cmpq	$1, %rsi
	jne	.L112
.L67:
	movl	$1, %eax
	jmp	.L20
.L111:
	movq	8(%rsp), %rsi
	movl	$1, %edi
	leaq	6000000(%r12), %rbx
	call	clock_gettime@PLT
	pxor	%xmm0, %xmm0
	pxor	%xmm1, %xmm1
	movq	16(%rsp), %rax
	cvtsi2sdq	72(%rsp), %xmm0
	mulsd	.LC1(%rip), %xmm0
	movq	(%rsp), %rdi
	cvtsi2sdq	64(%rsp), %xmm1
	addq	$8, %rax
	addsd	%xmm1, %xmm0
	subsd	24(%rsp), %xmm0
	divsd	.LC7(%rip), %xmm0
	mulsd	.LC3(%rip), %xmm0
	movsd	%xmm0, -8(%rax)
	movq	%rax, 16(%rsp)
	cmpq	%rdi, %rax
	jne	.L22
	movq	48(%rsp), %rbx
	movq	40(%rsp), %rdi
	movq	%rbp, %r8
	movl	$1, %esi
.L26:
	movsd	(%rdi), %xmm1
	movq	%rdi, %rax
	movl	%esi, %edx
	jmp	.L23
	.p2align 4,,10
	.p2align 3
.L25:
	movsd	%xmm0, (%rax)
	subq	$8, %rax
	testl	%edx, %edx
	je	.L68
.L23:
	movsd	-8(%rax), %xmm0
	movslq	%edx, %rcx
	leal	-1(%rcx), %edx
	comisd	%xmm1, %xmm0
	ja	.L25
.L24:
	movsd	%xmm1, 336(%rsp,%rcx,8)
	addl	$1, %esi
	addq	$8, %rdi
	cmpl	$15, %esi
	jne	.L26
	leaq	88(%rsp), %rbp
	movl	$2, %edi
	movl	$3, %eax
	movsd	448(%rsp), %xmm2
	movsd	392(%rsp), %xmm1
	leaq	.LC8(%rip), %rdx
	movsd	336(%rsp), %xmm0
	leaq	.LC5(%rip), %rsi
	movq	%r8, 16(%rsp)
	leaq	96(%rsp), %r12
	leaq	120(%rsp), %r15
	call	__printf_chk@PLT
	movl	$4096, %edx
	movq	%rbx, %rsi
	movq	%rbp, %rdi
	movq	$0, 88(%rsp)
	leaq	104(%rsp), %r13
	movq	$0, 96(%rsp)
	movq	$0, 104(%rsp)
	movq	$0, 112(%rsp)
	movq	$0, 120(%rsp)
	call	fmpz_randbits@PLT
	movq	%rbp, %rsi
	movq	%rbp, %rdi
	call	fmpz_abs@PLT
	movl	$4096, %edx
	movq	%rbx, %rsi
	movq	%r12, %rdi
	call	fmpz_randbits@PLT
	movq	%r12, %rsi
	movq	%r12, %rdi
	call	fmpz_abs@PLT
	movq	%r15, %rdi
	movl	$4096, %edx
	movq	%rbx, %rsi
	call	fmpz_randbits@PLT
	movq	%r15, %rsi
	movq	%r15, %rdi
	call	fmpz_abs@PLT
	movq	16(%rsp), %r8
	movq	%r15, 48(%rsp)
	movq	32(%rsp), %r14
	movq	8(%rsp), %r15
	movq	%r8, 24(%rsp)
	.p2align 4,,10
	.p2align 3
.L28:
	movq	%r15, %rsi
	movl	$1, %edi
	movl	$1000000, %ebx
	call	clock_gettime@PLT
	pxor	%xmm0, %xmm0
	pxor	%xmm1, %xmm1
	cvtsi2sdq	72(%rsp), %xmm0
	mulsd	.LC1(%rip), %xmm0
	cvtsi2sdq	64(%rsp), %xmm1
	addsd	%xmm1, %xmm0
	movsd	%xmm0, 16(%rsp)
	.p2align 4,,10
	.p2align 3
.L27:
	movq	%r12, %rdx
	movq	%rbp, %rsi
	movq	%r13, %rdi
	call	fmpz_add@PLT
	subq	$1, %rbx
	jne	.L27
	movq	%r15, %rsi
	movl	$1, %edi
	addq	$8, %r14
	call	clock_gettime@PLT
	pxor	%xmm0, %xmm0
	pxor	%xmm1, %xmm1
	movq	(%rsp), %rax
	cvtsi2sdq	72(%rsp), %xmm0
	mulsd	.LC1(%rip), %xmm0
	cvtsi2sdq	64(%rsp), %xmm1
	addsd	%xmm1, %xmm0
	subsd	16(%rsp), %xmm0
	divsd	.LC9(%rip), %xmm0
	mulsd	.LC3(%rip), %xmm0
	movsd	%xmm0, -8(%r14)
	cmpq	%rax, %r14
	jne	.L28
	movq	24(%rsp), %r8
	movq	48(%rsp), %r15
	movl	$1, %esi
	movq	40(%rsp), %rdi
.L32:
	movsd	(%rdi), %xmm1
	movq	%rdi, %rax
	movl	%esi, %edx
	jmp	.L29
	.p2align 4,,10
	.p2align 3
.L31:
	movsd	%xmm0, (%rax)
	subq	$8, %rax
	testl	%edx, %edx
	je	.L69
.L29:
	movsd	-8(%rax), %xmm0
	movslq	%edx, %rcx
	leal	-1(%rcx), %edx
	comisd	%xmm1, %xmm0
	ja	.L31
.L30:
	movsd	%xmm1, 336(%rsp,%rcx,8)
	addl	$1, %esi
	addq	$8, %rdi
	cmpl	$15, %esi
	jne	.L32
	movsd	448(%rsp), %xmm2
	leaq	.LC10(%rip), %rdx
	movsd	392(%rsp), %xmm1
	leaq	.LC5(%rip), %rsi
	movl	$2, %edi
	movl	$3, %eax
	movq	%r8, 16(%rsp)
	movsd	336(%rsp), %xmm0
	leaq	112(%rsp), %r14
	call	__printf_chk@PLT
	movq	16(%rsp), %r8
	movq	32(%rsp), %rax
	movq	%r15, 48(%rsp)
	movq	8(%rsp), %r15
	movq	%r13, 56(%rsp)
	movq	%r8, 24(%rsp)
	movq	%rax, %r13
	.p2align 4,,10
	.p2align 3
.L34:
	movq	%r15, %rsi
	movl	$1, %edi
	movl	$100000, %ebx
	call	clock_gettime@PLT
	pxor	%xmm0, %xmm0
	pxor	%xmm1, %xmm1
	cvtsi2sdq	72(%rsp), %xmm0
	mulsd	.LC1(%rip), %xmm0
	cvtsi2sdq	64(%rsp), %xmm1
	addsd	%xmm1, %xmm0
	movsd	%xmm0, 16(%rsp)
	.p2align 4,,10
	.p2align 3
.L33:
	movq	%r12, %rdx
	movq	%rbp, %rsi
	movq	%r14, %rdi
	call	fmpz_mul@PLT
	subq	$1, %rbx
	jne	.L33
	movq	%r15, %rsi
	movl	$1, %edi
	addq	$8, %r13
	call	clock_gettime@PLT
	pxor	%xmm0, %xmm0
	pxor	%xmm1, %xmm1
	movq	(%rsp), %rax
	cvtsi2sdq	72(%rsp), %xmm0
	mulsd	.LC1(%rip), %xmm0
	cvtsi2sdq	64(%rsp), %xmm1
	addsd	%xmm1, %xmm0
	subsd	16(%rsp), %xmm0
	divsd	.LC11(%rip), %xmm0
	mulsd	.LC3(%rip), %xmm0
	movsd	%xmm0, -8(%r13)
	cmpq	%rax, %r13
	jne	.L34
	movq	24(%rsp), %r8
	movq	48(%rsp), %r15
	movl	$1, %esi
	movq	56(%rsp), %r13
	movq	40(%rsp), %rdi
.L38:
	movsd	(%rdi), %xmm1
	movq	%rdi, %rax
	movl	%esi, %edx
	jmp	.L35
	.p2align 4,,10
	.p2align 3
.L37:
	movsd	%xmm0, (%rax)
	subq	$8, %rax
	testl	%edx, %edx
	je	.L70
.L35:
	movsd	-8(%rax), %xmm0
	movslq	%edx, %rcx
	leal	-1(%rcx), %edx
	comisd	%xmm1, %xmm0
	ja	.L37
.L36:
	movsd	%xmm1, 336(%rsp,%rcx,8)
	addl	$1, %esi
	addq	$8, %rdi
	cmpl	$15, %esi
	jne	.L38
	movsd	448(%rsp), %xmm2
	leaq	.LC12(%rip), %rdx
	movsd	392(%rsp), %xmm1
	leaq	.LC5(%rip), %rsi
	movl	$2, %edi
	movl	$3, %eax
	movq	%r8, 16(%rsp)
	movsd	336(%rsp), %xmm0
	call	__printf_chk@PLT
	movq	16(%rsp), %r8
	movq	32(%rsp), %rax
	movq	%r12, 56(%rsp)
	movq	8(%rsp), %r12
	movq	%rbp, 48(%rsp)
	movq	%r8, 24(%rsp)
	movq	%rax, %rbp
	.p2align 4,,10
	.p2align 3
.L40:
	movq	%r12, %rsi
	movl	$1, %edi
	movl	$100000, %ebx
	call	clock_gettime@PLT
	pxor	%xmm0, %xmm0
	pxor	%xmm1, %xmm1
	cvtsi2sdq	72(%rsp), %xmm0
	mulsd	.LC1(%rip), %xmm0
	cvtsi2sdq	64(%rsp), %xmm1
	addsd	%xmm1, %xmm0
	movsd	%xmm0, 16(%rsp)
	.p2align 4,,10
	.p2align 3
.L39:
	movq	%r15, %rdx
	movq	%r14, %rsi
	movq	%r13, %rdi
	call	fmpz_mod@PLT
	subq	$1, %rbx
	jne	.L39
	movq	%r12, %rsi
	movl	$1, %edi
	addq	$8, %rbp
	call	clock_gettime@PLT
	pxor	%xmm0, %xmm0
	pxor	%xmm1, %xmm1
	movq	(%rsp), %rax
	cvtsi2sdq	72(%rsp), %xmm0
	mulsd	.LC1(%rip), %xmm0
	cvtsi2sdq	64(%rsp), %xmm1
	addsd	%xmm1, %xmm0
	subsd	16(%rsp), %xmm0
	divsd	.LC11(%rip), %xmm0
	mulsd	.LC3(%rip), %xmm0
	movsd	%xmm0, -8(%rbp)
	cmpq	%rax, %rbp
	jne	.L40
	movq	24(%rsp), %r8
	movq	48(%rsp), %rbp
	movl	$1, %esi
	movq	56(%rsp), %r12
	movq	40(%rsp), %rdi
.L44:
	movsd	(%rdi), %xmm1
	movq	%rdi, %rax
	movl	%esi, %edx
	jmp	.L41
	.p2align 4,,10
	.p2align 3
.L43:
	movsd	%xmm0, (%rax)
	subq	$8, %rax
	testl	%edx, %edx
	je	.L71
.L41:
	movsd	-8(%rax), %xmm0
	movslq	%edx, %rcx
	leal	-1(%rcx), %edx
	comisd	%xmm1, %xmm0
	ja	.L43
.L42:
	movsd	%xmm1, 336(%rsp,%rcx,8)
	addl	$1, %esi
	addq	$8, %rdi
	cmpl	$15, %esi
	jne	.L44
	movsd	448(%rsp), %xmm2
	leaq	.LC13(%rip), %rdx
	movsd	392(%rsp), %xmm1
	leaq	.LC5(%rip), %rsi
	movl	$2, %edi
	movl	$3, %eax
	movsd	336(%rsp), %xmm0
	movq	%r8, 16(%rsp)
	leaq	408(%rsp), %r15
	call	__printf_chk@PLT
	movq	16(%rsp), %r8
	movq	32(%rsp), %r14
	movq	%r8, 24(%rsp)
.L46:
	movq	8(%rsp), %rsi
	movl	$1, %edi
	movl	$5000, %ebx
	call	clock_gettime@PLT
	pxor	%xmm0, %xmm0
	pxor	%xmm1, %xmm1
	cvtsi2sdq	72(%rsp), %xmm0
	mulsd	.LC1(%rip), %xmm0
	cvtsi2sdq	64(%rsp), %xmm1
	addsd	%xmm1, %xmm0
	movsd	%xmm0, 16(%rsp)
	.p2align 4,,10
	.p2align 3
.L45:
	movq	%r12, %rdx
	movq	%rbp, %rsi
	movq	%r13, %rdi
	call	fmpz_gcd@PLT
	subq	$1, %rbx
	jne	.L45
	movq	8(%rsp), %rsi
	movl	$1, %edi
	addq	$8, %r14
	call	clock_gettime@PLT
	pxor	%xmm0, %xmm0
	pxor	%xmm1, %xmm1
	cvtsi2sdq	72(%rsp), %xmm0
	mulsd	.LC1(%rip), %xmm0
	cvtsi2sdq	64(%rsp), %xmm1
	addsd	%xmm1, %xmm0
	subsd	16(%rsp), %xmm0
	divsd	.LC14(%rip), %xmm0
	mulsd	.LC3(%rip), %xmm0
	movsd	%xmm0, -8(%r14)
	cmpq	%r14, %r15
	jne	.L46
	movq	24(%rsp), %r8
	movq	40(%rsp), %rdi
	movl	$1, %esi
.L50:
	movsd	(%rdi), %xmm1
	movq	%rdi, %rax
	movl	%esi, %edx
	jmp	.L47
	.p2align 4,,10
	.p2align 3
.L49:
	movsd	%xmm0, (%rax)
	subq	$8, %rax
	testl	%edx, %edx
	je	.L72
.L47:
	movsd	-8(%rax), %xmm0
	movslq	%edx, %rcx
	leal	-1(%rcx), %edx
	comisd	%xmm1, %xmm0
	ja	.L49
.L48:
	movsd	%xmm1, 336(%rsp,%rcx,8)
	addl	$1, %esi
	addq	$8, %rdi
	cmpl	$9, %esi
	jne	.L50
	movsd	400(%rsp), %xmm2
	leaq	.LC15(%rip), %rdx
	movsd	368(%rsp), %xmm1
	leaq	.LC5(%rip), %rsi
	movl	$2, %edi
	movl	$3, %eax
	movsd	336(%rsp), %xmm0
	movq	%r8, 16(%rsp)
	leaq	128(%rsp), %r12
	leaq	176(%rsp), %rbp
	call	__printf_chk@PLT
	movq	%r13, %rdi
	leaq	224(%rsp), %r13
	call	fmpz_get_ui@PLT
	movq	16(%rsp), %r8
	pxor	%xmm0, %xmm0
	movq	%r12, %rdi
	movl	$3, %esi
	movaps	%xmm0, 128(%rsp)
	leaq	(%rax,%r8), %r15
	movaps	%xmm0, 176(%rsp)
	movaps	%xmm0, 224(%rsp)
	movq	$0, 160(%rsp)
	movq	$0, 168(%rsp)
	movq	$0, 208(%rsp)
	movq	$0, 216(%rsp)
	movq	$0, 256(%rsp)
	movq	$0, 264(%rsp)
	call	arb_set_ui@PLT
	movl	$128, %edx
	movq	%r12, %rsi
	movq	%r12, %rdi
	call	arb_sqrt@PLT
	movl	$5, %esi
	movq	%rbp, %rdi
	call	arb_set_ui@PLT
	movl	$128, %edx
	movq	%rbp, %rsi
	movq	%rbp, %rdi
	call	arb_sqrt@PLT
	movl	$128, %ecx
	movq	%rbp, %rdx
	movq	%rbp, %rdi
	movl	$1, %esi
	call	arb_ui_div@PLT
	movq	%r15, 24(%rsp)
	movq	32(%rsp), %r14
	movq	8(%rsp), %r15
	.p2align 4,,10
	.p2align 3
.L52:
	movq	%r15, %rsi
	movl	$1, %edi
	movl	$5000000, %ebx
	call	clock_gettime@PLT
	pxor	%xmm0, %xmm0
	pxor	%xmm1, %xmm1
	cvtsi2sdq	72(%rsp), %xmm0
	mulsd	.LC1(%rip), %xmm0
	cvtsi2sdq	64(%rsp), %xmm1
	addsd	%xmm1, %xmm0
	movsd	%xmm0, 16(%rsp)
	.p2align 4,,10
	.p2align 3
.L51:
	movl	$128, %ecx
	movq	%rbp, %rdx
	movq	%r12, %rsi
	movq	%r13, %rdi
	call	arb_add@PLT
	subq	$1, %rbx
	jne	.L51
	movq	%r15, %rsi
	movl	$1, %edi
	addq	$8, %r14
	call	clock_gettime@PLT
	pxor	%xmm0, %xmm0
	pxor	%xmm1, %xmm1
	cvtsi2sdq	72(%rsp), %xmm0
	mulsd	.LC1(%rip), %xmm0
	cvtsi2sdq	64(%rsp), %xmm1
	addsd	%xmm1, %xmm0
	subsd	16(%rsp), %xmm0
	divsd	.LC16(%rip), %xmm0
	mulsd	.LC3(%rip), %xmm0
	movsd	%xmm0, -8(%r14)
	cmpq	%r14, (%rsp)
	jne	.L52
	movq	24(%rsp), %r15
	movq	40(%rsp), %rdi
	movl	$1, %esi
.L56:
	movsd	(%rdi), %xmm1
	movq	%rdi, %rax
	movl	%esi, %edx
	jmp	.L53
	.p2align 4,,10
	.p2align 3
.L55:
	movsd	%xmm0, (%rax)
	subq	$8, %rax
	testl	%edx, %edx
	je	.L73
.L53:
	movsd	-8(%rax), %xmm0
	movslq	%edx, %rcx
	leal	-1(%rcx), %edx
	comisd	%xmm1, %xmm0
	ja	.L55
.L54:
	movsd	%xmm1, 336(%rsp,%rcx,8)
	addl	$1, %esi
	addq	$8, %rdi
	cmpl	$15, %esi
	jne	.L56
	movsd	448(%rsp), %xmm2
	leaq	.LC17(%rip), %rdx
	movsd	392(%rsp), %xmm1
	leaq	.LC5(%rip), %rsi
	movsd	336(%rsp), %xmm0
	movl	$2, %edi
	movl	$3, %eax
	call	__printf_chk@PLT
	movq	%r15, 16(%rsp)
	movq	32(%rsp), %r14
	movq	8(%rsp), %r15
	.p2align 4,,10
	.p2align 3
.L58:
	movq	%r15, %rsi
	movl	$1, %edi
	movl	$5000000, %ebx
	call	clock_gettime@PLT
	pxor	%xmm0, %xmm0
	pxor	%xmm1, %xmm1
	cvtsi2sdq	72(%rsp), %xmm0
	mulsd	.LC1(%rip), %xmm0
	cvtsi2sdq	64(%rsp), %xmm1
	addsd	%xmm1, %xmm0
	movsd	%xmm0, 8(%rsp)
	.p2align 4,,10
	.p2align 3
.L57:
	movl	$128, %ecx
	movq	%rbp, %rdx
	movq	%r12, %rsi
	movq	%r13, %rdi
	call	arb_mul@PLT
	subq	$1, %rbx
	jne	.L57
	movq	%r15, %rsi
	movl	$1, %edi
	addq	$8, %r14
	call	clock_gettime@PLT
	pxor	%xmm0, %xmm0
	pxor	%xmm1, %xmm1
	cvtsi2sdq	72(%rsp), %xmm0
	mulsd	.LC1(%rip), %xmm0
	cvtsi2sdq	64(%rsp), %xmm1
	addsd	%xmm1, %xmm0
	subsd	8(%rsp), %xmm0
	divsd	.LC16(%rip), %xmm0
	mulsd	.LC3(%rip), %xmm0
	movsd	%xmm0, -8(%r14)
	cmpq	%r14, (%rsp)
	jne	.L58
	movq	16(%rsp), %r15
	movq	40(%rsp), %rsi
	movl	$1, %edi
.L62:
	movsd	(%rsi), %xmm1
	movq	%rsi, %rax
	movl	%edi, %edx
	jmp	.L59
	.p2align 4,,10
	.p2align 3
.L61:
	movsd	%xmm0, (%rax)
	subq	$8, %rax
	testl	%edx, %edx
	je	.L74
.L59:
	movsd	-8(%rax), %xmm0
	movslq	%edx, %rcx
	leal	-1(%rcx), %edx
	comisd	%xmm1, %xmm0
	ja	.L61
.L60:
	movsd	%xmm1, 336(%rsp,%rcx,8)
	addl	$1, %edi
	addq	$8, %rsi
	cmpl	$15, %edi
	jne	.L62
	movsd	448(%rsp), %xmm2
	leaq	.LC18(%rip), %rdx
	movsd	392(%rsp), %xmm1
	leaq	.LC5(%rip), %rsi
	movsd	336(%rsp), %xmm0
	movl	$2, %edi
	movl	$3, %eax
	call	__printf_chk@PLT
	movq	%r15, %rdx
	movl	$2, %edi
	xorl	%eax, %eax
	leaq	.LC19(%rip), %rsi
	call	__printf_chk@PLT
	xorl	%eax, %eax
	movl	$24, %r9d
	movl	$96, %r8d
	movl	$48, %ecx
	movl	$8, %edx
	leaq	.LC20(%rip), %rsi
	leaq	.LC21(%rip), %rdi
	call	flint_printf@PLT
	movq	456(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L113
	addq	$472, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	xorl	%eax, %eax
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
.L74:
	.cfi_restore_state
	xorl	%ecx, %ecx
	jmp	.L60
.L64:
	xorl	%ecx, %ecx
	jmp	.L8
.L65:
	xorl	%esi, %esi
	jmp	.L16
.L68:
	xorl	%ecx, %ecx
	jmp	.L24
.L69:
	xorl	%ecx, %ecx
	jmp	.L30
.L70:
	xorl	%ecx, %ecx
	jmp	.L36
.L71:
	xorl	%ecx, %ecx
	jmp	.L42
.L73:
	xorl	%ecx, %ecx
	jmp	.L54
.L72:
	xorl	%ecx, %ecx
	jmp	.L48
.L113:
	call	__stack_chk_fail@PLT
	.cfi_endproc
.LFE434:
	.size	main, .-main
	.section	.rodata.cst16,"aM",@progbits,16
	.align 16
.LC0:
	.quad	-4601097622831300607
	.quad	-5304373996139296842
	.section	.rodata.cst8,"aM",@progbits,8
	.align 8
.LC1:
	.long	-400107883
	.long	1041313291
	.align 8
.LC2:
	.long	0
	.long	1098060496
	.align 8
.LC3:
	.long	0
	.long	1104006501
	.align 8
.LC7:
	.long	0
	.long	1094616192
	.align 8
.LC9:
	.long	0
	.long	1093567616
	.align 8
.LC11:
	.long	0
	.long	1090021888
	.align 8
.LC14:
	.long	0
	.long	1085507584
	.align 8
.LC16:
	.long	0
	.long	1095963344
	.ident	"GCC: (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0"
	.section	.note.GNU-stack,"",@progbits
	.section	.note.gnu.property,"a"
	.align 8
	.long	1f - 0f
	.long	4f - 1f
	.long	5
0:
	.string	"GNU"
1:
	.align 8
	.long	0xc0000002
	.long	3f - 2f
2:
	.long	0x3
3:
	.align 8
4:
