	.text
	.globl main 
main:
	addi sp, sp, -32
e0:
	j mystart
mystart:
	li t0, 1
	li t1, 1
	add t0, t0, t1
	sw t0, 4(sp)
	lw t0, 4(sp)
	bnez t0, if_0
	j then_0
if_0:
	li t0, 1
	sw t0, 0(sp)
	j end
then_0:
	li t0, 0
	sw t0, 0(sp)
	j end
end:
	lw t0, 0(sp)
	sw t0, 8(sp)
	lw a0, 0(sp)
	addi sp, sp, 32
	ret
