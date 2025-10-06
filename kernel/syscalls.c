#include <stdint.h>
#include "tty.h"

/* Таблица системных вызовов: прототип */
typedef uint32_t (*sys_t)(uint32_t a, uint32_t b, uint32_t c, uint32_t d);

/* --- Реализации syscalls --- */

/* sys_write(no=0): пишет len байт из ptr на экран, возвращает len */
static uint32_t sys_write(uint32_t ptr, uint32_t len, uint32_t unused1, uint32_t unused2) {
    (void)unused1; (void)unused2;
    const char* s = (const char*)ptr;
    for (uint32_t i = 0; i < len; i++) vga_putchar(s[i]);
    return len;
}

/* sys_time(no=1): возвращает младшие 32 бита тиков PIT */
extern volatile uint64_t jiffies;
static uint32_t sys_time(uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
    (void)a; (void)b; (void)c; (void)d;
    return (uint32_t)jiffies;
}

/* Таблица */
static sys_t sys_table[] = {
    sys_write, /* 0 */
    sys_time   /* 1 */
};
#define SYS_MAX ((uint32_t)(sizeof(sys_table)/sizeof(sys_table[0])))

/* Диспетчер: вызывайте из обработчика int 0x80
   no = номер вызова, a..d = аргументы */
uint32_t sys_dispatch(uint32_t no, uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
    if (no >= SYS_MAX) return (uint32_t)-1;
    return sys_table[no](a, b, c, d);
}
