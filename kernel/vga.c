// kernel/vga.c
#include <stdint.h>
#include "util.h"

#define VGA_MEM   ((uint16_t*)0xB8000)
#define VGA_W     80
#define VGA_H     25

/* Логический курсор и цвет */
static uint8_t cx = 0, cy = 0;
static uint8_t color = 0x0F;

/* ===== Аппаратный курсор (будем выключать) ===== */
static inline void hw_setcursor(uint16_t pos){
    uint8_t hi = (pos >> 8) & 0xFF;
    uint8_t lo =  pos       & 0xFF;
    __asm__ __volatile__(
        "movb $0x0E, %%al; outb %%al, $0x3D4;"
        "movb %0,    %%al; outb %%al, $0x3D5;"
        "movb $0x0F, %%al; outb %%al, $0x3D4;"
        "movb %1,    %%al; outb %%al, $0x3D5;"
        :: "r"(hi), "r"(lo) : "al"
    );
}
static inline void hw_cursor_enable(int on){
    /* CRTC index 0x0A (Cursor Start), bit 5 = 1 -> disable */
    uint8_t val;
    __asm__ __volatile__(
        "movb $0x0A, %%al; outb %%al, $0x3D4; inb $0x3D5, %%al;"
        "movb %%al, %0;"
        : "=r"(val) :: "al"
    );
    if (on) val &= ~(1<<5); else val |= (1<<5);
    __asm__ __volatile__(
        "movb $0x0A, %%al; outb %%al, $0x3D4;"
        "movb %0,    %%al; outb %%al, $0x3D5;"
        :: "r"(val) : "al"
    );
}

/* ===== Прокрутка ===== */
static void scroll_up(void){
    for (int r = 0; r < VGA_H - 1; ++r)
        for (int x = 0; x < VGA_W; ++x)
            VGA_MEM[r*VGA_W + x] = VGA_MEM[(r+1)*VGA_W + x];
    for (int x = 0; x < VGA_W; ++x)
        VGA_MEM[(VGA_H-1)*VGA_W + x] = (uint16_t)(' ') | ((uint16_t)color << 8);
}

/* ===== Низкоуровневый вывод ===== */
static void putc_raw(char ch){
    if (ch == '\n') {
        cx = 0;
        if (++cy >= VGA_H) { cy = VGA_H - 1; scroll_up(); }
        return;
    }
    VGA_MEM[cy*VGA_W + cx] = (uint16_t)ch | ((uint16_t)color << 8);
    if (++cx >= VGA_W) {
        cx = 0;
        if (++cy >= VGA_H) { cy = VGA_H - 1; scroll_up(); }
    }
}

/* ===== Публичное API печати ===== */
void vga_clear(void){
    for (int i = 0; i < VGA_W*VGA_H; ++i)
        VGA_MEM[i] = (uint16_t)(' ') | ((uint16_t)color << 8);
    cx = 0; cy = 0;
}
void vga_setcolor(uint8_t c){ color = c; }
void vga_putchar(char c){ putc_raw(c); }
void vga_puts(const char* s){ while (*s) putc_raw(*s++); }

void vga_puthex(uint32_t x){
    static const char* h = "0123456789ABCDEF";
    for (int i = 7; i >= 0; --i) putc_raw(h[(x >> (i*4)) & 0xF]);
}
void vga_putdec(uint32_t x){
    char buf[11]; int i = 0;
    if (x == 0){ vga_putchar('0'); return; }
    while (x){ buf[i++] = (char)('0' + (x % 10)); x /= 10; }
    while (i--) vga_putchar(buf[i]);
}

/* ===== ЕДИНЫЙ логический курсор ===== */
uint16_t vga_getpos(void){ return (uint16_t)cy * VGA_W + cx; }
void     vga_setcursor(uint16_t pos){ /* совместимость, просто синхронизируем */
    cy = (uint8_t)(pos / VGA_W);
    cx = (uint8_t)(pos % VGA_W);
    if (cy >= VGA_H) cy = VGA_H - 1;
    if (cx >= VGA_W) cx = VGA_W - 1;
}
void     vga_move_to(uint16_t pos){ vga_setcursor(pos); }

/* ===== Software-курсор (инверсия атрибутов клетки) ===== */
static uint16_t swc_prev_pos = 0xFFFF;
static uint16_t swc_prev_cell;

void vga_cursor_on(void){
    hw_cursor_enable(0); /* отключаем HW-курсор */
}
void vga_cursor_off(void){
    /* убрать SW-курсор, если был */
    if (swc_prev_pos != 0xFFFF){
        VGA_MEM[swc_prev_pos] = swc_prev_cell;
        swc_prev_pos = 0xFFFF;
    }
}
void vga_cursor_move(uint16_t pos){
    /* снять прошлую подсветку */
    if (swc_prev_pos != 0xFFFF){
        VGA_MEM[swc_prev_pos] = swc_prev_cell;
    }
    /* запомнить клетку и инвертировать атрибуты (XOR 0x70) */
    swc_prev_pos  = pos;
    swc_prev_cell = VGA_MEM[pos];
    uint16_t ch   = swc_prev_cell & 0x00FF;
    uint16_t attr = (swc_prev_cell & 0xFF00) ^ 0x7000;
    VGA_MEM[pos]  = ch | attr;

    /* синхронизировать логический курсор для последующего вывода */
    vga_move_to(pos);
}
