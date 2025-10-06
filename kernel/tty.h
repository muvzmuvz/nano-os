// kernel/tty.h
#pragma once
#include <stdint.h>
#include <stddef.h>

void     vga_clear(void);
void     vga_setcolor(uint8_t color);
void     vga_putchar(char c);
void     vga_puts(const char* s);
void     vga_puthex(uint32_t x);
void     vga_putdec(uint32_t x);

uint16_t vga_getpos(void);
void     vga_setcursor(uint16_t pos);
void     vga_move_to(uint16_t pos);

void     vga_cursor_on(void);
void     vga_cursor_off(void);
void     vga_cursor_move(uint16_t pos);
