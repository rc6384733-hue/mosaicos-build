/* MosaicOS 0.10 - arranque Multiboot + rutinas de interrupción (x86 32 bits) */
.set ALIGN,    1<<0
.set MEMINFO,  1<<1
.set FLAGS,    ALIGN | MEMINFO
.set MAGIC,    0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)

.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

.section .bss
.align 16
stack_bottom:
.skip 16384
stack_top:

.section .text
.global _start
.type _start, @function
_start:
    cli
    mov $stack_top, %esp
    push %ebx
    push %eax
    call kernel_main
1:  hlt
    jmp 1b

/* Excepciones sin código de error */
.irp n, 0,1,2,3,4,5,6,7,9,15,16,18,19,20,22,23,24,25,26,27,28,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47
.global isr\n
isr\n:
    pushl $0
    pushl $\n
    jmp isr_common
.endr

/* Excepciones con código de error */
.irp n, 8,10,11,12,13,14,17,21,29,30
.global isr\n
isr\n:
    pushl $\n
    jmp isr_common
.endr

/* Vector de respaldo para entradas IDT no asignadas. */
.global isr_default
isr_default:
    pushl $0
    pushl $255
    jmp isr_common

isr_common:
    pusha
    cld
    push %esp
    call isr_handler
    add $4, %esp
    popa
    add $8, %esp
    iret

.section .data
.global isr_table
isr_table:
.irp n, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47
.long isr\n
.endr

.section .note.GNU-stack,"",@progbits
