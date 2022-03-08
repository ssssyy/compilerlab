	.text
	.globl main 
main:
	addi sp, sp, -16
	li t0, 1
	li t1, 2
	add t0, t0, t1
	sw t0, 0(sp)
	lw t0, 0(sp)
	li t1, 3
	sub t0, t0, t1
	sw t0, 4(sp)
	lw a0, 4(sp)
	addi sp, sp, 16
	ret
