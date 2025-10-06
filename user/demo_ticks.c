#include <stdint.h>
#include "tty.h"
extern volatile uint64_t jiffies;

void user_ticks(){
    uint64_t last=0;
    while(1){
        if(jiffies!=last){ last=jiffies; vga_puts("."); }
        __asm__ __volatile__("hlt");
    }
}
