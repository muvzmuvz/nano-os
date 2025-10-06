[bits 32]
section .text
global ctx_switch
; void ctx_switch(ctx_t* old, ctx_t* new)
; ctx_t: struct { uint32_t esp, ebp, ebx, esi, edi, eip; }
ctx_switch:
    ; save old
    mov eax, [esp+4]
    mov [eax+0], esp
    mov [eax+4], ebp
    mov [eax+8], ebx
    mov [eax+12], esi
    mov [eax+16], edi
    call .get_eip
.get_eip:
    pop edx
    mov [eax+20], edx

    ; load new
    mov eax, [esp+8]
    mov esp, [eax+0]
    mov ebp, [eax+4]
    mov ebx, [eax+8]
    mov esi, [eax+12]
    mov edi, [eax+16]
    jmp dword [eax+20]
