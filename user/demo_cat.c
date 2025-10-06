#include "tty.h"
#include "util.h"
const char* vfs_read(const char*);

void user_cat(){
    const char* s=vfs_read("/hello.txt");
    if(!s){ vga_puts("no file\n"); return; }
    vga_puts(s);
    for(volatile int i=0;i<10000000;i++){}
}
