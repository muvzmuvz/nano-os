[bits 32]
section .multiboot
align 4
%define MULTIBOOT_MAGIC 0x1BADB002
%define MULTIBOOT_FLAGS 0x00000003 ; align + mem info
%define MULTIBOOT_CHECK (-(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS))

dd MULTIBOOT_MAGIC
dd MULTIBOOT_FLAGS
dd MULTIBOOT_CHECK

section .text
global multiboot_entry
extern kmain

multiboot_entry:
    cli
    mov esp, stack_top
    push eax        ; multiboot magic
    push ebx        ; multiboot info ptr
    call kmain
.hang: hlt
      jmp .hang

section .bss
align 16
stack_bottom: resb 16384
stack_top:
