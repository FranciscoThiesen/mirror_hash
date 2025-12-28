	.build_version macos, 15, 0	sdk_version 15, 4
	.section	__TEXT,__text,regular,pure_instructions
	.globl	_rapidhash_nano_wrapper         ## -- Begin function rapidhash_nano_wrapper
	.p2align	4
_rapidhash_nano_wrapper:                ## @rapidhash_nano_wrapper
	.cfi_startproc
## %bb.0:
	movq	%rdx, %rcx
	movabsq	$-8378864009470890807, %r10     ## imm = 0x8BB84B93962EACC9
	movabsq	$5418857496715711651, %r11      ## imm = 0x4B33A62ED433D4A3
	movq	%rdx, %rax
	xorq	%r11, %rax
	mulq	%r10
	xorq	%rcx, %rdx
	xorq	%rax, %rdx
	cmpq	$16, %rsi
	ja	LBB0_8
## %bb.1:
	cmpq	$4, %rsi
	jb	LBB0_5
## %bb.2:
	xorq	%rsi, %rdx
	cmpq	$8, %rsi
	jb	LBB0_4
## %bb.3:
	movq	(%rdi), %rcx
	movq	-8(%rdi,%rsi), %rax
	jmp	LBB0_15
LBB0_5:
	testq	%rsi, %rsi
	je	LBB0_6
## %bb.7:
	movzbl	(%rdi), %eax
	shlq	$45, %rax
	movzbl	-1(%rdi,%rsi), %ecx
	orq	%rax, %rcx
	movq	%rsi, %rax
	shrq	%rax
	movzbl	(%rdi,%rax), %eax
	jmp	LBB0_15
LBB0_4:
	movl	(%rdi), %ecx
	movl	-4(%rdi,%rsi), %eax
	jmp	LBB0_15
LBB0_6:
	xorl	%ecx, %ecx
	xorl	%eax, %eax
	xorl	%esi, %esi
LBB0_15:
	xorq	%rdx, %rax
	xorq	%r10, %rcx
	mulq	%rcx
	movabsq	$-6148914691236517206, %rcx     ## imm = 0xAAAAAAAAAAAAAAAA
	xorq	%rax, %rcx
	xorq	%r10, %rsi
	xorq	%rdx, %rsi
	movq	%rsi, %rax
	mulq	%rcx
	xorq	%rdx, %rax
	retq
LBB0_8:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	pushq	%rbx
	.cfi_offset %rbx, -24
	cmpq	$49, %rsi
	jb	LBB0_12
## %bb.9:
	movabsq	$3257665815644502181, %rbx      ## imm = 0x2D358DCCAA6C78A5
	movq	%rdx, %rcx
	movq	%rdx, %r8
LBB0_10:                                ## =>This Inner Loop Header: Depth=1
	movq	(%rdi), %r9
	xorq	%rbx, %r9
	xorq	8(%rdi), %rdx
	movq	%rdx, %rax
	mulq	%r9
	movq	%rdx, %r9
	xorq	%rax, %r9
	movq	16(%rdi), %rdx
	xorq	24(%rdi), %rcx
	xorq	%r10, %rdx
	movq	%rcx, %rax
	mulq	%rdx
	movq	%rdx, %rcx
	xorq	%rax, %rcx
	movq	32(%rdi), %rdx
	xorq	%r11, %rdx
	xorq	40(%rdi), %r8
	movq	%r8, %rax
	mulq	%rdx
	movq	%rdx, %r8
	movq	%r9, %rdx
	xorq	%rax, %r8
	addq	$48, %rdi
	addq	$-48, %rsi
	cmpq	$48, %rsi
	ja	LBB0_10
## %bb.11:
	xorq	%rdx, %rcx
	xorq	%r8, %rcx
	movq	%rcx, %rdx
	cmpq	$17, %rsi
	jb	LBB0_14
LBB0_12:
	movq	(%rdi), %rcx
	xorq	8(%rdi), %rdx
	xorq	%r11, %rcx
	movq	%rdx, %rax
	mulq	%rcx
	xorq	%rax, %rdx
	cmpq	$33, %rsi
	jb	LBB0_14
## %bb.13:
	xorq	16(%rdi), %r11
	xorq	24(%rdi), %rdx
	movq	%rdx, %rax
	mulq	%r11
	xorq	%rax, %rdx
