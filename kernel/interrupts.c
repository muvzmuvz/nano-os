#include <stdint.h>
#include "tty.h"
#include "util.h"

struct idt_entry {
    uint16_t off1;
    uint16_t sel;
    uint8_t  zero;
    uint8_t  type;
    uint16_t off2;
} __attribute__((packed));

struct idt_ptr_t {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct idt_entry idt[256];
struct idt_ptr_t idt_ptr;

extern void idt_load(void);

extern void isr0();  extern void isr1();  extern void isr2();  extern void isr3();
extern void isr4();  extern void isr5();  extern void isr6();  extern void isr7();
extern void isr8();  extern void isr9();  extern void isr10(); extern void isr11();
extern void isr12(); extern void isr13(); extern void isr14(); extern void isr15();
extern void isr16(); extern void isr17(); extern void isr18(); extern void isr19();
extern void isr20(); extern void isr21(); extern void isr22(); extern void isr23();
extern void isr24(); extern void isr25(); extern void isr26(); extern void isr27();
extern void isr28(); extern void isr29(); extern void isr30(); extern void isr31();

extern void irq0(); extern void irq1();
extern void int80();

extern uint32_t sys_dispatch(uint32_t no, uint32_t a, uint32_t b, uint32_t c, uint32_t d);

static inline void outb(uint16_t p, uint8_t v){ __asm__ __volatile__("outb %0,%1"::"a"(v),"Nd"(p)); }
static inline uint8_t inb(uint16_t p){ uint8_t r; __asm__ __volatile__("inb %1,%0":"=a"(r):"Nd"(p)); return r; }

static void set_gate(int n, uint32_t off){
    idt[n].off1 = off & 0xFFFF;
    idt[n].sel  = 0x08;
    idt[n].zero = 0;
    idt[n].type = 0x8E;
    idt[n].off2 = (off >> 16) & 0xFFFF;
}

void idt_init(void){
    kmemset(idt, 0, sizeof(idt));

    set_gate(0, (uint32_t)isr0);  set_gate(1, (uint32_t)isr1);
    set_gate(2, (uint32_t)isr2);  set_gate(3, (uint32_t)isr3);
    set_gate(4, (uint32_t)isr4);  set_gate(5, (uint32_t)isr5);
    set_gate(6, (uint32_t)isr6);  set_gate(7, (uint32_t)isr7);
    set_gate(8, (uint32_t)isr8);  set_gate(9, (uint32_t)isr9);
    set_gate(10,(uint32_t)isr10); set_gate(11,(uint32_t)isr11);
    set_gate(12,(uint32_t)isr12); set_gate(13,(uint32_t)isr13);
    set_gate(14,(uint32_t)isr14); set_gate(15,(uint32_t)isr15);
    set_gate(16,(uint32_t)isr16); set_gate(17,(uint32_t)isr17);
    set_gate(18,(uint32_t)isr18); set_gate(19,(uint32_t)isr19);
    set_gate(20,(uint32_t)isr20); set_gate(21,(uint32_t)isr21);
    set_gate(22,(uint32_t)isr22); set_gate(23,(uint32_t)isr23);
    set_gate(24,(uint32_t)isr24); set_gate(25,(uint32_t)isr25);
    set_gate(26,(uint32_t)isr26); set_gate(27,(uint32_t)isr27);
    set_gate(28,(uint32_t)isr28); set_gate(29,(uint32_t)isr29);
    set_gate(30,(uint32_t)isr30); set_gate(31,(uint32_t)isr31);

    set_gate(32,(uint32_t)irq0);
    set_gate(33,(uint32_t)irq1);

    set_gate(0x80,(uint32_t)int80);

    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base  = (uint32_t)idt;
    idt_load();
}

/* PIC */
void pic_init(void){
    outb(0x20,0x11); outb(0xA0,0x11);
    outb(0x21,0x20); outb(0xA1,0x28);
    outb(0x21,0x04); outb(0xA1,0x02);
    outb(0x21,0x01); outb(0xA1,0x01);
    outb(0x21,0xFC); outb(0xA1,0xFF);
}
static inline void pic_eoi_master(){ outb(0x20,0x20); }

extern void pit_on_tick(void);
extern void kb_on_irq(void);

void isr_handler_c(uint32_t intno, uint32_t* regs){
    if (intno == 32) { pit_on_tick(); pic_eoi_master(); return; }
    if (intno == 33) { kb_on_irq();   pic_eoi_master(); return; }

    if (intno == 0x80) {
        uint32_t no = regs[0];
        uint32_t a  = regs[3];
        uint32_t b  = regs[1];
        uint32_t c  = regs[2];
        uint32_t ret = sys_dispatch(no, a, b, c, 0);
        regs[0] = ret;
        return;
    }
    if (intno == 14) {
        uint32_t cr2;
        __asm__ __volatile__("mov %%cr2, %0" : "=r"(cr2));
        vga_puts("\n#PF @"); vga_puthex(cr2);
        while (1) { __asm__ __volatile__("cli; hlt"); }
    }

    vga_puts("\n#EXC "); vga_puthex(intno);
}
