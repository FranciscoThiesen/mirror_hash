	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 15, 0	sdk_version 15, 4
	.section	__TEXT,__literal16,16byte_literals
	.p2align	4, 0x0                          ; -- Begin function _ZN11mirror_hash6detail15hash_aes_mediumEPKvmy
lCPI0_0:
	.byte	45                              ; 0x2d
	.byte	53                              ; 0x35
	.byte	141                             ; 0x8d
	.byte	204                             ; 0xcc
	.byte	170                             ; 0xaa
	.byte	108                             ; 0x6c
	.byte	120                             ; 0x78
	.byte	165                             ; 0xa5
	.byte	139                             ; 0x8b
	.byte	184                             ; 0xb8
	.byte	75                              ; 0x4b
	.byte	147                             ; 0x93
	.byte	150                             ; 0x96
	.byte	46                              ; 0x2e
	.byte	172                             ; 0xac
	.byte	201                             ; 0xc9
lCPI0_1:
	.byte	75                              ; 0x4b
	.byte	51                              ; 0x33
	.byte	166                             ; 0xa6
	.byte	46                              ; 0x2e
	.byte	212                             ; 0xd4
	.byte	51                              ; 0x33
	.byte	212                             ; 0xd4
	.byte	163                             ; 0xa3
	.byte	77                              ; 0x4d
	.byte	90                              ; 0x5a
	.byte	45                              ; 0x2d
	.byte	165                             ; 0xa5
	.byte	29                              ; 0x1d
	.byte	225                             ; 0xe1
	.byte	170                             ; 0xaa
	.byte	71                              ; 0x47
lCPI0_2:
	.byte	160                             ; 0xa0
	.byte	118                             ; 0x76
	.byte	29                              ; 0x1d
	.byte	100                             ; 0x64
	.byte	120                             ; 0x78
	.byte	189                             ; 0xbd
	.byte	100                             ; 0x64
	.byte	47                              ; 0x2f
	.byte	231                             ; 0xe7
	.byte	3                               ; 0x3
	.byte	126                             ; 0x7e
	.byte	209                             ; 0xd1
	.byte	160                             ; 0xa0
	.byte	180                             ; 0xb4
	.byte	40                              ; 0x28
	.byte	219                             ; 0xdb
lCPI0_3:
	.byte	144                             ; 0x90
	.byte	237                             ; 0xed
	.byte	23                              ; 0x17
	.byte	101                             ; 0x65
	.byte	40                              ; 0x28
	.byte	28                              ; 0x1c
	.byte	56                              ; 0x38
	.byte	140                             ; 0x8c
	.byte	170                             ; 0xaa
	.byte	170                             ; 0xaa
	.byte	170                             ; 0xaa
	.byte	170                             ; 0xaa
	.byte	170                             ; 0xaa
	.byte	170                             ; 0xaa
	.byte	170                             ; 0xaa
	.byte	170                             ; 0xaa
	.section	__TEXT,__text,regular,pure_instructions
	.globl	__ZN11mirror_hash6detail15hash_aes_mediumEPKvmy
	.p2align	2