LBB0_14:
	movq	-16(%rdi,%rsi), %rcx
	xorq	%rsi, %rcx
	movq	-8(%rdi,%rsi), %rax
	popq	%rbx
	popq	%rbp
	jmp	LBB0_15
	.cfi_endproc
                                        ## -- End function
	.globl	_rapidhash_micro_wrapper        ## -- Begin function rapidhash_micro_wrapper
	.p2align	4
_rapidhash_micro_wrapper:               ## @rapidhash_micro_wrapper
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	.cfi_offset %rbx, -56
	.cfi_offset %r12, -48
	.cfi_offset %r13, -40
	.cfi_offset %r14, -32
	.cfi_offset %r15, -24
	movq	%rdx, %rcx
	movabsq	$-8378864009470890807, %rbx     ## imm = 0x8BB84B93962EACC9
	movabsq	$5418857496715711651, %r14      ## imm = 0x4B33A62ED433D4A3
	movq	%rdx, %rax
	xorq	%r14, %rax
	mulq	%rbx
	xorq	%rcx, %rdx
	xorq	%rax, %rdx
	cmpq	$16, %rsi
	ja	LBB1_8
## %bb.1:
	cmpq	$4, %rsi
	jb	LBB1_5
## %bb.2:
	xorq	%rsi, %rdx
	cmpq	$8, %rsi
	jb	LBB1_4
## %bb.3:
	movq	(%rdi), %rcx
LBB1_17:
	movq	-8(%rdi,%rsi), %rax
	jmp	LBB1_18
LBB1_5:
	testq	%rsi, %rsi
	je	LBB1_6
## %bb.7:
	movzbl	(%rdi), %eax
	shlq	$45, %rax
	movzbl	-1(%rdi,%rsi), %ecx
	orq	%rax, %rcx
	movq	%rsi, %rax
	shrq	%rax
	movzbl	(%rdi,%rax), %eax
	jmp	LBB1_18
LBB1_4:
	movl	(%rdi), %ecx
	movl	-4(%rdi,%rsi), %eax
	jmp	LBB1_18
LBB1_6:
	xorl	%ecx, %ecx
	xorl	%eax, %eax
	xorl	%esi, %esi
LBB1_18:
	xorq	%rdx, %rax
	xorq	%rbx, %rcx
	mulq	%rcx
	movabsq	$-6148914691236517206, %rcx     ## imm = 0xAAAAAAAAAAAAAAAA
	xorq	%rax, %rcx
	xorq	%rbx, %rsi
	xorq	%rdx, %rsi
	movq	%rsi, %rax
	mulq	%rcx
	xorq	%rdx, %rax
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
LBB1_8:
	cmpq	$81, %rsi
	jb	LBB1_12
## %bb.9:
	movabsq	$3257665815644502181, %r15      ## imm = 0x2D358DCCAA6C78A5
	movabsq	$5573817676018592327, %r12      ## imm = 0x4D5A2DA51DE1AA47
	movabsq	$-6884282663029611473, %r13     ## imm = 0xA0761D6478BD642F
	movq	%rdx, %r8
	movq	%rdx, %rcx
	movq	%rdx, %r10
	movq	%rdx, %r9
LBB1_10:                                ## =>This Inner Loop Header: Depth=1
	movq	(%rdi), %r11
	xorq	%r15, %r11
	xorq	8(%rdi), %rdx
	movq	%rdx, %rax
	mulq	%r11
	movq	%rdx, %r11
	xorq	%rax, %r11
	movq	16(%rdi), %rdx
	xorq	24(%rdi), %r8
	xorq	%rbx, %rdx
	movq	%r8, %rax
	mulq	%rdx
	movq	%rdx, %r8
	xorq	%rax, %r8
	movq	32(%rdi), %rdx
	xorq	%r14, %rdx
	xorq	40(%rdi), %rcx
	movq	%rcx, %rax
	mulq	%rdx
	movq	%rdx, %rcx
	xorq	%rax, %rcx
	movq	48(%rdi), %rdx
	xorq	56(%rdi), %r10
	xorq	%r12, %rdx
	movq	%r10, %rax
	mulq	%rdx
	movq	%rdx, %r10
	xorq	%rax, %r10
	movq	64(%rdi), %rdx
	xorq	%r13, %rdx
	xorq	72(%rdi), %r9
	movq	%r9, %rax
	mulq	%rdx
	movq	%rdx, %r9
	movq	%r11, %rdx
	xorq	%rax, %r9
	addq	$80, %rdi
	addq	$-80, %rsi
	cmpq	$80, %rsi
	ja	LBB1_10
