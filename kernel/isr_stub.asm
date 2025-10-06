[bits 32]
section .text

; C-обработчик:
; void isr_handler_c(uint32_t intno, uint32_t* regs);
extern isr_handler_c

; -------------------------------
; Общий шаблон без кода ошибки
; -------------------------------
%macro ISR_NOERR 1
global isr%1
isr%1:
    pusha                   ; EAX,ECX,EDX,EBX,ESP,EBP,ESI,EDI
    push ds
    push es
    push fs
    push gs
    mov ax, 0x10            ; ядровые сегменты (селектор code/data = 0x08/0x10)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov eax, esp            ; regs* указывает на область, сохранённую pusha/seg
    push eax                ; arg2: regs*
    push dword %1           ; arg1: intno
    call isr_handler_c
    add esp, 8
    pop gs
    pop fs
    pop es
    pop ds
    popa
    iretd
%endmacro

; -------------------------------
; Шаблон для исключений с кодом ошибки
; CPU кладёт error code перед возвратным кадром.
; Мы его не используем, но обязаны убрать со стека перед IRETD.
; -------------------------------
%macro ISR_ERR 1
global isr%1
isr%1:
    pusha
    push ds
    push es
    push fs
    push gs
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov eax, esp
    push eax                ; arg2: regs*
    push dword %1           ; arg1: intno
    call isr_handler_c
    add esp, 8
    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 4              ; <<< снять error code, который положил CPU
    iretd
%endmacro

; --------
; ISRs 0..31
; Исключения с error code: 8, 10, 11, 12, 13, 14, 17
; --------
ISR_NOERR 0     ; #DE
ISR_NOERR 1     ; #DB
ISR_NOERR 2     ; NMI
ISR_NOERR 3     ; #BP
ISR_NOERR 4     ; #OF
ISR_NOERR 5     ; #BR
ISR_NOERR 6     ; #UD
ISR_NOERR 7     ; #NM
ISR_ERR   8     ; #DF (error code = 0)
ISR_NOERR 9     ; Coprocessor Segment Overrun (legacy)
ISR_ERR   10    ; #TS
ISR_ERR   11    ; #NP
ISR_ERR   12    ; #SS
ISR_ERR   13    ; #GP
ISR_ERR   14    ; #PF
ISR_NOERR 15
ISR_NOERR 16    ; #MF
ISR_ERR   17    ; #AC
ISR_NOERR 18    ; #MC
ISR_NOERR 19    ; #XM
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31

; --------
; IRQ0..1 (после ремапа: 32,33)
; --------
global irq0, irq1
irq0:
    pusha
    push ds
    push es
    push fs
    push gs
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov eax, esp
    push eax                ; regs*
    push dword 32           ; intno
    call isr_handler_c
    add esp, 8
    pop gs
    pop fs
    pop es
    pop ds
    popa
    iretd

irq1:
    pusha
    push ds
    push es
    push fs
    push gs
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov eax, esp
    push eax
    push dword 33
    call isr_handler_c
    add esp, 8
    pop gs
    pop fs
    pop es
    pop ds
    popa
    iretd

; --------
; int 0x80 — системные вызовы (без error code)
; --------
global int80
int80:
    pusha
    push ds
    push es
    push fs
    push gs
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov eax, esp
    push eax                ; regs*
    push dword 0x80         ; intno
    call isr_handler_c
    add esp, 8
    pop gs
    pop fs
    pop es
    pop ds
    popa
    iretd


isr_stub.asm