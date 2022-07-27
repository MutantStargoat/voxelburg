	.text
	.thumb

	.globl fillblock_16byte
fillblock_16byte:
	push {r4-r6}
	mov r3, r1
	mov r4, r1
	mov r5, r1
	mov r6, r1
0:	stmia r0!, {r3, r4, r5, r6}
	sub r2, #1
	bne 0b
	pop {r4-r6}
	bx lr

	.globl get_pc
get_pc:
	mov r0, lr
	bx lr

	.globl get_sp
get_sp:
	mov r0, sp
	bx lr

	.arm
	.extern panic_regs
	.globl get_panic_regs
	.type get_panic_regs, %function
get_panic_regs:
	stmfd sp!, {sp, lr}
	ldr lr, =panic_regs
	stm lr, {r0-r15}
	ldmfd sp!, {r0, lr}
	ldr r0, =panic_regs + 13 * 4
	stm r0, {sp, lr}
	bx lr
