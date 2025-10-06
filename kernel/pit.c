#include <stdint.h>
#include "tty.h"

static inline void outb(uint16_t p, uint8_t v){ __asm__ __volatile__("outb %0,%1"::"a"(v),"Nd"(p)); }

volatile uint64_t jiffies=0;

void pit_init(){
    uint16_t div=11932; // ~100Hz
    outb(0x43,0x36);
    outb(0x40,div&0xFF);
    outb(0x40,div>>8);
}

extern void sched_tick();

void pit_on_tick(){
    jiffies++;
    sched_tick();
}
