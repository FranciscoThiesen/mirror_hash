	.text
	.file	"asm_test.cpp"
	.globl	_Z26test_rapidhash_nano_8bytesPKvm // -- Begin function _Z26test_rapidhash_nano_8bytesPKvm
	.p2align	2
	.type	_Z26test_rapidhash_nano_8bytesPKvm,@function
_Z26test_rapidhash_nano_8bytesPKvm:     // @_Z26test_rapidhash_nano_8bytesPKvm
	.cfi_startproc
// %bb.0:
	mov	x8, #54435
	mov	x9, #44233
	movk	x8, #54323, lsl #16
	movk	x9, #38446, lsl #16
	movk	x8, #42542, lsl #32
	movk	x9, #19347, lsl #32
	movk	x8, #19251, lsl #48
	movk	x9, #35768, lsl #48
	eor	x8, x1, x8
	mul	x10, x8, x9
	umulh	x8, x8, x9
	ldr	x9, [x0]
	eor	x8, x8, x10
	mov	x10, #44225
	movk	x10, #38446, lsl #16
	eor	x8, x8, x1
	movk	x10, #19347, lsl #32
	eor	x8, x8, x9
	movk	x10, #35768, lsl #48
	eor	x8, x8, #0x8
	orr	x11, x10, #0x8
	eor	x9, x9, x11
	mul	x11, x8, x9
	umulh	x8, x8, x9
	eor	x9, x11, #0xaaaaaaaaaaaaaaaa
	eor	x8, x8, x10
	mul	x10, x8, x9
	umulh	x8, x8, x9
	eor	x0, x8, x10
	ret
.Lfunc_end0:
	.size	_Z26test_rapidhash_nano_8bytesPKvm, .Lfunc_end0-_Z26test_rapidhash_nano_8bytesPKvm
	.cfi_endproc
                                        // -- End function
	.globl	_Z27test_rapidhash_nano_16bytesPKvm // -- Begin function _Z27test_rapidhash_nano_16bytesPKvm
	.p2align	2
	.type	_Z27test_rapidhash_nano_16bytesPKvm,@function
_Z27test_rapidhash_nano_16bytesPKvm:    // @_Z27test_rapidhash_nano_16bytesPKvm
	.cfi_startproc
// %bb.0:
	mov	x8, #54435
	mov	x9, #44233
	movk	x8, #54323, lsl #16
	movk	x9, #38446, lsl #16
	movk	x8, #42542, lsl #32
	movk	x9, #19347, lsl #32
	movk	x8, #19251, lsl #48
	movk	x9, #35768, lsl #48
	eor	x8, x1, x8
	ldp	x12, x11, [x0]
	mul	x10, x8, x9
	umulh	x8, x8, x9
	eor	x8, x8, x10
	eor	x10, x12, x9
	eor	x8, x8, x1
	orr	x9, x9, #0x10
	eor	x8, x8, x11
	eor	x8, x8, #0x10
	mul	x11, x8, x10
	umulh	x8, x8, x10
	eor	x10, x11, #0xaaaaaaaaaaaaaaaa
	eor	x8, x8, x9
	mul	x9, x8, x10
	umulh	x8, x8, x10
	eor	x0, x8, x9
	ret
.Lfunc_end1:
	.size	_Z27test_rapidhash_nano_16bytesPKvm, .Lfunc_end1-_Z27test_rapidhash_nano_16bytesPKvm
	.cfi_endproc
                                        // -- End function
	.globl	main                            // -- Begin function main
	.p2align	2
	.type	main,@function
main:                                   // @main
	.cfi_startproc
// %bb.0:
	stp	x29, x30, [sp, #-32]!           // 16-byte Folded Spill
	str	x19, [sp, #16]                  // 8-byte Folded Spill
	mov	x29, sp
	.cfi_def_cfa w29, 32
	.cfi_offset w19, -16
	.cfi_offset w30, -24
	.cfi_offset w29, -32
	adrp	x19, .L__const.main.data
	mov	x1, xzr
	add	x19, x19, :lo12:.L__const.main.data
	mov	x0, x19
	bl	_Z26test_rapidhash_nano_8bytesPKvm
	str	x0, [x29, #24]
	mov	x0, x19
	mov	x1, xzr
	bl	_Z27test_rapidhash_nano_16bytesPKvm
	str	x0, [x29, #24]
	ldr	x8, [x29, #24]
	ldr	x19, [sp, #16]                  // 8-byte Folded Reload
	and	w0, w8, #0x1
	ldp	x29, x30, [sp], #32             // 16-byte Folded Reload
	ret
.Lfunc_end2:
	.size	main, .Lfunc_end2-main
	.cfi_endproc
                                        // -- End function
	.type	.L__const.main.data,@object     // @__const.main.data
	.section	.rodata.cst16,"aM",@progbits,16
.L__const.main.data:
	.ascii	"\001\002\003\004\005\006\007\b\t\n\013\f\r\016\017\020"
	.size	.L__const.main.data, 16

	.ident	"Ubuntu clang version 14.0.0-1ubuntu1.1"
	.section	".note.GNU-stack","",@progbits
	.addrsig
