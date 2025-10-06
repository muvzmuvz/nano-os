; kernel/gdt_reload.asm
[bits 32]
section .text
global gdt_reload

; После lgdt нужно обновить CS (через far jmp) и остальные сегменты.
gdt_reload:
    cli
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ; дальний прыжок меняет CS на 0x08
    jmp 0x08:flush_label

flush_label:
    sti
    ret