## %bb.11:
	xorq	%rdx, %r8
	xorq	%r10, %rcx
	xorq	%r8, %rcx
	xorq	%r9, %rcx
	movq	%rcx, %rdx
	cmpq	$17, %rsi
	jb	LBB1_16
LBB1_12:
	movq	(%rdi), %rcx
	xorq	8(%rdi), %rdx
	xorq	%r14, %rcx
	movq	%rdx, %rax
	mulq	%rcx
	xorq	%rax, %rdx
	cmpq	$33, %rsi
	jb	LBB1_16
## %bb.13:
	xorq	16(%rdi), %r14
	xorq	24(%rdi), %rdx
	movq	%rdx, %rax
	mulq	%r14
	xorq	%rax, %rdx
	cmpq	$49, %rsi
	jb	LBB1_16
## %bb.14:
	movq	32(%rdi), %rcx
	xorq	40(%rdi), %rdx
	xorq	%rbx, %rcx
	movq	%rdx, %rax
	mulq	%rcx
	xorq	%rax, %rdx
	cmpq	$65, %rsi
	jb	LBB1_16
## %bb.15:
	movq	48(%rdi), %rcx
	xorq	56(%rdi), %rdx
	xorq	%rbx, %rcx
	movq	%rdx, %rax
	mulq	%rcx
	xorq	%rax, %rdx
LBB1_16:
	movq	-16(%rdi,%rsi), %rcx
	xorq	%rsi, %rcx
	jmp	LBB1_17
	.cfi_endproc
                                        ## -- End function
	.globl	_rapidhash_bulk_wrapper         ## -- Begin function rapidhash_bulk_wrapper
	.p2align	4
_rapidhash_bulk_wrapper:                ## @rapidhash_bulk_wrapper
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	.cfi_offset %rbx, -56
	.cfi_offset %r12, -48
	.cfi_offset %r13, -40
	.cfi_offset %r14, -32
	.cfi_offset %r15, -24
	movq	%rdx, %rcx
	movabsq	$-8378864009470890807, %r15     ## imm = 0x8BB84B93962EACC9
	movabsq	$5418857496715711651, %r12      ## imm = 0x4B33A62ED433D4A3
	movq	%rdx, %rax
	xorq	%r12, %rax
	mulq	%r15
	xorq	%rcx, %rdx
	xorq	%rax, %rdx
	cmpq	$16, %rsi
	ja	LBB2_8
## %bb.1:
	cmpq	$4, %rsi
	jb	LBB2_5
## %bb.2:
	xorq	%rsi, %rdx
	cmpq	$8, %rsi
	jb	LBB2_4
## %bb.3:
	movq	(%rdi), %rcx
LBB2_19:
	movq	-8(%rdi,%rsi), %rax
	jmp	LBB2_20
LBB2_5:
	testq	%rsi, %rsi
	je	LBB2_6
## %bb.7:
	movzbl	(%rdi), %eax
	shlq	$45, %rax
	movzbl	-1(%rdi,%rsi), %ecx
	orq	%rax, %rcx
	movq	%rsi, %rax
	shrq	%rax
	movzbl	(%rdi,%rax), %eax
	jmp	LBB2_20
LBB2_4:
	movl	(%rdi), %ecx
	movl	-4(%rdi,%rsi), %eax
	jmp	LBB2_20
LBB2_6:
	xorl	%ecx, %ecx
	xorl	%eax, %eax
	xorl	%esi, %esi
LBB2_20:
	xorq	%rdx, %rax
	xorq	%r15, %rcx
	mulq	%rcx
	movabsq	$-6148914691236517206, %rcx     ## imm = 0xAAAAAAAAAAAAAAAA
	xorq	%rax, %rcx
	xorq	%r15, %rsi
	xorq	%rdx, %rsi
	movq	%rsi, %rax
	mulq	%rcx
	xorq	%rdx, %rax
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
LBB2_8:
	cmpq	$113, %rsi
	jb	LBB2_12
## %bb.9:
	movabsq	$-8003715239535429492, %r13     ## imm = 0x90ED1765281C388C
	movq	%rdx, %r9
	movq	%rdx, %rcx
	movq	%rdx, %rbx
	movq	%rdx, %r8
	movq	%rdx, %r11
	movq	%rdx, %r10
