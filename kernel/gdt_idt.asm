[bits 32]
section .text
global gdt_load, idt_load
extern gdt_ptr, idt_ptr

gdt_load:
    lgdt [gdt_ptr]
    ret

idt_load:
    lidt [idt_ptr]
    ret
