// kernel/gdt.c
#include <stdint.h>
#include <stddef.h>

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  gran;
    uint8_t  base_hi;
} __attribute__((packed));

struct gdt_ptr_t {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct gdt_entry gdt[3];
struct gdt_ptr_t gdt_ptr;        // нужен lgdt (extern в ASM)

extern void gdt_load(void);      // из kernel/gdt_idt.asm
extern void gdt_reload(void);    // новый ASM: перезагрузка CS/DS/ES/FS/GS/SS

static void gdt_set(int i, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran)
{
    gdt[i].limit_low = (uint16_t)(limit & 0xFFFF);
    gdt[i].base_low  = (uint16_t)(base & 0xFFFF);
    gdt[i].base_mid  = (uint8_t)((base >> 16) & 0xFF);
    gdt[i].access    = access;
    gdt[i].gran      = (uint8_t)(((limit >> 16) & 0x0F) | (gran & 0xF0));
    gdt[i].base_hi   = (uint8_t)((base >> 24) & 0xFF);
}

void gdt_init(void)
{
    // null
    gdt_set(0, 0, 0, 0, 0);
    // code 0x08: base=0, limit=4ГБ, 32-бит, гранулярность 4К
    gdt_set(1, 0, 0x000FFFFF, 0x9A, 0xCF);
    // data 0x10: RW
    gdt_set(2, 0, 0x000FFFFF, 0x92, 0xCF);

    gdt_ptr.limit = sizeof(gdt) - 1;
    gdt_ptr.base  = (uint32_t)gdt;

    gdt_load();    // lgdt [gdt_ptr]
    gdt_reload();  // ДАЛЬНИЙ ПРЫЖОК на CS=0x08 и DS/ES/FS/GS/SS=0x10
}