LBB2_10:                                ## =>This Inner Loop Header: Depth=1
	movq	(%rdi), %r14
	movabsq	$3257665815644502181, %rax      ## imm = 0x2D358DCCAA6C78A5
	xorq	%rax, %r14
	xorq	8(%rdi), %rdx
	movq	%rdx, %rax
	mulq	%r14
	movq	%rdx, %r14
	xorq	%rax, %r14
	movq	16(%rdi), %rdx
	xorq	24(%rdi), %r9
	xorq	%r15, %rdx
	movq	%r9, %rax
	mulq	%rdx
	movq	%rdx, %r9
	xorq	%rax, %r9
	movq	32(%rdi), %rdx
	xorq	%r12, %rdx
	xorq	40(%rdi), %rcx
	movq	%rcx, %rax
	mulq	%rdx
	movq	%rdx, %rcx
	xorq	%rax, %rcx
	movq	48(%rdi), %rdx
	xorq	56(%rdi), %rbx
	movabsq	$5573817676018592327, %rax      ## imm = 0x4D5A2DA51DE1AA47
	xorq	%rax, %rdx
	movq	%rbx, %rax
	mulq	%rdx
	movq	%rdx, %rbx
	xorq	%rax, %rbx
	movq	64(%rdi), %rdx
	movabsq	$-6884282663029611473, %rax     ## imm = 0xA0761D6478BD642F
	xorq	%rax, %rdx
	xorq	72(%rdi), %r8
	movq	%r8, %rax
	mulq	%rdx
	movq	%rdx, %r8
	xorq	%rax, %r8
	movq	80(%rdi), %rdx
	xorq	88(%rdi), %r11
	movabsq	$-1800455987208640293, %rax     ## imm = 0xE7037ED1A0B428DB
	xorq	%rax, %rdx
	movq	%r11, %rax
	mulq	%rdx
	movq	%rdx, %r11
	xorq	%rax, %r11
	movq	96(%rdi), %rdx
	xorq	%r13, %rdx
	xorq	104(%rdi), %r10
	movq	%r10, %rax
	mulq	%rdx
	movq	%rdx, %r10
	movq	%r14, %rdx
	xorq	%rax, %r10
	addq	$112, %rdi
	addq	$-112, %rsi
	cmpq	$112, %rsi
	ja	LBB2_10
## %bb.11:
	xorq	%rdx, %r9
	xorq	%rbx, %rcx
	xorq	%r9, %rcx
	xorq	%r11, %r8
	xorq	%r10, %r8
	xorq	%rcx, %r8
	movq	%r8, %rdx
	cmpq	$17, %rsi
	jb	LBB2_18
LBB2_12:
	movq	(%rdi), %rcx
	xorq	8(%rdi), %rdx
	xorq	%r12, %rcx
	movq	%rdx, %rax
	mulq	%rcx
	xorq	%rax, %rdx
	cmpq	$33, %rsi
	jb	LBB2_18
## %bb.13:
	movq	16(%rdi), %rcx
	xorq	24(%rdi), %rdx
	xorq	%r12, %rcx
	movq	%rdx, %rax
	mulq	%rcx
	xorq	%rax, %rdx
	cmpq	$49, %rsi
	jb	LBB2_18
## %bb.14:
	movq	32(%rdi), %rcx
	xorq	40(%rdi), %rdx
	xorq	%r15, %rcx
	movq	%rdx, %rax
	mulq	%rcx
	xorq	%rax, %rdx
	cmpq	$65, %rsi
	jb	LBB2_18
## %bb.15:
	movq	48(%rdi), %rcx
	xorq	56(%rdi), %rdx
	xorq	%r15, %rcx
	movq	%rdx, %rax
	mulq	%rcx
	xorq	%rax, %rdx
	cmpq	$81, %rsi
	jb	LBB2_18
## %bb.16:
	xorq	64(%rdi), %r12
	movq	%rdx, %rax
	xorq	72(%rdi), %rax
	mulq	%r12
	xorq	%rax, %rdx
	cmpq	$97, %rsi
	jb	LBB2_18
## %bb.17:
	movq	80(%rdi), %rcx
	movq	%rdx, %rax
	xorq	88(%rdi), %rax
	xorq	%r15, %rcx
	mulq	%rcx
	xorq	%rax, %rdx
LBB2_18:
	movq	-16(%rdi,%rsi), %rcx
	xorq	%rsi, %rcx
	jmp	LBB2_19
	.cfi_endproc
                                        ## -- End function
.subsections_via_symbols
