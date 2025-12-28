	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 15, 0	sdk_version 15, 4
	.globl	_rapidhash_nano_wrapper         ; -- Begin function rapidhash_nano_wrapper
	.p2align	2
_rapidhash_nano_wrapper:                ; @rapidhash_nano_wrapper
	.cfi_startproc
; %bb.0:
	mov	x8, #44233
	movk	x8, #38446, lsl #16
	movk	x8, #19347, lsl #32
	movk	x8, #35768, lsl #48
	mov	x10, #54435
	movk	x10, #54323, lsl #16
	movk	x10, #42542, lsl #32
	movk	x10, #19251, lsl #48
	eor	x9, x2, x10
	mul	x11, x9, x8
	umulh	x9, x9, x8
	eor	x9, x9, x11
	eor	x9, x9, x2
	cmp	x1, #16
	b.hi	LBB0_9
; %bb.1:
	cmp	x1, #4
	b.lo	LBB0_4
; %bb.2:
	eor	x9, x9, x1
	add	x11, x0, x1
	cmp	x1, #8
	b.lo	LBB0_6
; %bb.3:
	ldr	x10, [x0]
	ldur	x11, [x11, #-8]
	b	LBB0_8
LBB0_4:
	cbz	x1, LBB0_7
; %bb.5:
	ldrb	w11, [x0]
	add	x10, x0, x1
	ldurb	w10, [x10, #-1]
	bfi	x10, x11, #45, #8
	lsr	x11, x1, #1
	ldrb	w11, [x0, x11]
	b	LBB0_8
LBB0_6:
	ldr	w10, [x0]
	ldur	w11, [x11, #-4]
	b	LBB0_8
LBB0_7:
	mov	x10, #0
	mov	x11, #0
LBB0_8:
	eor	x10, x10, x8
	eor	x9, x9, x11
	mul	x11, x9, x10
	umulh	x9, x9, x10
	eor	x10, x11, #0xaaaaaaaaaaaaaaaa
	eor	x9, x1, x9
	eor	x8, x9, x8
	mul	x9, x8, x10
	umulh	x8, x8, x10
	eor	x0, x8, x9
	ret
LBB0_9:
	cmp	x1, #49
	b.lo	LBB0_13
; %bb.10:
	mov	x11, #30885
	movk	x11, #43628, lsl #16
	movk	x11, #36300, lsl #32
	movk	x11, #11573, lsl #48
	mov	x12, x9
	mov	x13, x9
LBB0_11:                                ; =>This Inner Loop Header: Depth=1
	ldp	x14, x15, [x0]
	eor	x14, x14, x11
	eor	x9, x15, x9
	mul	x15, x9, x14
	umulh	x9, x9, x14
	eor	x9, x9, x15
	ldp	x14, x15, [x0, #16]
	eor	x14, x14, x8
	eor	x12, x15, x12
	mul	x15, x12, x14
	umulh	x12, x12, x14
	eor	x12, x12, x15
	ldp	x14, x15, [x0, #32]
	eor	x14, x14, x10
	eor	x13, x15, x13
	mul	x15, x13, x14
	umulh	x13, x13, x14
	eor	x13, x13, x15
	add	x0, x0, #48
	sub	x1, x1, #48
	cmp	x1, #48
	b.hi	LBB0_11
; %bb.12:
	eor	x9, x12, x9
	eor	x9, x9, x13
	cmp	x1, #17
	b.lo	LBB0_15
LBB0_13:
	ldp	x11, x12, [x0]
	eor	x11, x11, x10
	eor	x9, x12, x9
	mul	x12, x9, x11
	umulh	x9, x9, x11
	eor	x9, x9, x12
	cmp	x1, #33
	b.lo	LBB0_15
; %bb.14:
	ldp	x11, x12, [x0, #16]
	eor	x10, x11, x10
	eor	x9, x12, x9
	mul	x11, x9, x10
	umulh	x9, x9, x10
	eor	x9, x9, x11
LBB0_15:
	add	x10, x0, x1
	ldp	x12, x11, [x10, #-16]
	eor	x10, x12, x1
	b	LBB0_8
	.cfi_endproc
                                        ; -- End function
	.globl	_rapidhash_micro_wrapper        ; -- Begin function rapidhash_micro_wrapper
	.p2align	2
_rapidhash_micro_wrapper:               ; @rapidhash_micro_wrapper
	.cfi_startproc
; %bb.0:
	mov	x8, #44233
	movk	x8, #38446, lsl #16
	movk	x8, #19347, lsl #32
	movk	x8, #35768, lsl #48
	mov	x10, #54435
	movk	x10, #54323, lsl #16
	movk	x10, #42542, lsl #32
	movk	x10, #19251, lsl #48
	eor	x9, x2, x10
	mul	x11, x9, x8
	umulh	x9, x9, x8
	eor	x9, x9, x11
	eor	x9, x9, x2
	cmp	x1, #16
	b.hi	LBB1_9
; %bb.1:
	cmp	x1, #4
	b.lo	LBB1_4
; %bb.2:
	eor	x9, x9, x1
	add	x11, x0, x1
	cmp	x1, #8
	b.lo	LBB1_6
; %bb.3:
	ldr	x10, [x0]
	ldur	x11, [x11, #-8]
	b	LBB1_8
LBB1_4:
	cbz	x1, LBB1_7
; %bb.5:
	ldrb	w11, [x0]
	add	x10, x0, x1
	ldurb	w10, [x10, #-1]
	bfi	x10, x11, #45, #8
	lsr	x11, x1, #1
	ldrb	w11, [x0, x11]
	b	LBB1_8
LBB1_6:
	ldr	w10, [x0]
	ldur	w11, [x11, #-4]
	b	LBB1_8
LBB1_7:
	mov	x10, #0
	mov	x11, #0
LBB1_8:
	eor	x10, x10, x8
	eor	x9, x9, x11
	mul	x11, x9, x10
	umulh	x9, x9, x10
	eor	x10, x11, #0xaaaaaaaaaaaaaaaa
	eor	x9, x1, x9
	eor	x8, x9, x8
	mul	x9, x8, x10
	umulh	x8, x8, x10
	eor	x0, x8, x9
	ret
LBB1_9:
	cmp	x1, #81
	b.lo	LBB1_13
; %bb.10:
	mov	x11, #30885
	movk	x11, #43628, lsl #16
	movk	x11, #36300, lsl #32
	movk	x11, #11573, lsl #48
	mov	x12, #43591
	movk	x12, #7649, lsl #16
	movk	x12, #11685, lsl #32
	movk	x12, #19802, lsl #48
	mov	x13, #25647
	movk	x13, #30909, lsl #16
	movk	x13, #7524, lsl #32
	movk	x13, #41078, lsl #48
	mov	x14, x9
	mov	x15, x9
	mov	x16, x9
	mov	x17, x9
LBB1_11:                                ; =>This Inner Loop Header: Depth=1
	ldp	x2, x3, [x0]
	eor	x2, x2, x11
	eor	x9, x3, x9
	mul	x3, x9, x2
	umulh	x9, x9, x2
	eor	x9, x9, x3
	ldp	x2, x3, [x0, #16]
	eor	x2, x2, x8
	eor	x14, x3, x14
	mul	x3, x14, x2
	umulh	x14, x14, x2
	eor	x14, x14, x3
	ldp	x2, x3, [x0, #32]
	eor	x2, x2, x10
	eor	x15, x3, x15
	mul	x3, x15, x2
	umulh	x15, x15, x2
	eor	x15, x15, x3
	ldp	x2, x3, [x0, #48]
	eor	x2, x2, x12
	eor	x16, x3, x16
	mul	x3, x16, x2
	umulh	x16, x16, x2
	eor	x16, x16, x3
	ldp	x2, x3, [x0, #64]
	eor	x2, x2, x13
	eor	x17, x3, x17
	mul	x3, x17, x2
	umulh	x17, x17, x2
	eor	x17, x17, x3
	add	x0, x0, #80
	sub	x1, x1, #80
	cmp	x1, #80
	b.hi	LBB1_11
; %bb.12:
	eor	x9, x14, x9
	eor	x9, x9, x15
	eor	x9, x9, x16
	eor	x9, x9, x17
	cmp	x1, #17
	b.lo	LBB1_17
LBB1_13:
	ldp	x11, x12, [x0]
	eor	x11, x11, x10
	eor	x9, x12, x9
	mul	x12, x9, x11
	umulh	x9, x9, x11
	eor	x9, x9, x12
	cmp	x1, #33
	b.lo	LBB1_17
; %bb.14:
	ldp	x11, x12, [x0, #16]
	eor	x10, x11, x10
	eor	x9, x12, x9
	mul	x11, x9, x10
	umulh	x9, x9, x10
	eor	x9, x9, x11
	cmp	x1, #49
	b.lo	LBB1_17
; %bb.15:
	ldp	x10, x11, [x0, #32]
	eor	x10, x10, x8
	eor	x9, x11, x9
	mul	x11, x9, x10
	umulh	x9, x9, x10
	eor	x9, x9, x11
	cmp	x1, #65
	b.lo	LBB1_17
; %bb.16:
	ldp	x10, x11, [x0, #48]
	eor	x10, x10, x8
	eor	x9, x11, x9
	mul	x11, x9, x10
	umulh	x9, x9, x10
	eor	x9, x9, x11
LBB1_17:
	add	x10, x0, x1
	ldp	x12, x11, [x10, #-16]
	eor	x10, x12, x1
	b	LBB1_8
	.cfi_endproc
                                        ; -- End function
	.globl	_rapidhash_bulk_wrapper         ; -- Begin function rapidhash_bulk_wrapper
	.p2align	2
_rapidhash_bulk_wrapper:                ; @rapidhash_bulk_wrapper
	.cfi_startproc
; %bb.0:
	mov	x8, #44233
	movk	x8, #38446, lsl #16
	movk	x8, #19347, lsl #32
	movk	x8, #35768, lsl #48
	mov	x10, #54435
	movk	x10, #54323, lsl #16
	movk	x10, #42542, lsl #32
	movk	x10, #19251, lsl #48
	eor	x9, x2, x10
	mul	x11, x9, x8
	umulh	x9, x9, x8
	eor	x9, x9, x11
	eor	x9, x9, x2
	cmp	x1, #16
	b.hi	LBB2_9
; %bb.1:
	cmp	x1, #4
	b.lo	LBB2_4
; %bb.2:
	eor	x9, x9, x1
	add	x11, x0, x1
	cmp	x1, #8
	b.lo	LBB2_6
; %bb.3:
	ldr	x10, [x0]
	ldur	x11, [x11, #-8]
	b	LBB2_8
LBB2_4:
	cbz	x1, LBB2_7
; %bb.5:
	ldrb	w11, [x0]
	add	x10, x0, x1
	ldurb	w10, [x10, #-1]
	bfi	x10, x11, #45, #8
	lsr	x11, x1, #1
	ldrb	w11, [x0, x11]
	b	LBB2_8
LBB2_6:
	ldr	w10, [x0]
	ldur	w11, [x11, #-4]
	b	LBB2_8
LBB2_7:
	mov	x10, #0
	mov	x11, #0
LBB2_8:
	eor	x10, x10, x8
	eor	x9, x9, x11
	mul	x11, x9, x10
	umulh	x9, x9, x10
	eor	x10, x11, #0xaaaaaaaaaaaaaaaa
	eor	x9, x1, x9
	eor	x8, x9, x8
	mul	x9, x8, x10
	umulh	x8, x8, x10
	eor	x0, x8, x9
	ret
LBB2_9:
	cmp	x1, #113
	b.lo	LBB2_13
; %bb.10:
	mov	x11, #30885
	movk	x11, #43628, lsl #16
	movk	x11, #36300, lsl #32
	movk	x11, #11573, lsl #48
	mov	x12, #43591
	movk	x12, #7649, lsl #16
	movk	x12, #11685, lsl #32
	movk	x12, #19802, lsl #48
	mov	x13, #25647
	movk	x13, #30909, lsl #16
	movk	x13, #7524, lsl #32
	movk	x13, #41078, lsl #48
	mov	x14, #10459
	movk	x14, #41140, lsl #16
	movk	x14, #32465, lsl #32
	movk	x14, #59139, lsl #48
	mov	x15, #14476
	movk	x15, #10268, lsl #16
	movk	x15, #5989, lsl #32
	movk	x15, #37101, lsl #48
	mov	x16, x9
	mov	x17, x9
	mov	x2, x9
	mov	x3, x9
	mov	x4, x9
	mov	x5, x9
LBB2_11:                                ; =>This Inner Loop Header: Depth=1
	ldp	x6, x7, [x0]
	eor	x6, x6, x11
	eor	x9, x7, x9
	mul	x7, x9, x6
	umulh	x9, x9, x6
	eor	x9, x9, x7
	ldp	x6, x7, [x0, #16]
	eor	x6, x6, x8
	eor	x16, x7, x16
	mul	x7, x16, x6
	umulh	x16, x16, x6
	eor	x16, x16, x7
	ldp	x6, x7, [x0, #32]
	eor	x6, x6, x10
	eor	x17, x7, x17
	mul	x7, x17, x6
	umulh	x17, x17, x6
	eor	x17, x17, x7
	ldp	x6, x7, [x0, #48]
	eor	x6, x6, x12
	eor	x2, x7, x2
	mul	x7, x2, x6
	umulh	x2, x2, x6
	eor	x2, x2, x7
	ldp	x6, x7, [x0, #64]
	eor	x6, x6, x13
	eor	x3, x7, x3
	mul	x7, x3, x6
	umulh	x3, x3, x6
	eor	x3, x3, x7
	ldp	x6, x7, [x0, #80]
	eor	x6, x6, x14
	eor	x4, x7, x4
	mul	x7, x4, x6
	umulh	x4, x4, x6
	eor	x4, x4, x7
	ldp	x6, x7, [x0, #96]
	eor	x6, x6, x15
	eor	x5, x7, x5
	mul	x7, x5, x6
	umulh	x5, x5, x6
	eor	x5, x5, x7
	add	x0, x0, #112
	sub	x1, x1, #112
	cmp	x1, #112
	b.hi	LBB2_11
; %bb.12:
	eor	x9, x16, x9
	eor	x9, x9, x17
	eor	x9, x9, x2
	eor	x9, x9, x3
	eor	x9, x9, x4
	eor	x9, x9, x5
	cmp	x1, #17
	b.lo	LBB2_19
LBB2_13:
	ldp	x11, x12, [x0]
	eor	x11, x11, x10
	eor	x9, x12, x9
	mul	x12, x9, x11
	umulh	x9, x9, x11
	eor	x9, x9, x12
	cmp	x1, #33
	b.lo	LBB2_19
; %bb.14:
	ldp	x11, x12, [x0, #16]
	eor	x11, x11, x10
	eor	x9, x12, x9
	mul	x12, x9, x11
	umulh	x9, x9, x11
	eor	x9, x9, x12
	cmp	x1, #49
	b.lo	LBB2_19
; %bb.15:
	ldp	x11, x12, [x0, #32]
	eor	x11, x11, x8
	eor	x9, x12, x9
	mul	x12, x9, x11
	umulh	x9, x9, x11
	eor	x9, x9, x12
	cmp	x1, #65
	b.lo	LBB2_19
; %bb.16:
	ldp	x11, x12, [x0, #48]
	eor	x11, x11, x8
	eor	x9, x12, x9
	mul	x12, x9, x11
	umulh	x9, x9, x11
	eor	x9, x9, x12
	cmp	x1, #81
	b.lo	LBB2_19
; %bb.17:
	ldp	x11, x12, [x0, #64]
	eor	x10, x11, x10
	eor	x9, x12, x9
	mul	x11, x9, x10
	umulh	x9, x9, x10
	eor	x9, x9, x11
	cmp	x1, #97
	b.lo	LBB2_19
; %bb.18:
	ldp	x10, x11, [x0, #80]
	eor	x10, x10, x8
	eor	x9, x11, x9
	mul	x11, x9, x10
	umulh	x9, x9, x10
	eor	x9, x9, x11
LBB2_19:
	add	x10, x0, x1
	ldp	x12, x11, [x10, #-16]
	eor	x10, x12, x1
	b	LBB2_8
	.cfi_endproc
                                        ; -- End function
.subsections_via_symbols