__ZN11mirror_hash6detail15hash_aes_mediumEPKvmy: ; @_ZN11mirror_hash6detail15hash_aes_mediumEPKvmy
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #160
	.cfi_def_cfa_offset 160
	stp	x20, x19, [sp, #128]            ; 16-byte Folded Spill
	stp	x29, x30, [sp, #144]            ; 16-byte Folded Spill
	add	x29, sp, #144
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	mov	x19, x1
	mov	x1, x0
	dup.2d	v16, x2
Lloh0:
	adrp	x8, lCPI0_0@PAGE
Lloh1:
	ldr	q7, [x8, lCPI0_0@PAGEOFF]
	eor.16b	v17, v16, v7
Lloh2:
	adrp	x8, lCPI0_1@PAGE
Lloh3:
	ldr	q6, [x8, lCPI0_1@PAGEOFF]
	eor.16b	v18, v16, v6
Lloh4:
	adrp	x8, lCPI0_2@PAGE
Lloh5:
	ldr	q5, [x8, lCPI0_2@PAGEOFF]
	eor.16b	v19, v16, v5
	cmp	x19, #64
	b.lo	LBB0_3
; %bb.1:
Lloh6:
	adrp	x8, lCPI0_3@PAGE
Lloh7:
	ldr	q0, [x8, lCPI0_3@PAGEOFF]
LBB0_2:                                 ; =>This Inner Loop Header: Depth=1
	ldp	q1, q2, [x1]
	ldp	q3, q4, [x1, #32]
	eor.16b	v16, v1, v16
	eor.16b	v17, v2, v17
	eor.16b	v18, v3, v18
	eor.16b	v19, v4, v19
	aese.16b	v16, v7
	aesmc.16b	v16, v16
	aese.16b	v17, v6
	aesmc.16b	v17, v17
	aese.16b	v18, v5
	aesmc.16b	v18, v18
	aese.16b	v19, v0
	aesmc.16b	v19, v19
	add	x1, x1, #64
	sub	x19, x19, #64
	cmp	x19, #63
	b.hi	LBB0_2
LBB0_3:
	cmp	x19, #16
	b.lo	LBB0_5
LBB0_4:                                 ; =>This Inner Loop Header: Depth=1
	ldr	q0, [x1], #16
	eor.16b	v16, v0, v16
	aese.16b	v16, v7
	aesmc.16b	v16, v16
	sub	x19, x19, #16
	cmp	x19, #15
	b.hi	LBB0_4
LBB0_5:
	cbz	x19, LBB0_7
; %bb.6:
	stp	xzr, xzr, [x29, #-32]
	sub	x0, x29, #32
	mov	x2, x19
	stp	q6, q5, [x29, #-64]             ; 32-byte Folded Spill
	stp	q16, q7, [sp, #48]              ; 32-byte Folded Spill
	stp	q18, q17, [sp, #16]             ; 32-byte Folded Spill
	str	q19, [sp]                       ; 16-byte Folded Spill
	bl	_memcpy
	ldp	q19, q18, [sp]                  ; 32-byte Folded Reload
	ldp	q17, q16, [sp, #32]             ; 32-byte Folded Reload
	ldr	q7, [sp, #64]                   ; 16-byte Folded Reload
	ldp	q6, q5, [x29, #-64]             ; 32-byte Folded Reload
	sturb	w19, [x29, #-17]
	ldur	q0, [x29, #-32]
	eor.16b	v16, v0, v16
LBB0_7:
	eor.16b	v0, v18, v19
	eor3.16b	v0, v0, v17, v16
	aese.16b	v0, v7
	aesmc.16b	v0, v0
	aese.16b	v0, v6
	aesmc.16b	v0, v0
	aese.16b	v0, v5
	aesmc.16b	v0, v0
	dup.2d	v1, v0[1]
	eor.16b	v0, v1, v0
	fmov	x0, d0
	ldp	x29, x30, [sp, #144]            ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #128]            ; 16-byte Folded Reload
	add	sp, sp, #160
	ret
	.loh AdrpLdr	Lloh4, Lloh5
	.loh AdrpAdrp	Lloh2, Lloh4
	.loh AdrpLdr	Lloh2, Lloh3
	.loh AdrpAdrp	Lloh0, Lloh2
	.loh AdrpLdr	Lloh0, Lloh1
	.loh AdrpLdr	Lloh6, Lloh7
	.cfi_endproc
                                        ; -- End function
	.section	__TEXT,__literal16,16byte_literals
	.p2align	4, 0x0                          ; -- Begin function _ZN11mirror_hash6detail13hash_aes_bulkEPKvmy
lCPI1_0:
	.byte	45                              ; 0x2d
	.byte	53                              ; 0x35
	.byte	141                             ; 0x8d
	.byte	204                             ; 0xcc
	.byte	170                             ; 0xaa
	.byte	108                             ; 0x6c
	.byte	120                             ; 0x78
	.byte	165                             ; 0xa5
	.byte	139                             ; 0x8b
	.byte	184                             ; 0xb8
	.byte	75                              ; 0x4b
	.byte	147                             ; 0x93
	.byte	150                             ; 0x96
	.byte	46                              ; 0x2e
	.byte	172                             ; 0xac
	.byte	201                             ; 0xc9
lCPI1_1:
	.byte	75                              ; 0x4b
	.byte	51                              ; 0x33
	.byte	166                             ; 0xa6
	.byte	46                              ; 0x2e
	.byte	212                             ; 0xd4
	.byte	51                              ; 0x33
	.byte	212                             ; 0xd4
	.byte	163                             ; 0xa3
	.byte	77                              ; 0x4d
	.byte	90                              ; 0x5a
	.byte	45                              ; 0x2d
	.byte	165                             ; 0xa5
	.byte	29                              ; 0x1d
	.byte	225                             ; 0xe1
	.byte	170                             ; 0xaa
	.byte	71                              ; 0x47
lCPI1_2:
	.byte	160                             ; 0xa0
	.byte	118                             ; 0x76
	.byte	29                              ; 0x1d
	.byte	100                             ; 0x64
	.byte	120                             ; 0x78
	.byte	189                             ; 0xbd
	.byte	100                             ; 0x64
	.byte	47                              ; 0x2f
	.byte	231                             ; 0xe7
	.byte	3                               ; 0x3
	.byte	126                             ; 0x7e
	.byte	209                             ; 0xd1
	.byte	160                             ; 0xa0
	.byte	180                             ; 0xb4
	.byte	40                              ; 0x28
	.byte	219                             ; 0xdb
lCPI1_3:
	.byte	144                             ; 0x90
	.byte	237                             ; 0xed
	.byte	23                              ; 0x17
	.byte	101                             ; 0x65
	.byte	40                              ; 0x28
	.byte	28                              ; 0x1c
	.byte	56                              ; 0x38
	.byte	140                             ; 0x8c
	.byte	170                             ; 0xaa
	.byte	170                             ; 0xaa
	.byte	170                             ; 0xaa
	.byte	170                             ; 0xaa
	.byte	170                             ; 0xaa
	.byte	170                             ; 0xaa
	.byte	170                             ; 0xaa
	.byte	170                             ; 0xaa
	.section	__TEXT,__text,regular,pure_instructions
	.globl	__ZN11mirror_hash6detail13hash_aes_bulkEPKvmy
	.p2align	2
__ZN11mirror_hash6detail13hash_aes_bulkEPKvmy: ; @_ZN11mirror_hash6detail13hash_aes_bulkEPKvmy
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #80
	.cfi_def_cfa_offset 80
	stp	x22, x21, [sp, #32]             ; 16-byte Folded Spill
	stp	x20, x19, [sp, #48]             ; 16-byte Folded Spill
	stp	x29, x30, [sp, #64]             ; 16-byte Folded Spill
	add	x29, sp, #64
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	.cfi_offset w21, -40
	.cfi_offset w22, -48
	mov	x19, x1
	mov	x1, x0
	dup.2d	v19, x2
	adrp	x21, lCPI1_0@PAGE
	adrp	x20, lCPI1_1@PAGE
	cmp	x19, #128
	b.lo	LBB1_3
; %bb.1:
	ldr	q0, [x21, lCPI1_0@PAGEOFF]
	ldr	q1, [x20, lCPI1_1@PAGEOFF]
	movi.2d	v2, #0000000000000000
LBB1_2:                                 ; =>This Inner Loop Header: Depth=1
	ldp	q3, q4, [x1]
	ldp	q5, q6, [x1, #32]
	ldp	q7, q16, [x1, #64]
	ldp	q17, q18, [x1, #96]
	aese.16b	v3, v4
	aesmc.16b	v3, v3
	aese.16b	v3, v5
	aesmc.16b	v3, v3
	aese.16b	v3, v6
	aesmc.16b	v3, v3
	aese.16b	v3, v7
	aesmc.16b	v3, v3
	aese.16b	v3, v16
	aesmc.16b	v3, v3
	aese.16b	v3, v17
	aesmc.16b	v3, v3
	aese.16b	v3, v18
	aesmc.16b	v3, v3
	aese.16b	v3, v0
	aesmc.16b	v3, v3
	aese.16b	v3, v1
	aesmc.16b	v3, v3
	aese.16b	v19, v2
	eor.16b	v19, v19, v3
	add	x1, x1, #128
	sub	x19, x19, #128
	cmp	x19, #127
	b.hi	LBB1_2
LBB1_3:
	subs	x8, x19, #64
	b.lo	LBB1_5
; %bb.4:
	ldp	q0, q1, [x1]
	ldp	q2, q3, [x1, #32]
	aese.16b	v0, v1
	aesmc.16b	v0, v0
	aese.16b	v0, v2
	aesmc.16b	v0, v0
	aese.16b	v0, v3
	aesmc.16b	v0, v0
	ldr	q1, [x21, lCPI1_0@PAGEOFF]
	aese.16b	v0, v1
	aesmc.16b	v0, v0
	ldr	q1, [x20, lCPI1_1@PAGEOFF]
	aese.16b	v0, v1
	aesmc.16b	v0, v0
	movi.2d	v1, #0000000000000000
	aese.16b	v19, v1
	eor.16b	v19, v19, v0
	add	x1, x1, #64
	mov	x19, x8
LBB1_5:
	cmp	x19, #16
	b.lo	LBB1_8
; %bb.6:
	ldr	q0, [x21, lCPI1_0@PAGEOFF]
LBB1_7:                                 ; =>This Inner Loop Header: Depth=1
	ldr	q1, [x1], #16
	eor.16b	v19, v1, v19
	aese.16b	v19, v0
	aesmc.16b	v19, v19
	sub	x19, x19, #16
	cmp	x19, #15
	b.hi	LBB1_7
LBB1_8:
	cbz	x19, LBB1_10
; %bb.9:
	stp	xzr, xzr, [sp, #16]
	add	x0, sp, #16
	mov	x2, x19
	str	q19, [sp]                       ; 16-byte Folded Spill
	bl	_memcpy
	strb	w19, [sp, #31]
	ldp	q19, q0, [sp]                   ; 16-byte Folded Reload
	eor.16b	v19, v0, v19
LBB1_10:
	ldr	q0, [x21, lCPI1_0@PAGEOFF]
	aese.16b	v19, v0
	aesmc.16b	v19, v19
	ldr	q0, [x20, lCPI1_1@PAGEOFF]
	aese.16b	v19, v0
	aesmc.16b	v19, v19
Lloh8:
	adrp	x8, lCPI1_2@PAGE
Lloh9:
	ldr	q0, [x8, lCPI1_2@PAGEOFF]
	aese.16b	v19, v0
	aesmc.16b	v19, v19
Lloh10:
	adrp	x8, lCPI1_3@PAGE
Lloh11:
	ldr	q0, [x8, lCPI1_3@PAGEOFF]
	aese.16b	v19, v0
	aesmc.16b	v19, v19
	dup.2d	v0, v19[1]
	eor.16b	v0, v0, v19
	fmov	x0, d0
	ldp	x29, x30, [sp, #64]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #48]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #32]             ; 16-byte Folded Reload
	add	sp, sp, #80
	ret
	.loh AdrpLdr	Lloh10, Lloh11
	.loh AdrpAdrp	Lloh8, Lloh10
	.loh AdrpLdr	Lloh8, Lloh9
	.cfi_endproc
                                        ; -- End function
	.globl	__Z6escapey                     ; -- Begin function _Z6escapey
	.p2align	2
__Z6escapey:                            ; @_Z6escapey
	.cfi_startproc
; %bb.0:
	; InlineAsm Start
	; InlineAsm End
	ret
	.cfi_endproc
                                        ; -- End function
	.globl	__Z7clobberv                    ; -- Begin function _Z7clobberv
	.p2align	2
__Z7clobberv:                           ; @_Z7clobberv
	.cfi_startproc
; %bb.0:
	; InlineAsm Start
	; InlineAsm End
	ret
	.cfi_endproc
                                        ; -- End function
	.globl	__Z17bench_mirror_hashPKvmm     ; -- Begin function _Z17bench_mirror_hashPKvmm
	.p2align	2
__Z17bench_mirror_hashPKvmm:            ; @_Z17bench_mirror_hashPKvmm
	.cfi_startproc
; %bb.0:
	stp	x28, x27, [sp, #-96]!           ; 16-byte Folded Spill
	.cfi_def_cfa_offset 96
	stp	x26, x25, [sp, #16]             ; 16-byte Folded Spill
	stp	x24, x23, [sp, #32]             ; 16-byte Folded Spill
	stp	x22, x21, [sp, #48]             ; 16-byte Folded Spill
	stp	x20, x19, [sp, #64]             ; 16-byte Folded Spill
	stp	x29, x30, [sp, #80]             ; 16-byte Folded Spill
	add	x29, sp, #80
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	.cfi_offset w21, -40
	.cfi_offset w22, -48
	.cfi_offset w23, -56
	.cfi_offset w24, -64
	.cfi_offset w25, -72
	.cfi_offset w26, -80
	.cfi_offset w27, -88
	.cfi_offset w28, -96
	cbz	x2, LBB4_8
; %bb.1:
	mov	x19, x2
	mov	x22, x1
	mov	x21, x0
	mov	x8, #44233
	movk	x8, #38446, lsl #16
	movk	x8, #19347, lsl #32
	movk	x8, #35768, lsl #48
	add	x14, x0, x1
	cmp	x1, #16
	b.hi	LBB4_9
; %bb.2:
	eor	x9, x22, x8
	cmp	x22, #4
	b.lo	LBB4_13
; %bb.3:
	mov	x10, #0
	mov	x20, #0
	mov	x11, #44233
	movk	x11, #38446, lsl #16
	movk	x11, #19347, lsl #32
	movk	x11, #35768, lsl #48
	mov	x12, #54435
	movk	x12, #54323, lsl #16
	movk	x12, #42542, lsl #32
	movk	x12, #19251, lsl #48
	b	LBB4_6
LBB4_4:                                 ;   in Loop: Header=BB4_6 Depth=1
	ldr	x13, [x21]
	ldur	x15, [x14, #-8]
LBB4_5:                                 ;   in Loop: Header=BB4_6 Depth=1
	eor	x16, x10, x12
	mul	x17, x16, x11
	umulh	x16, x16, x11
	eor	x16, x16, x17
	eor	x13, x13, x8
	eor	x15, x15, x16
	eor	x15, x15, x10
	eor	x15, x15, x22
	mul	x16, x15, x13
	umulh	x13, x15, x13
	eor	x15, x16, #0xaaaaaaaaaaaaaaaa
	eor	x13, x9, x13
	mul	x16, x13, x15
	umulh	x13, x13, x15
	eor	x13, x13, x16
	add	x20, x20, x13
	add	x10, x10, #1
	cmp	x19, x10
	b.eq	LBB4_31
LBB4_6:                                 ; =>This Inner Loop Header: Depth=1
	cmp	x22, #7
	b.hi	LBB4_4
; %bb.7:                                ;   in Loop: Header=BB4_6 Depth=1
	ldr	w13, [x21]
	ldur	w15, [x14, #-4]
	b	LBB4_5
LBB4_8:
	mov	x20, #0
	b	LBB4_31
LBB4_9:
	cmp	x22, #2, lsl #12                ; =8192
	b.hi	LBB4_16
; %bb.10:
	cmp	x22, #129
	b.hs	LBB4_27
; %bb.11:
	mov	x23, #0
	mov	x20, #0
LBB4_12:                                ; =>This Inner Loop Header: Depth=1
	mov	x0, x21
	mov	x1, x22
	mov	x2, x23
	bl	__ZN11mirror_hash6detail15hash_aes_mediumEPKvmy
	add	x20, x0, x20
	add	x23, x23, #1
	cmp	x19, x23
	b.ne	LBB4_12
	b	LBB4_31
LBB4_13:
	cbz	x22, LBB4_29
; %bb.14:
	mov	x10, #0
	mov	x20, #0
	lsr	x13, x22, #1
	mov	x11, #44233
	movk	x11, #38446, lsl #16
	movk	x11, #19347, lsl #32
	movk	x11, #35768, lsl #48
	mov	x12, #54435
	movk	x12, #54323, lsl #16
	movk	x12, #42542, lsl #32
	movk	x12, #19251, lsl #48
	ldrb	w15, [x21]
	ldurb	w14, [x14, #-1]
	bfi	x14, x15, #45, #8
	ldrb	w13, [x21, x13]
	eor	x8, x14, x8
LBB4_15:                                ; =>This Inner Loop Header: Depth=1
	eor	x14, x10, x12
	mul	x15, x14, x11
	umulh	x14, x14, x11
	eor	x14, x14, x15
	eor	x14, x14, x13
	eor	x14, x14, x10
	mul	x15, x14, x8
	umulh	x14, x14, x8
	eor	x15, x15, #0xaaaaaaaaaaaaaaaa
	eor	x14, x9, x14
	mul	x16, x14, x15
	umulh	x14, x14, x15
	eor	x14, x14, x16
	add	x20, x20, x14
	add	x10, x10, #1
	cmp	x19, x10
	b.ne	LBB4_15
	b	LBB4_31
LBB4_16:
	mov	x9, #0
	mov	x20, #0
	mov	x10, #44233
	movk	x10, #38446, lsl #16
	movk	x10, #19347, lsl #32
	movk	x10, #35768, lsl #48
	mov	x11, #54435
	movk	x11, #54323, lsl #16
	movk	x11, #42542, lsl #32
	movk	x11, #19251, lsl #48
	sub	x12, x22, #113
	lsr	x12, x12, #4
	mov	x13, #9363
	movk	x13, #37449, lsl #16
	movk	x13, #18724, lsl #32
	movk	x13, #9362, lsl #48
	umulh	x12, x12, x13
	mov	w13, #112
	madd	x12, x12, x13, x21
	add	x12, x12, #113
	ldp	x13, x14, [x14, #-16]
	mov	x15, #30885
	movk	x15, #43628, lsl #16
	movk	x15, #36300, lsl #32
	movk	x15, #11573, lsl #48
	mov	x16, #43591
	movk	x16, #7649, lsl #16
	movk	x16, #11685, lsl #32
	movk	x16, #19802, lsl #48
	mov	x17, #25647
	movk	x17, #30909, lsl #16
	movk	x17, #7524, lsl #32
	movk	x17, #41078, lsl #48
	mov	x0, #10459
	movk	x0, #41140, lsl #16
	movk	x0, #32465, lsl #32
	movk	x0, #59139, lsl #48
	mov	x1, #14476
	movk	x1, #10268, lsl #16
	movk	x1, #5989, lsl #32
	movk	x1, #37101, lsl #48
	b	LBB4_18
LBB4_17:                                ;   in Loop: Header=BB4_18 Depth=1
	eor	x3, x2, x13
	eor	x3, x3, x8
	eor	x4, x14, x4
	mul	x5, x4, x3
	umulh	x3, x4, x3
	eor	x4, x5, #0xaaaaaaaaaaaaaaaa
	eor	x2, x2, x3
	eor	x2, x2, x8
	mul	x3, x2, x4
	umulh	x2, x2, x4
	eor	x2, x2, x3
	add	x20, x20, x2
	add	x9, x9, #1
	cmp	x9, x19
	b.eq	LBB4_31
LBB4_18:                                ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB4_19 Depth 2
	eor	x2, x9, x11
	mul	x3, x2, x10
	umulh	x2, x2, x10
	eor	x2, x2, x3
	eor	x4, x9, x2
	mov	x2, x22
	mov	x3, x21
	mov	x5, x4
	mov	x6, x4
	mov	x7, x4
	mov	x23, x4
	mov	x24, x4
	mov	x25, x4
LBB4_19:                                ;   Parent Loop BB4_18 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldp	x26, x27, [x3]
	eor	x26, x26, x15
	eor	x4, x27, x4
	mul	x27, x4, x26
	umulh	x4, x4, x26
	eor	x4, x4, x27
	ldp	x26, x27, [x3, #16]
	eor	x26, x26, x8
	eor	x5, x27, x5
	mul	x27, x5, x26
	umulh	x5, x5, x26
	eor	x5, x5, x27
	ldp	x26, x27, [x3, #32]
	eor	x26, x26, x11
	eor	x6, x27, x6
	mul	x27, x6, x26
	umulh	x6, x6, x26
	eor	x6, x6, x27
	ldp	x26, x27, [x3, #48]
	eor	x26, x26, x16
	eor	x7, x27, x7
	mul	x27, x7, x26
	umulh	x7, x7, x26
	eor	x7, x7, x27
	ldp	x26, x27, [x3, #64]
	eor	x26, x26, x17
	eor	x23, x27, x23
	mul	x27, x23, x26
	umulh	x23, x23, x26
	eor	x23, x23, x27
	ldp	x26, x27, [x3, #80]
	eor	x26, x26, x0
	eor	x24, x27, x24
	mul	x27, x24, x26
	umulh	x24, x24, x26
	eor	x24, x24, x27
	ldp	x26, x27, [x3, #96]
	eor	x26, x26, x1
	eor	x25, x27, x25
	mul	x27, x25, x26
	umulh	x25, x25, x26
	eor	x25, x25, x27
	add	x3, x3, #112
	sub	x2, x2, #112
	cmp	x2, #112
	b.hi	LBB4_19
; %bb.20:                               ;   in Loop: Header=BB4_18 Depth=1
	eor	x4, x5, x4
	eor	x4, x4, x6
	eor	x4, x4, x7
	eor	x4, x4, x23
	eor	x4, x4, x24
	eor	x4, x4, x25
	cmp	x2, #17
	b.lo	LBB4_17
; %bb.21:                               ;   in Loop: Header=BB4_18 Depth=1
	ldr	x3, [x3]
	eor	x3, x3, x11
	ldur	x5, [x12, #7]
	eor	x4, x5, x4
	mul	x5, x4, x3
	umulh	x3, x4, x3
	eor	x4, x3, x5
	cmp	x2, #33
	b.lo	LBB4_17
; %bb.22:                               ;   in Loop: Header=BB4_18 Depth=1
	ldur	x3, [x12, #15]
	eor	x3, x3, x11
	ldur	x5, [x12, #23]
	eor	x4, x5, x4
	mul	x5, x4, x3
	umulh	x3, x4, x3
	eor	x4, x3, x5
	cmp	x2, #49
	b.lo	LBB4_17
; %bb.23:                               ;   in Loop: Header=BB4_18 Depth=1
	ldur	x3, [x12, #31]
	eor	x3, x3, x8
	ldur	x5, [x12, #39]
	eor	x4, x5, x4
	mul	x5, x4, x3
	umulh	x3, x4, x3
	eor	x4, x3, x5
	cmp	x2, #65
	b.lo	LBB4_17
; %bb.24:                               ;   in Loop: Header=BB4_18 Depth=1
	ldur	x3, [x12, #47]
	eor	x3, x3, x8
	ldur	x5, [x12, #55]
	eor	x4, x5, x4
	mul	x5, x4, x3
	umulh	x3, x4, x3
	eor	x4, x3, x5
	cmp	x2, #81
	b.lo	LBB4_17
; %bb.25:                               ;   in Loop: Header=BB4_18 Depth=1
	ldur	x3, [x12, #63]
	eor	x3, x3, x11
	ldur	x5, [x12, #71]
	eor	x4, x5, x4
	mul	x5, x4, x3
	umulh	x3, x4, x3
	eor	x4, x3, x5
	cmp	x2, #97
	b.lo	LBB4_17
; %bb.26:                               ;   in Loop: Header=BB4_18 Depth=1
	ldur	x3, [x12, #79]
	eor	x3, x3, x8
	ldur	x5, [x12, #87]
	eor	x4, x5, x4
	mul	x5, x4, x3
	umulh	x3, x4, x3
	eor	x4, x3, x5
	b	LBB4_17
LBB4_27:
	mov	x23, #0
	mov	x20, #0
LBB4_28:                                ; =>This Inner Loop Header: Depth=1
	mov	x0, x21
	mov	x1, x22
	mov	x2, x23
	bl	__ZN11mirror_hash6detail13hash_aes_bulkEPKvmy
	add	x20, x0, x20
	add	x23, x23, #1
	cmp	x19, x23
	b.ne	LBB4_28
	b	LBB4_31
LBB4_29:
	mov	x8, #0
	mov	x20, #0
	mov	x10, #44233
	movk	x10, #38446, lsl #16
	movk	x10, #19347, lsl #32
	movk	x10, #35768, lsl #48
	mov	x11, #54435
	movk	x11, #54323, lsl #16
	movk	x11, #42542, lsl #32
	movk	x11, #19251, lsl #48
LBB4_30:                                ; =>This Inner Loop Header: Depth=1
	eor	x12, x8, x11
	mul	x13, x12, x10
	umulh	x12, x12, x10
	eor	x12, x12, x13
	eor	x12, x8, x12
	umulh	x13, x12, x10
	mul	x12, x12, x10
	eor	x12, x12, #0xaaaaaaaaaaaaaaaa
	eor	x13, x9, x13
	mul	x14, x13, x12
	umulh	x12, x13, x12
	eor	x12, x12, x14
	add	x20, x20, x12
	add	x8, x8, #1
	cmp	x19, x8
	b.ne	LBB4_30
LBB4_31:
	mov	x0, x20
	ldp	x29, x30, [sp, #80]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #64]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #48]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp, #32]             ; 16-byte Folded Reload
	ldp	x26, x25, [sp, #16]             ; 16-byte Folded Reload
	ldp	x28, x27, [sp], #96             ; 16-byte Folded Reload
	ret
	.cfi_endproc
                                        ; -- End function
	.globl	__Z15bench_rapidhashPKvmm       ; -- Begin function _Z15bench_rapidhashPKvmm
	.p2align	2
__Z15bench_rapidhashPKvmm:              ; @_Z15bench_rapidhashPKvmm
	.cfi_startproc
; %bb.0:
	stp	x26, x25, [sp, #-64]!           ; 16-byte Folded Spill
	.cfi_def_cfa_offset 64
	stp	x24, x23, [sp, #16]             ; 16-byte Folded Spill
	stp	x22, x21, [sp, #32]             ; 16-byte Folded Spill
	stp	x20, x19, [sp, #48]             ; 16-byte Folded Spill
	.cfi_offset w19, -8
	.cfi_offset w20, -16
	.cfi_offset w21, -24
	.cfi_offset w22, -32
	.cfi_offset w23, -40
	.cfi_offset w24, -48
	.cfi_offset w25, -56
	.cfi_offset w26, -64
	cbz	x2, LBB5_9
; %bb.1:
	mov	x8, x0
	mov	x9, #44233
	movk	x9, #38446, lsl #16
	movk	x9, #19347, lsl #32
	movk	x9, #35768, lsl #48
	mov	x10, #54435
	movk	x10, #54323, lsl #16
	movk	x10, #42542, lsl #32
	movk	x10, #19251, lsl #48
	add	x17, x0, x1
	cmp	x1, #48
	b.hi	LBB5_10
; %bb.2:
	eor	x11, x1, x9
	cmp	x1, #16
	b.hi	LBB5_39
; %bb.3:
	cmp	x1, #4
	b.lo	LBB5_33
; %bb.4:
	mov	x12, #0
	mov	x0, #0
	b	LBB5_7
LBB5_5:                                 ;   in Loop: Header=BB5_7 Depth=1
	ldr	x13, [x8]
	ldur	x14, [x17, #-8]
LBB5_6:                                 ;   in Loop: Header=BB5_7 Depth=1
	eor	x15, x12, x10
	mul	x16, x15, x9
	umulh	x15, x15, x9
	eor	x15, x15, x16
	eor	x13, x13, x9
	eor	x14, x14, x15
	eor	x14, x14, x12
	eor	x14, x14, x1
	mul	x15, x14, x13
	umulh	x13, x14, x13
	eor	x14, x15, #0xaaaaaaaaaaaaaaaa
	eor	x13, x11, x13
	mul	x15, x13, x14
	umulh	x13, x13, x14
	eor	x13, x13, x15
	add	x0, x0, x13
	add	x12, x12, #1
	cmp	x2, x12
	b.eq	LBB5_38
LBB5_7:                                 ; =>This Inner Loop Header: Depth=1
	cmp	x1, #7
	b.hi	LBB5_5
; %bb.8:                                ;   in Loop: Header=BB5_7 Depth=1
	ldr	w13, [x8]
	ldur	w14, [x17, #-4]
	b	LBB5_6
LBB5_9:
	mov	x0, #0
	b	LBB5_38
LBB5_10:
	mov	x11, #25647
	movk	x11, #30909, lsl #16
	movk	x11, #7524, lsl #32
	movk	x11, #41078, lsl #48
	mov	x12, #43591
	movk	x12, #7649, lsl #16
	movk	x12, #11685, lsl #32
	movk	x12, #19802, lsl #48
	mov	x13, #30885
	movk	x13, #43628, lsl #16
	movk	x13, #36300, lsl #32
	movk	x13, #11573, lsl #48
	cmp	x1, #512
	b.hi	LBB5_22
; %bb.11:
	mov	x14, #0
	mov	x0, #0
	b	LBB5_13
LBB5_12:                                ;   in Loop: Header=BB5_13 Depth=1
	add	x16, x16, x15
	ldp	x3, x16, [x16, #-16]
	eor	x3, x15, x3
	eor	x3, x3, x9
	eor	x16, x16, x17
	mul	x17, x16, x3
	umulh	x16, x16, x3
	eor	x17, x17, #0xaaaaaaaaaaaaaaaa
	eor	x15, x15, x16
	eor	x15, x15, x9
	mul	x16, x15, x17
	umulh	x15, x15, x17
	eor	x15, x15, x16
	add	x0, x0, x15
	add	x14, x14, #1
	cmp	x14, x2
	b.eq	LBB5_38
LBB5_13:                                ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB5_15 Depth 2
	eor	x15, x14, x10
	mul	x16, x15, x9
	umulh	x15, x15, x9
	eor	x15, x15, x16
	eor	x17, x14, x15
	cmp	x1, #81
	b.lo	LBB5_17
; %bb.14:                               ;   in Loop: Header=BB5_13 Depth=1
	mov	x15, x1
	mov	x16, x8
	mov	x3, x17
	mov	x4, x17
	mov	x5, x17
	mov	x6, x17
LBB5_15:                                ;   Parent Loop BB5_13 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldp	x7, x19, [x16]
	eor	x7, x7, x13
	eor	x17, x19, x17
	mul	x19, x17, x7
	umulh	x17, x17, x7
	eor	x17, x17, x19
	ldp	x7, x19, [x16, #16]
	eor	x7, x7, x9
	eor	x3, x19, x3
	mul	x19, x3, x7
	umulh	x3, x3, x7
	eor	x3, x3, x19
	ldp	x7, x19, [x16, #32]
	eor	x7, x7, x10
	eor	x4, x19, x4
	mul	x19, x4, x7
	umulh	x4, x4, x7
	eor	x4, x4, x19
	ldp	x7, x19, [x16, #48]
	eor	x7, x7, x12
	eor	x5, x19, x5
	mul	x19, x5, x7
	umulh	x5, x5, x7
	eor	x5, x5, x19
	ldp	x7, x19, [x16, #64]
	eor	x7, x7, x11
	eor	x6, x19, x6
	mul	x19, x6, x7
	umulh	x6, x6, x7
	eor	x6, x6, x19
	add	x16, x16, #80
	sub	x15, x15, #80
	cmp	x15, #80
	b.hi	LBB5_15
; %bb.16:                               ;   in Loop: Header=BB5_13 Depth=1
	eor	x17, x3, x17
	eor	x17, x17, x4
	eor	x17, x17, x5
	eor	x17, x17, x6
	cmp	x15, #17
	b.lo	LBB5_12
	b	LBB5_18
LBB5_17:                                ;   in Loop: Header=BB5_13 Depth=1
	mov	x16, x8
	mov	x15, x1
LBB5_18:                                ;   in Loop: Header=BB5_13 Depth=1
	ldp	x3, x4, [x16]
	eor	x3, x3, x10
	eor	x17, x4, x17
	mul	x4, x17, x3
	umulh	x17, x17, x3
	eor	x17, x17, x4
	cmp	x15, #33
	b.lo	LBB5_12
; %bb.19:                               ;   in Loop: Header=BB5_13 Depth=1
	ldp	x3, x4, [x16, #16]
	eor	x3, x3, x10
	eor	x17, x4, x17
	mul	x4, x17, x3
	umulh	x17, x17, x3
	eor	x17, x17, x4
	cmp	x15, #49
	b.lo	LBB5_12
; %bb.20:                               ;   in Loop: Header=BB5_13 Depth=1
	ldp	x3, x4, [x16, #32]
	eor	x3, x3, x9
	eor	x17, x4, x17
	mul	x4, x17, x3
	umulh	x17, x17, x3
	eor	x17, x17, x4
	cmp	x15, #65
	b.lo	LBB5_12
; %bb.21:                               ;   in Loop: Header=BB5_13 Depth=1
	ldp	x3, x4, [x16, #48]
	eor	x3, x3, x9
	eor	x17, x4, x17
	mul	x4, x17, x3
	umulh	x17, x17, x3
	eor	x17, x17, x4
	b	LBB5_12
LBB5_22:
	mov	x14, #0
	mov	x0, #0
	sub	x15, x1, #113
	lsr	x15, x15, #4
	mov	x16, #9363
	movk	x16, #37449, lsl #16
	movk	x16, #18724, lsl #32
	movk	x16, #9362, lsl #48
	umulh	x15, x15, x16
	mov	w16, #112
	madd	x15, x15, x16, x8
	add	x15, x15, #113
	ldp	x16, x17, [x17, #-16]
	mov	x3, #10459
	movk	x3, #41140, lsl #16
	movk	x3, #32465, lsl #32
	movk	x3, #59139, lsl #48
	mov	x4, #14476
	movk	x4, #10268, lsl #16
	movk	x4, #5989, lsl #32
	movk	x4, #37101, lsl #48
	b	LBB5_24
LBB5_23:                                ;   in Loop: Header=BB5_24 Depth=1
	eor	x6, x5, x16
	eor	x6, x6, x9
	eor	x7, x17, x7
	mul	x19, x7, x6
	umulh	x6, x7, x6
	eor	x7, x19, #0xaaaaaaaaaaaaaaaa
	eor	x5, x5, x6
	eor	x5, x5, x9
	mul	x6, x5, x7
	umulh	x5, x5, x7
	eor	x5, x5, x6
	add	x0, x0, x5
	add	x14, x14, #1
	cmp	x14, x2
	b.eq	LBB5_38
LBB5_24:                                ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB5_25 Depth 2
	eor	x5, x14, x10
	mul	x6, x5, x9
	umulh	x5, x5, x9
	eor	x5, x5, x6
	eor	x7, x14, x5
	mov	x5, x1
	mov	x6, x8
	mov	x19, x7
	mov	x20, x7
	mov	x21, x7
	mov	x22, x7
	mov	x23, x7
	mov	x24, x7
LBB5_25:                                ;   Parent Loop BB5_24 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldp	x25, x26, [x6]
	eor	x25, x25, x13
	eor	x7, x26, x7
	mul	x26, x7, x25
	umulh	x7, x7, x25
	eor	x7, x7, x26
	ldp	x25, x26, [x6, #16]
	eor	x25, x25, x9
	eor	x19, x26, x19
	mul	x26, x19, x25
	umulh	x19, x19, x25
	eor	x19, x19, x26
	ldp	x25, x26, [x6, #32]
	eor	x25, x25, x10
	eor	x20, x26, x20
	mul	x26, x20, x25
	umulh	x20, x20, x25
	eor	x20, x20, x26
	ldp	x25, x26, [x6, #48]
	eor	x25, x25, x12
	eor	x21, x26, x21
	mul	x26, x21, x25
	umulh	x21, x21, x25
	eor	x21, x21, x26
	ldp	x25, x26, [x6, #64]
	eor	x25, x25, x11
	eor	x22, x26, x22
	mul	x26, x22, x25
	umulh	x22, x22, x25
	eor	x22, x22, x26
	ldp	x25, x26, [x6, #80]
	eor	x25, x25, x3
	eor	x23, x26, x23
	mul	x26, x23, x25
	umulh	x23, x23, x25
	eor	x23, x23, x26
	ldp	x25, x26, [x6, #96]
	eor	x25, x25, x4
	eor	x24, x26, x24
	mul	x26, x24, x25
	umulh	x24, x24, x25
	eor	x24, x24, x26
	add	x6, x6, #112
	sub	x5, x5, #112
	cmp	x5, #112
	b.hi	LBB5_25
; %bb.26:                               ;   in Loop: Header=BB5_24 Depth=1
	eor	x7, x19, x7
	eor	x7, x7, x20
	eor	x7, x7, x21
	eor	x7, x7, x22
	eor	x7, x7, x23
	eor	x7, x7, x24
	cmp	x5, #17
	b.lo	LBB5_23
; %bb.27:                               ;   in Loop: Header=BB5_24 Depth=1
	ldr	x6, [x6]
	eor	x6, x6, x10
	ldur	x19, [x15, #7]
	eor	x7, x19, x7
	mul	x19, x7, x6
	umulh	x6, x7, x6
	eor	x7, x6, x19
	cmp	x5, #33
	b.lo	LBB5_23
; %bb.28:                               ;   in Loop: Header=BB5_24 Depth=1
	ldur	x6, [x15, #15]
	eor	x6, x6, x10
	ldur	x19, [x15, #23]
	eor	x7, x19, x7
	mul	x19, x7, x6
	umulh	x6, x7, x6
	eor	x7, x6, x19
	cmp	x5, #49
	b.lo	LBB5_23
; %bb.29:                               ;   in Loop: Header=BB5_24 Depth=1
	ldur	x6, [x15, #31]
	eor	x6, x6, x9
	ldur	x19, [x15, #39]
	eor	x7, x19, x7
	mul	x19, x7, x6
	umulh	x6, x7, x6
	eor	x7, x6, x19
	cmp	x5, #65
	b.lo	LBB5_23
; %bb.30:                               ;   in Loop: Header=BB5_24 Depth=1
	ldur	x6, [x15, #47]
	eor	x6, x6, x9
	ldur	x19, [x15, #55]
	eor	x7, x19, x7
	mul	x19, x7, x6
	umulh	x6, x7, x6
	eor	x7, x6, x19
	cmp	x5, #81
	b.lo	LBB5_23
; %bb.31:                               ;   in Loop: Header=BB5_24 Depth=1
	ldur	x6, [x15, #63]
	eor	x6, x6, x10
	ldur	x19, [x15, #71]
	eor	x7, x19, x7
	mul	x19, x7, x6
	umulh	x6, x7, x6
	eor	x7, x6, x19
	cmp	x5, #97
	b.lo	LBB5_23
; %bb.32:                               ;   in Loop: Header=BB5_24 Depth=1
	ldur	x6, [x15, #79]
	eor	x6, x6, x9
	ldur	x19, [x15, #87]
	eor	x7, x19, x7
	mul	x19, x7, x6
	umulh	x6, x7, x6
	eor	x7, x6, x19
	b	LBB5_23
LBB5_33:
	cbz	x1, LBB5_36
; %bb.34:
	mov	x12, #0
	mov	x0, #0
	lsr	x13, x1, #1
	ldrb	w14, [x8]
	ldurb	w15, [x17, #-1]
	bfi	x15, x14, #45, #8
	ldrb	w8, [x8, x13]
	eor	x13, x15, x9
LBB5_35:                                ; =>This Inner Loop Header: Depth=1
	eor	x14, x12, x10
	mul	x15, x14, x9
	umulh	x14, x14, x9
	eor	x14, x14, x15
	eor	x14, x14, x8
	eor	x14, x14, x12
	mul	x15, x14, x13
	umulh	x14, x14, x13
	eor	x15, x15, #0xaaaaaaaaaaaaaaaa
	eor	x14, x11, x14
	mul	x16, x14, x15
	umulh	x14, x14, x15
	eor	x14, x14, x16
	add	x0, x0, x14
	add	x12, x12, #1
	cmp	x2, x12
	b.ne	LBB5_35
	b	LBB5_38
LBB5_36:
	mov	x8, #0
	mov	x0, #0
LBB5_37:                                ; =>This Inner Loop Header: Depth=1
	eor	x12, x8, x10
	mul	x13, x12, x9
	umulh	x12, x12, x9
	eor	x12, x12, x13
	eor	x12, x8, x12
	umulh	x13, x12, x9
	mul	x12, x12, x9
	eor	x12, x12, #0xaaaaaaaaaaaaaaaa
	eor	x13, x11, x13
	mul	x14, x13, x12
	umulh	x12, x13, x12
	eor	x12, x12, x14
	add	x0, x0, x12
	add	x8, x8, #1
	cmp	x2, x8
	b.ne	LBB5_37
LBB5_38:
	ldp	x20, x19, [sp, #48]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #32]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp, #16]             ; 16-byte Folded Reload
	ldp	x26, x25, [sp], #64             ; 16-byte Folded Reload
	ret
LBB5_39:
	mov	x12, #0
	mov	x0, #0
	ldp	x14, x13, [x8]
	eor	x14, x14, x10
	ldp	x16, x15, [x17, #-16]
	eor	x16, x16, x1
	eor	x16, x16, x9
	b	LBB5_41
LBB5_40:                                ;   in Loop: Header=BB5_41 Depth=1
	eor	x17, x17, x15
	mul	x3, x17, x16
	umulh	x17, x17, x16
	eor	x3, x3, #0xaaaaaaaaaaaaaaaa
	eor	x17, x11, x17
	mul	x4, x17, x3
	umulh	x17, x17, x3
	eor	x17, x17, x4
	add	x0, x0, x17
	add	x12, x12, #1
	cmp	x2, x12
	b.eq	LBB5_38
LBB5_41:                                ; =>This Inner Loop Header: Depth=1
	eor	x17, x12, x10
	mul	x3, x17, x9
	umulh	x17, x17, x9
	eor	x17, x17, x3
	eor	x17, x13, x17
	eor	x17, x17, x12
	mul	x3, x17, x14
	umulh	x17, x17, x14
	eor	x17, x17, x3
	cmp	x1, #33
	b.lo	LBB5_40
; %bb.42:                               ;   in Loop: Header=BB5_41 Depth=1
	ldp	x3, x4, [x8, #16]
	eor	x3, x3, x10
	eor	x17, x4, x17
	mul	x4, x17, x3
	umulh	x17, x17, x3
	eor	x17, x17, x4
	b	LBB5_40
	.cfi_endproc
                                        ; -- End function
	.globl	_main                           ; -- Begin function main
	.p2align	2
_main:                                  ; @main
Lfunc_begin0:
	.cfi_startproc
	.cfi_personality 155, ___gxx_personality_v0
	.cfi_lsda 16, Lexception0
; %bb.0:
	stp	d11, d10, [sp, #-128]!          ; 16-byte Folded Spill
	.cfi_def_cfa_offset 128
	stp	d9, d8, [sp, #16]               ; 16-byte Folded Spill
	stp	x28, x27, [sp, #32]             ; 16-byte Folded Spill
	stp	x26, x25, [sp, #48]             ; 16-byte Folded Spill
	stp	x24, x23, [sp, #64]             ; 16-byte Folded Spill
	stp	x22, x21, [sp, #80]             ; 16-byte Folded Spill
	stp	x20, x19, [sp, #96]             ; 16-byte Folded Spill
	stp	x29, x30, [sp, #112]            ; 16-byte Folded Spill
	add	x29, sp, #112
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	.cfi_offset w21, -40
	.cfi_offset w22, -48
	.cfi_offset w23, -56
	.cfi_offset w24, -64
	.cfi_offset w25, -72
	.cfi_offset w26, -80
	.cfi_offset w27, -88
	.cfi_offset w28, -96
	.cfi_offset b8, -104
	.cfi_offset b9, -112
	.cfi_offset b10, -120
	.cfi_offset b11, -128
	sub	sp, sp, #2592
	mov	w27, #38528
	movk	w27, #152, lsl #16
	cmp	w0, #2
	b.lt	LBB6_4
; %bb.1:
	mov	x21, x1
	mov	x22, x0
	mov	x26, #-9
	movk	x26, #32767, lsl #48
	ldr	x23, [x1, #8]
	mov	x0, x23
	bl	_strlen
	cmp	x0, x26
	b.hi	LBB6_79
; %bb.2:
	mov	x19, x0
	cmp	x0, #23
	b.hs	LBB6_5
; %bb.3:
	strb	w19, [sp, #79]
	add	x24, sp, #56
	cbnz	x19, LBB6_6
	b	LBB6_7
LBB6_4:
	mov	w19, #512
	b	LBB6_19
LBB6_5:
	and	x8, x19, #0xfffffffffffffff8
	add	x8, x8, #8
	orr	x9, x19, #0x7
	cmp	x9, #23
	csel	x8, x8, x9, eq
	add	x25, x8, #1
	mov	x0, x25
	bl	__Znwm
	mov	x24, x0
	orr	x8, x25, #0x8000000000000000
	stp	x19, x8, [sp, #64]
	str	x0, [sp, #56]
LBB6_6:
	mov	x0, x24
	mov	x1, x23
	mov	x2, x19
	bl	_memmove
LBB6_7:
	strb	wzr, [x24, x19]
Ltmp0:
	add	x0, sp, #56
	mov	x1, #0
	mov	w2, #10
	bl	__ZNSt3__16stoullERKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPmi
Ltmp1:
; %bb.8:
	mov	x19, x0
	ldrsb	w8, [sp, #79]
	tbnz	w8, #31, LBB6_10
; %bb.9:
	cmp	w22, #3
	b.hs	LBB6_11
	b	LBB6_19
LBB6_10:
	ldr	x0, [sp, #56]
	bl	__ZdlPv
	cmp	w22, #3
	b.lo	LBB6_19
LBB6_11:
	ldr	x21, [x21, #16]
	mov	x0, x21
	bl	_strlen
	cmp	x0, x26
	b.hi	LBB6_79
; %bb.12:
	mov	x20, x0
	cmp	x0, #23
	b.hs	LBB6_14
; %bb.13:
	strb	w20, [sp, #79]
	add	x22, sp, #56
	cbnz	x20, LBB6_15
	b	LBB6_16
LBB6_14:
	and	x8, x20, #0xfffffffffffffff8
	add	x8, x8, #8
	orr	x9, x20, #0x7
	cmp	x9, #23
	csel	x8, x8, x9, eq
	add	x23, x8, #1
	mov	x0, x23
	bl	__Znwm
	mov	x22, x0
	orr	x8, x23, #0x8000000000000000
	stp	x20, x8, [sp, #64]
	str	x0, [sp, #56]
LBB6_15:
	mov	x0, x22
	mov	x1, x21
	mov	x2, x20
	bl	_memmove
LBB6_16:
	strb	wzr, [x22, x20]
Ltmp3:
	add	x0, sp, #56
	mov	x1, #0
	mov	w2, #10
	bl	__ZNSt3__16stoullERKNS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEEPmi
Ltmp4:
; %bb.17:
	mov	x27, x0
	ldrsb	w8, [sp, #79]
	tbz	w8, #31, LBB6_19
; %bb.18:
	ldr	x0, [sp, #56]
	bl	__ZdlPv
LBB6_19:
	stp	x19, x27, [sp]
Lloh12:
	adrp	x0, l_.str@PAGE
Lloh13:
	add	x0, x0, l_.str@PAGEOFF
	bl	_printf
Lloh14:
	adrp	x0, l_str@PAGE
Lloh15:
	add	x0, x0, l_str@PAGEOFF
	bl	_puts
	stp	xzr, xzr, [x29, #-144]
	stur	xzr, [x29, #-128]
	cbz	x19, LBB6_23
; %bb.20:
	tbnz	x19, #63, LBB6_80
; %bb.21:
Ltmp6:
	mov	x0, x19
	bl	__Znwm
Ltmp7:
; %bb.22:
	mov	x21, x0
	add	x20, x0, x19
	stur	x0, [x29, #-144]
	stur	x20, [x29, #-128]
	mov	x1, x19
	bl	_bzero
	stur	x20, [x29, #-136]
	b	LBB6_24
LBB6_23:
	mov	x20, #0
	mov	x21, #0
LBB6_24:
	mov	w8, #12345
	str	x8, [sp, #56]
	mov	w9, #1
	mov	x10, #32557
	movk	x10, #19605, lsl #16
	movk	x10, #62509, lsl #32
	movk	x10, #22609, lsl #48
	add	x11, sp, #56
LBB6_25:                                ; =>This Inner Loop Header: Depth=1
	eor	x8, x8, x8, lsr #62
	madd	x8, x8, x10, x9
	str	x8, [x11, x9, lsl #3]
	add	x9, x9, #1
	cmp	x9, #312
	b.ne	LBB6_25
; %bb.26:
	cmp	x21, x20
	str	x27, [sp, #24]                  ; 8-byte Folded Spill
	b.eq	LBB6_29
; %bb.27:
	mov	x16, #0
	mov	x8, #3361
	movk	x8, #8402, lsl #16
	movk	x8, #53773, lsl #32
	movk	x8, #3360, lsl #48
	mov	w9, #312
	add	x10, sp, #56
	mov	x11, #6633
	movk	x11, #43366, lsl #16
	movk	x11, #28506, lsl #32
	movk	x11, #46338, lsl #48
	mov	x12, #131941395333120
	movk	x12, #6, lsl #48
	mov	x14, #255086697644032
	movk	x14, #7, lsl #48
	mov	x15, x21
LBB6_28:                                ; =>This Inner Loop Header: Depth=1
	add	x13, x16, #1
	lsr	x17, x13, #3
	umulh	x17, x17, x8
	lsr	x17, x17, #1
	msub	x13, x17, x9, x13
	lsl	x17, x16, #3
	ldr	x0, [x10, x17]
	and	x0, x0, #0xffffffff80000000
	ldr	x1, [x10, x13, lsl #3]
	and	x2, x1, #0x7ffffffe
	orr	x0, x2, x0
	add	x16, x16, #156
	lsr	x2, x16, #3
	umulh	x2, x2, x8
	lsr	x2, x2, #1
	msub	x16, x2, x9, x16
	ldr	x16, [x10, x16, lsl #3]
	eor	x16, x16, x0, lsr #1
	sbfx	x0, x1, #0, #1
	and	x0, x0, x11
	eor	x16, x16, x0
	str	x16, [x10, x17]
	lsr	x17, x16, #29
	and	x17, x17, #0x5555555555555555
	eor	x17, x17, x16
	and	x0, x12, x17, lsl #17
	and	x1, x14, x17, lsl #37
	eor	x0, x1, x0
	eor	x16, x0, x16
	lsr	x16, x16, #43
	eor	w16, w16, w17
	strb	w16, [x15], #1
	mov	x16, x13
	cmp	x15, x20
	b.ne	LBB6_28
	b	LBB6_30
LBB6_29:
	mov	x13, #0
LBB6_30:
	mov	x23, #44233
	movk	x23, #38446, lsl #16
	movk	x23, #19347, lsl #32
	movk	x23, #35768, lsl #48
	mov	x24, #10459
	movk	x24, #41140, lsl #16
	movk	x24, #32465, lsl #32
	movk	x24, #59139, lsl #48
	mov	x25, #25647
	movk	x25, #30909, lsl #16
	movk	x25, #7524, lsl #32
	movk	x25, #41078, lsl #48
	mov	x26, #14476
	movk	x26, #10268, lsl #16
	movk	x26, #5989, lsl #32
	movk	x26, #37101, lsl #48
	mov	x27, #43591
	movk	x27, #7649, lsl #16
	movk	x27, #11685, lsl #32
	movk	x27, #19802, lsl #48
	mov	x28, #54435
	movk	x28, #54323, lsl #16
	movk	x28, #42542, lsl #32
	movk	x28, #19251, lsl #48
	str	x13, [sp, #2552]
	mov	x20, #30885
	movk	x20, #43628, lsl #16
	movk	x20, #36300, lsl #32
	movk	x20, #11573, lsl #48
Lloh16:
	adrp	x0, l_str.11@PAGE
Lloh17:
	add	x0, x0, l_str.11@PAGEOFF
	bl	_puts
	mov	x22, #0
	lsr	x10, x19, #1
	sub	x8, x19, #113
	lsr	x8, x8, #4
	mov	x9, #9363
	movk	x9, #37449, lsl #16
	movk	x9, #18724, lsl #32
	movk	x9, #9362, lsl #48
	umulh	x8, x8, x9
	mov	w9, #112
	madd	x8, x8, x9, x21
	add	x8, x8, #113
	stp	x8, x10, [sp, #32]              ; 16-byte Folded Spill
	add	x8, x21, x19
	str	x8, [sp, #48]                   ; 8-byte Folded Spill
	b	LBB6_34
LBB6_31:                                ;   in Loop: Header=BB6_34 Depth=1
	ldr	w10, [x21]
	ldr	x9, [sp, #48]                   ; 8-byte Folded Reload
	ldur	w11, [x9, #-4]
LBB6_32:                                ;   in Loop: Header=BB6_34 Depth=1
	mov	x9, x19
LBB6_33:                                ;   in Loop: Header=BB6_34 Depth=1
	eor	x10, x10, x23
	eor	x8, x8, x11
	mul	x11, x8, x10
	umulh	x8, x8, x10
	eor	x10, x11, #0xaaaaaaaaaaaaaaaa
	eor	x8, x9, x8
	eor	x8, x8, x23
	mul	x9, x8, x10
	umulh	x8, x8, x10
	eor	x0, x8, x9
	bl	__Z6escapey
	add	x22, x22, #1
	mov	w8, #34464
	movk	w8, #1, lsl #16
	cmp	x22, x8
	b.eq	LBB6_76
LBB6_34:                                ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB6_44 Depth 2
                                        ;     Child Loop BB6_66 Depth 2
	cmp	x19, #16
	b.hi	LBB6_38
; %bb.35:                               ;   in Loop: Header=BB6_34 Depth=1
	eor	x8, x22, x28
	mul	x9, x8, x23
	umulh	x8, x8, x23
	eor	x8, x8, x9
	eor	x8, x22, x8
	cmp	x19, #4
	b.lo	LBB6_41
; %bb.36:                               ;   in Loop: Header=BB6_34 Depth=1
	eor	x8, x8, x19
	cmp	x19, #8
	b.lo	LBB6_53
; %bb.37:                               ;   in Loop: Header=BB6_34 Depth=1
	ldr	x9, [x21]
	ldr	x10, [sp, #48]                  ; 8-byte Folded Reload
	ldur	x10, [x10, #-8]
	b	LBB6_56
LBB6_38:                                ;   in Loop: Header=BB6_34 Depth=1
	cmp	x19, #2, lsl #12                ; =8192
	b.hi	LBB6_43
; %bb.39:                               ;   in Loop: Header=BB6_34 Depth=1
	mov	x0, x21
	mov	x1, x19
	mov	x2, x22
	cmp	x19, #128
	b.hi	LBB6_54
; %bb.40:                               ;   in Loop: Header=BB6_34 Depth=1
	bl	__ZN11mirror_hash6detail15hash_aes_mediumEPKvmy
	b	LBB6_57
LBB6_41:                                ;   in Loop: Header=BB6_34 Depth=1
	cbz	x19, LBB6_55
; %bb.42:                               ;   in Loop: Header=BB6_34 Depth=1
	ldrb	w10, [x21]
	ldr	x9, [sp, #48]                   ; 8-byte Folded Reload
	ldurb	w9, [x9, #-1]
	bfi	x9, x10, #45, #8
	ldr	x10, [sp, #40]                  ; 8-byte Folded Reload
	ldrb	w10, [x21, x10]
	b	LBB6_56
LBB6_43:                                ;   in Loop: Header=BB6_34 Depth=1
	eor	x8, x22, x28
	mul	x9, x8, x23
	umulh	x8, x8, x23
	eor	x8, x8, x9
	eor	x10, x22, x8
	mov	x8, x19
	mov	x9, x21
	mov	x11, x10
	mov	x12, x10
	mov	x13, x10
	mov	x14, x10
	mov	x15, x10
	mov	x16, x10
LBB6_44:                                ;   Parent Loop BB6_34 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldp	x17, x0, [x9]
	eor	x17, x17, x20
	eor	x10, x0, x10
	mul	x0, x10, x17
	umulh	x10, x10, x17
	eor	x10, x10, x0
	ldp	x17, x0, [x9, #16]
	eor	x17, x17, x23
	eor	x11, x0, x11
	mul	x0, x11, x17
	umulh	x11, x11, x17
	eor	x11, x11, x0
	ldp	x17, x0, [x9, #32]
	eor	x17, x17, x28
	eor	x12, x0, x12
	mul	x0, x12, x17
	umulh	x12, x12, x17
	eor	x12, x12, x0
	ldp	x17, x0, [x9, #48]
	eor	x17, x17, x27
	eor	x13, x0, x13
	mul	x0, x13, x17
	umulh	x13, x13, x17
	eor	x13, x13, x0
	ldp	x17, x0, [x9, #64]
	eor	x17, x17, x25
	eor	x14, x0, x14
	mul	x0, x14, x17
	umulh	x14, x14, x17
	eor	x14, x14, x0
	ldp	x17, x0, [x9, #80]
	eor	x17, x17, x24
	eor	x15, x0, x15
	mul	x0, x15, x17
	umulh	x15, x15, x17
	eor	x15, x15, x0
	ldp	x17, x0, [x9, #96]
	eor	x17, x17, x26
	eor	x16, x0, x16
	mul	x0, x16, x17
	umulh	x16, x16, x17
	eor	x16, x16, x0
	add	x9, x9, #112
	sub	x8, x8, #112
	cmp	x8, #112
	b.hi	LBB6_44
; %bb.45:                               ;   in Loop: Header=BB6_34 Depth=1
	eor	x10, x11, x10
	eor	x10, x10, x12
	eor	x10, x10, x13
	eor	x10, x10, x14
	eor	x10, x10, x15
	eor	x10, x10, x16
	cmp	x8, #17
	b.lo	LBB6_52
; %bb.46:                               ;   in Loop: Header=BB6_34 Depth=1
	ldr	x9, [x9]
	eor	x9, x9, x28
	ldr	x11, [sp, #32]                  ; 8-byte Folded Reload
	ldur	x11, [x11, #7]
	eor	x10, x11, x10
	mul	x11, x10, x9
	umulh	x9, x10, x9
	eor	x10, x9, x11
	cmp	x8, #33
	b.lo	LBB6_52
; %bb.47:                               ;   in Loop: Header=BB6_34 Depth=1
	ldr	x11, [sp, #32]                  ; 8-byte Folded Reload
	ldur	x9, [x11, #15]
	eor	x9, x9, x28
	ldur	x11, [x11, #23]
	eor	x10, x11, x10
	mul	x11, x10, x9
	umulh	x9, x10, x9
	eor	x10, x9, x11
	cmp	x8, #49
	b.lo	LBB6_52
; %bb.48:                               ;   in Loop: Header=BB6_34 Depth=1
	ldr	x11, [sp, #32]                  ; 8-byte Folded Reload
	ldur	x9, [x11, #31]
	eor	x9, x9, x23
	ldur	x11, [x11, #39]
	eor	x10, x11, x10
	mul	x11, x10, x9
	umulh	x9, x10, x9
	eor	x10, x9, x11
	cmp	x8, #65
	b.lo	LBB6_52
; %bb.49:                               ;   in Loop: Header=BB6_34 Depth=1
	ldr	x11, [sp, #32]                  ; 8-byte Folded Reload
	ldur	x9, [x11, #47]
	eor	x9, x9, x23
	ldur	x11, [x11, #55]
	eor	x10, x11, x10
	mul	x11, x10, x9
	umulh	x9, x10, x9
	eor	x10, x9, x11
	cmp	x8, #81
	b.lo	LBB6_52
; %bb.50:                               ;   in Loop: Header=BB6_34 Depth=1
	ldr	x11, [sp, #32]                  ; 8-byte Folded Reload
	ldur	x9, [x11, #63]
	eor	x9, x9, x28
	ldur	x11, [x11, #71]
	eor	x10, x11, x10
	mul	x11, x10, x9
	umulh	x9, x10, x9
	eor	x10, x9, x11
	cmp	x8, #97
	b.lo	LBB6_52
; %bb.51:                               ;   in Loop: Header=BB6_34 Depth=1
	ldr	x11, [sp, #32]                  ; 8-byte Folded Reload
	ldur	x9, [x11, #79]
	eor	x9, x9, x23
	ldur	x11, [x11, #87]
	eor	x10, x11, x10
	mul	x11, x10, x9
	umulh	x9, x10, x9
	eor	x10, x9, x11
LBB6_52:                                ;   in Loop: Header=BB6_34 Depth=1
	ldr	x11, [sp, #48]                  ; 8-byte Folded Reload
	ldp	x9, x11, [x11, #-16]
	eor	x9, x8, x9
	eor	x9, x9, x23
	eor	x10, x11, x10
	mul	x11, x10, x9
	umulh	x9, x10, x9
	eor	x10, x11, #0xaaaaaaaaaaaaaaaa
	eor	x8, x8, x9
	eor	x8, x8, x23
	mul	x9, x8, x10
	umulh	x8, x8, x10
	eor	x0, x8, x9
	b	LBB6_57
LBB6_53:                                ;   in Loop: Header=BB6_34 Depth=1
	ldr	w9, [x21]
	ldr	x10, [sp, #48]                  ; 8-byte Folded Reload
	ldur	w10, [x10, #-4]
	b	LBB6_56
LBB6_54:                                ;   in Loop: Header=BB6_34 Depth=1
	bl	__ZN11mirror_hash6detail13hash_aes_bulkEPKvmy
	b	LBB6_57
LBB6_55:                                ;   in Loop: Header=BB6_34 Depth=1
	mov	x10, #0
	mov	x9, #0
LBB6_56:                                ;   in Loop: Header=BB6_34 Depth=1
	eor	x9, x9, x23
	eor	x8, x8, x10
	mul	x10, x8, x9
	umulh	x8, x8, x9
	eor	x9, x10, #0xaaaaaaaaaaaaaaaa
	eor	x8, x19, x8
	eor	x8, x8, x23
	mul	x10, x8, x9
	umulh	x8, x8, x9
	eor	x0, x8, x10
LBB6_57:                                ;   in Loop: Header=BB6_34 Depth=1
	bl	__Z6escapey
	eor	x8, x22, x28
	mul	x9, x8, x23
	umulh	x8, x8, x23
	eor	x8, x8, x9
	eor	x8, x22, x8
	cmp	x19, #16
	b.hi	LBB6_64
; %bb.58:                               ;   in Loop: Header=BB6_34 Depth=1
	cmp	x19, #4
	b.lo	LBB6_61
; %bb.59:                               ;   in Loop: Header=BB6_34 Depth=1
	eor	x8, x8, x19
	cmp	x19, #8
	b.lo	LBB6_31
; %bb.60:                               ;   in Loop: Header=BB6_34 Depth=1
	ldr	x10, [x21]
	ldr	x9, [sp, #48]                   ; 8-byte Folded Reload
	ldur	x11, [x9, #-8]
	b	LBB6_32
LBB6_61:                                ;   in Loop: Header=BB6_34 Depth=1
	cbz	x19, LBB6_63
; %bb.62:                               ;   in Loop: Header=BB6_34 Depth=1
	ldrb	w9, [x21]
	ldr	x10, [sp, #48]                  ; 8-byte Folded Reload
	ldurb	w10, [x10, #-1]
	bfi	x10, x9, #45, #8
	ldr	x9, [sp, #40]                   ; 8-byte Folded Reload
	ldrb	w11, [x21, x9]
	b	LBB6_32
LBB6_63:                                ;   in Loop: Header=BB6_34 Depth=1
	mov	x11, #0
	mov	x10, #0
	mov	x9, #0
	b	LBB6_33
LBB6_64:                                ;   in Loop: Header=BB6_34 Depth=1
	cmp	x19, #113
	b.lo	LBB6_68
; %bb.65:                               ;   in Loop: Header=BB6_34 Depth=1
	mov	x9, x19
	mov	x10, x21
	mov	x11, x8
	mov	x12, x8
	mov	x13, x8
	mov	x14, x8
	mov	x15, x8
	mov	x16, x8
LBB6_66:                                ;   Parent Loop BB6_34 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldp	x17, x0, [x10]
	eor	x17, x17, x20
	eor	x8, x0, x8
	mul	x0, x8, x17
	umulh	x8, x8, x17
	eor	x8, x8, x0
	ldp	x17, x0, [x10, #16]
	eor	x17, x17, x23
	eor	x11, x0, x11
	mul	x0, x11, x17
	umulh	x11, x11, x17
	eor	x11, x11, x0
	ldp	x17, x0, [x10, #32]
	eor	x17, x17, x28
	eor	x12, x0, x12
	mul	x0, x12, x17
	umulh	x12, x12, x17
	eor	x12, x12, x0
	ldp	x17, x0, [x10, #48]
	eor	x17, x17, x27
	eor	x13, x0, x13
	mul	x0, x13, x17
	umulh	x13, x13, x17
	eor	x13, x13, x0
	ldp	x17, x0, [x10, #64]
	eor	x17, x17, x25
	eor	x14, x0, x14
	mul	x0, x14, x17
	umulh	x14, x14, x17
	eor	x14, x14, x0
	ldp	x17, x0, [x10, #80]
	eor	x17, x17, x24
	eor	x15, x0, x15
	mul	x0, x15, x17
	umulh	x15, x15, x17
	eor	x15, x15, x0
	ldp	x17, x0, [x10, #96]
	eor	x17, x17, x26
	eor	x16, x0, x16
	mul	x0, x16, x17
	umulh	x16, x16, x17
	eor	x16, x16, x0
	add	x10, x10, #112
	sub	x9, x9, #112
	cmp	x9, #112
	b.hi	LBB6_66
; %bb.67:                               ;   in Loop: Header=BB6_34 Depth=1
	eor	x8, x11, x8
	eor	x8, x8, x12
	eor	x8, x8, x13
	eor	x8, x8, x14
	eor	x8, x8, x15
	eor	x8, x8, x16
	cmp	x9, #17
	b.hs	LBB6_69
	b	LBB6_75
LBB6_68:                                ;   in Loop: Header=BB6_34 Depth=1
	mov	x10, x21
	mov	x9, x19
LBB6_69:                                ;   in Loop: Header=BB6_34 Depth=1
	ldp	x11, x12, [x10]
	eor	x11, x11, x28
	eor	x8, x12, x8
	mul	x12, x8, x11
	umulh	x8, x8, x11
	eor	x8, x8, x12
	cmp	x9, #33
	b.lo	LBB6_75
; %bb.70:                               ;   in Loop: Header=BB6_34 Depth=1
	ldp	x11, x12, [x10, #16]
	eor	x11, x11, x28
	eor	x8, x12, x8
	mul	x12, x8, x11
	umulh	x8, x8, x11
	eor	x8, x8, x12
	cmp	x9, #49
	b.lo	LBB6_75
; %bb.71:                               ;   in Loop: Header=BB6_34 Depth=1
	ldp	x11, x12, [x10, #32]
	eor	x11, x11, x23
	eor	x8, x12, x8
	mul	x12, x8, x11
	umulh	x8, x8, x11
	eor	x8, x8, x12
	cmp	x9, #65
	b.lo	LBB6_75
; %bb.72:                               ;   in Loop: Header=BB6_34 Depth=1
	ldp	x11, x12, [x10, #48]
	eor	x11, x11, x23
	eor	x8, x12, x8
	mul	x12, x8, x11
	umulh	x8, x8, x11
	eor	x8, x8, x12
	cmp	x9, #81
	b.lo	LBB6_75
; %bb.73:                               ;   in Loop: Header=BB6_34 Depth=1
	ldp	x11, x12, [x10, #64]
	eor	x11, x11, x28
	eor	x8, x12, x8
	mul	x12, x8, x11
	umulh	x8, x8, x11
	eor	x8, x8, x12
	cmp	x9, #97
	b.lo	LBB6_75
; %bb.74:                               ;   in Loop: Header=BB6_34 Depth=1
	ldp	x11, x12, [x10, #80]
	eor	x11, x11, x23
	eor	x8, x12, x8
	mul	x12, x8, x11
	umulh	x8, x8, x11
	eor	x8, x8, x12
LBB6_75:                                ;   in Loop: Header=BB6_34 Depth=1
	add	x10, x10, x9
	ldp	x12, x11, [x10, #-16]
	eor	x10, x12, x9
	b	LBB6_33
LBB6_76:
Lloh18:
	adrp	x0, l_str.12@PAGE
Lloh19:
	add	x0, x0, l_str.12@PAGEOFF
	bl	_puts
	bl	__Z7clobberv
	bl	__ZNSt3__16chrono12steady_clock3nowEv
	mov	x22, x0
	mov	x0, x21
	mov	x1, x19
	ldr	x25, [sp, #24]                  ; 8-byte Folded Reload
	mov	x2, x25
	bl	__Z17bench_mirror_hashPKvmm
	mov	x23, x0
	bl	__ZNSt3__16chrono12steady_clock3nowEv
	mov	x24, x0
	mov	x0, x23
	bl	__Z6escapey
	sub	x20, x24, x22
Lloh20:
	adrp	x0, l_str.13@PAGE
Lloh21:
	add	x0, x0, l_str.13@PAGEOFF
	bl	_puts
	bl	__Z7clobberv
	bl	__ZNSt3__16chrono12steady_clock3nowEv
	mov	x22, x0
	mov	x0, x21
	mov	x1, x19
	mov	x2, x25
	bl	__Z15bench_rapidhashPKvmm
	mov	x23, x0
	bl	__ZNSt3__16chrono12steady_clock3nowEv
	mov	x24, x0
	mov	x0, x23
	bl	__Z6escapey
	sub	x8, x24, x22
	scvtf	d0, x20
	ucvtf	d1, x25
	fdiv	d8, d0, d1
	scvtf	d0, x8
	fdiv	d9, d0, d1
	str	x19, [sp]
Lloh22:
	adrp	x0, l_.str.5@PAGE
Lloh23:
	add	x0, x0, l_.str.5@PAGEOFF
	bl	_printf
	ucvtf	d10, x19
	fdiv	d0, d10, d8
	stp	d8, d0, [sp]
Lloh24:
	adrp	x0, l_.str.6@PAGE
Lloh25:
	add	x0, x0, l_.str.6@PAGEOFF
	bl	_printf
	fdiv	d0, d10, d9
	stp	d9, d0, [sp]
Lloh26:
	adrp	x0, l_.str.7@PAGE
Lloh27:
	add	x0, x0, l_.str.7@PAGEOFF
	bl	_printf
	fdiv	d0, d9, d8
	fmov	d1, #-1.00000000
	fadd	d0, d0, d1
	mov	x8, #4636737291354636288
	fmov	d1, x8
	fmul	d0, d0, d1
	str	d0, [sp]
Lloh28:
	adrp	x0, l_.str.8@PAGE
Lloh29:
	add	x0, x0, l_.str.8@PAGEOFF
	bl	_printf
	cbz	x21, LBB6_78
; %bb.77:
	mov	x0, x21
	bl	__ZdlPv
LBB6_78:
	mov	w0, #0
	add	sp, sp, #2592
	ldp	x29, x30, [sp, #112]            ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #96]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #80]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp, #64]             ; 16-byte Folded Reload
	ldp	x26, x25, [sp, #48]             ; 16-byte Folded Reload
	ldp	x28, x27, [sp, #32]             ; 16-byte Folded Reload
	ldp	d9, d8, [sp, #16]               ; 16-byte Folded Reload
	ldp	d11, d10, [sp], #128            ; 16-byte Folded Reload
	ret
LBB6_79:
	add	x0, sp, #56
	bl	__ZNKSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE20__throw_length_errorB8ne190102Ev
LBB6_80:
Ltmp8:
	sub	x0, x29, #144
	bl	__ZNKSt3__16vectorIhNS_9allocatorIhEEE20__throw_length_errorB8ne190102Ev
Ltmp9:
; %bb.81:
	brk	#0x1
LBB6_82:
Ltmp5:
	b	LBB6_84
LBB6_83:
Ltmp2:
LBB6_84:
	mov	x19, x0
	ldrsb	w8, [sp, #79]
	tbz	w8, #31, LBB6_89
; %bb.85:
	ldr	x0, [sp, #56]
	b	LBB6_88
LBB6_86:
Ltmp10:
	mov	x19, x0
	ldur	x0, [x29, #-144]
	cbz	x0, LBB6_89
; %bb.87:
	stur	x0, [x29, #-136]
LBB6_88:
	bl	__ZdlPv
LBB6_89:
	mov	x0, x19
	bl	__Unwind_Resume
	.loh AdrpAdd	Lloh14, Lloh15
	.loh AdrpAdd	Lloh12, Lloh13
	.loh AdrpAdd	Lloh16, Lloh17
	.loh AdrpAdd	Lloh28, Lloh29
	.loh AdrpAdd	Lloh26, Lloh27
	.loh AdrpAdd	Lloh24, Lloh25
	.loh AdrpAdd	Lloh22, Lloh23
	.loh AdrpAdd	Lloh20, Lloh21
	.loh AdrpAdd	Lloh18, Lloh19
Lfunc_end0:
	.cfi_endproc
	.section	__TEXT,__gcc_except_tab
	.p2align	2, 0x0
GCC_except_table6:
Lexception0:
	.byte	255                             ; @LPStart Encoding = omit
	.byte	255                             ; @TType Encoding = omit
	.byte	1                               ; Call site Encoding = uleb128
	.uleb128 Lcst_end0-Lcst_begin0
Lcst_begin0:
	.uleb128 Lfunc_begin0-Lfunc_begin0      ; >> Call Site 1 <<
	.uleb128 Ltmp0-Lfunc_begin0             ;   Call between Lfunc_begin0 and Ltmp0
	.byte	0                               ;     has no landing pad
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp0-Lfunc_begin0             ; >> Call Site 2 <<
	.uleb128 Ltmp1-Ltmp0                    ;   Call between Ltmp0 and Ltmp1
	.uleb128 Ltmp2-Lfunc_begin0             ;     jumps to Ltmp2
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp1-Lfunc_begin0             ; >> Call Site 3 <<
	.uleb128 Ltmp3-Ltmp1                    ;   Call between Ltmp1 and Ltmp3
	.byte	0                               ;     has no landing pad
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp3-Lfunc_begin0             ; >> Call Site 4 <<
	.uleb128 Ltmp4-Ltmp3                    ;   Call between Ltmp3 and Ltmp4
	.uleb128 Ltmp5-Lfunc_begin0             ;     jumps to Ltmp5
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp6-Lfunc_begin0             ; >> Call Site 5 <<
	.uleb128 Ltmp7-Ltmp6                    ;   Call between Ltmp6 and Ltmp7
	.uleb128 Ltmp10-Lfunc_begin0            ;     jumps to Ltmp10
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp7-Lfunc_begin0             ; >> Call Site 6 <<
	.uleb128 Ltmp8-Ltmp7                    ;   Call between Ltmp7 and Ltmp8
	.byte	0                               ;     has no landing pad
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp8-Lfunc_begin0             ; >> Call Site 7 <<
	.uleb128 Ltmp9-Ltmp8                    ;   Call between Ltmp8 and Ltmp9
	.uleb128 Ltmp10-Lfunc_begin0            ;     jumps to Ltmp10
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp9-Lfunc_begin0             ; >> Call Site 8 <<
	.uleb128 Lfunc_end0-Ltmp9               ;   Call between Ltmp9 and Lfunc_end0
	.byte	0                               ;     has no landing pad
	.byte	0                               ;   On action: cleanup
Lcst_end0:
	.p2align	2, 0x0
                                        ; -- End function
	.section	__TEXT,__text,regular,pure_instructions
	.private_extern	__ZNKSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE20__throw_length_errorB8ne190102Ev ; -- Begin function _ZNKSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE20__throw_length_errorB8ne190102Ev
	.globl	__ZNKSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE20__throw_length_errorB8ne190102Ev
	.weak_def_can_be_hidden	__ZNKSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE20__throw_length_errorB8ne190102Ev
	.p2align	2
__ZNKSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE20__throw_length_errorB8ne190102Ev: ; @_ZNKSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE20__throw_length_errorB8ne190102Ev
	.cfi_startproc
; %bb.0:
	stp	x29, x30, [sp, #-16]!           ; 16-byte Folded Spill
	.cfi_def_cfa_offset 16
	mov	x29, sp
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
Lloh30:
	adrp	x0, l_.str.9@PAGE
Lloh31:
	add	x0, x0, l_.str.9@PAGEOFF
	bl	__ZNSt3__120__throw_length_errorB8ne190102EPKc
	.loh AdrpAdd	Lloh30, Lloh31
	.cfi_endproc
                                        ; -- End function
	.private_extern	__ZNSt3__120__throw_length_errorB8ne190102EPKc ; -- Begin function _ZNSt3__120__throw_length_errorB8ne190102EPKc
	.globl	__ZNSt3__120__throw_length_errorB8ne190102EPKc
	.weak_def_can_be_hidden	__ZNSt3__120__throw_length_errorB8ne190102EPKc
	.p2align	2
__ZNSt3__120__throw_length_errorB8ne190102EPKc: ; @_ZNSt3__120__throw_length_errorB8ne190102EPKc
Lfunc_begin1:
	.cfi_startproc
	.cfi_personality 155, ___gxx_personality_v0
	.cfi_lsda 16, Lexception1
; %bb.0:
	stp	x20, x19, [sp, #-32]!           ; 16-byte Folded Spill
	.cfi_def_cfa_offset 32
	stp	x29, x30, [sp, #16]             ; 16-byte Folded Spill
	add	x29, sp, #16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	mov	x20, x0
	mov	w0, #16
	bl	___cxa_allocate_exception
	mov	x19, x0
Ltmp11:
	mov	x1, x20
	bl	__ZNSt12length_errorC1B8ne190102EPKc
Ltmp12:
; %bb.1:
Lloh32:
	adrp	x1, __ZTISt12length_error@GOTPAGE
Lloh33:
	ldr	x1, [x1, __ZTISt12length_error@GOTPAGEOFF]
Lloh34:
	adrp	x2, __ZNSt12length_errorD1Ev@GOTPAGE
Lloh35:
	ldr	x2, [x2, __ZNSt12length_errorD1Ev@GOTPAGEOFF]
	mov	x0, x19
	bl	___cxa_throw
LBB8_2:
Ltmp13:
	mov	x20, x0
	mov	x0, x19
	bl	___cxa_free_exception
	mov	x0, x20
	bl	__Unwind_Resume
	.loh AdrpLdrGot	Lloh34, Lloh35
	.loh AdrpLdrGot	Lloh32, Lloh33
Lfunc_end1:
	.cfi_endproc
	.section	__TEXT,__gcc_except_tab
	.p2align	2, 0x0
GCC_except_table8:
Lexception1:
	.byte	255                             ; @LPStart Encoding = omit
	.byte	255                             ; @TType Encoding = omit
	.byte	1                               ; Call site Encoding = uleb128
	.uleb128 Lcst_end1-Lcst_begin1
Lcst_begin1:
	.uleb128 Lfunc_begin1-Lfunc_begin1      ; >> Call Site 1 <<
	.uleb128 Ltmp11-Lfunc_begin1            ;   Call between Lfunc_begin1 and Ltmp11
	.byte	0                               ;     has no landing pad
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp11-Lfunc_begin1            ; >> Call Site 2 <<
	.uleb128 Ltmp12-Ltmp11                  ;   Call between Ltmp11 and Ltmp12
	.uleb128 Ltmp13-Lfunc_begin1            ;     jumps to Ltmp13
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp12-Lfunc_begin1            ; >> Call Site 3 <<
	.uleb128 Lfunc_end1-Ltmp12              ;   Call between Ltmp12 and Lfunc_end1
	.byte	0                               ;     has no landing pad
	.byte	0                               ;   On action: cleanup
Lcst_end1:
	.p2align	2, 0x0
                                        ; -- End function
	.section	__TEXT,__text,regular,pure_instructions
	.private_extern	__ZNSt12length_errorC1B8ne190102EPKc ; -- Begin function _ZNSt12length_errorC1B8ne190102EPKc
	.globl	__ZNSt12length_errorC1B8ne190102EPKc
	.weak_def_can_be_hidden	__ZNSt12length_errorC1B8ne190102EPKc
	.p2align	2
__ZNSt12length_errorC1B8ne190102EPKc:   ; @_ZNSt12length_errorC1B8ne190102EPKc
	.cfi_startproc
; %bb.0:
	stp	x29, x30, [sp, #-16]!           ; 16-byte Folded Spill
	.cfi_def_cfa_offset 16
	mov	x29, sp
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	bl	__ZNSt11logic_errorC2EPKc
Lloh36:
	adrp	x8, __ZTVSt12length_error@GOTPAGE
Lloh37:
	ldr	x8, [x8, __ZTVSt12length_error@GOTPAGEOFF]
	add	x8, x8, #16
	str	x8, [x0]
	ldp	x29, x30, [sp], #16             ; 16-byte Folded Reload
	ret
	.loh AdrpLdrGot	Lloh36, Lloh37
	.cfi_endproc
                                        ; -- End function
	.private_extern	__ZNKSt3__16vectorIhNS_9allocatorIhEEE20__throw_length_errorB8ne190102Ev ; -- Begin function _ZNKSt3__16vectorIhNS_9allocatorIhEEE20__throw_length_errorB8ne190102Ev
	.globl	__ZNKSt3__16vectorIhNS_9allocatorIhEEE20__throw_length_errorB8ne190102Ev
	.weak_def_can_be_hidden	__ZNKSt3__16vectorIhNS_9allocatorIhEEE20__throw_length_errorB8ne190102Ev
	.p2align	2
__ZNKSt3__16vectorIhNS_9allocatorIhEEE20__throw_length_errorB8ne190102Ev: ; @_ZNKSt3__16vectorIhNS_9allocatorIhEEE20__throw_length_errorB8ne190102Ev
	.cfi_startproc
; %bb.0:
	stp	x29, x30, [sp, #-16]!           ; 16-byte Folded Spill
	.cfi_def_cfa_offset 16
	mov	x29, sp
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
Lloh38:
	adrp	x0, l_.str.10@PAGE
Lloh39:
	add	x0, x0, l_.str.10@PAGEOFF
	bl	__ZNSt3__120__throw_length_errorB8ne190102EPKc
	.loh AdrpAdd	Lloh38, Lloh39
	.cfi_endproc
                                        ; -- End function
	.section	__TEXT,__cstring,cstring_literals
l_.str:                                 ; @.str
	.asciz	"Profiling benchmark: %zu bytes, %zu iterations\n"

l_.str.5:                               ; @.str.5
	.asciz	"\nResults for %zu bytes:\n"

l_.str.6:                               ; @.str.6
	.asciz	"  mirror_hash: %.2f ns/hash (%.2f GB/s)\n"

l_.str.7:                               ; @.str.7
	.asciz	"  rapidhash:   %.2f ns/hash (%.2f GB/s)\n"

l_.str.8:                               ; @.str.8
	.asciz	"  speedup:     %.1f%%\n"

l_.str.9:                               ; @.str.9
	.asciz	"basic_string"

l_.str.10:                              ; @.str.10
	.asciz	"vector"

l_str:                                  ; @str
	.asciz	"Use 'sample' command to profile: sample ./profile_benchmark <size> <iters> -f profile.txt\n"

l_str.11:                               ; @str.11
	.asciz	"Warming up..."

l_str.12:                               ; @str.12
	.asciz	"Running mirror_hash benchmark..."

l_str.13:                               ; @str.13
	.asciz	"Running rapidhash benchmark..."

.subsections_via_symbols
