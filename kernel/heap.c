// kernel/heap.c
#include <stdint.h>
#include <stddef.h>
#include "util.h"

/* Куча в замапленном диапазоне (paging: 0..16MiB)
   Возьмём 4MiB с 0x00400000 по 0x007FFFFF. */
#define HEAP_BASE 0x00400000u
#define HEAP_SIZE 0x00400000u

static uint8_t* brk  = (uint8_t*)HEAP_BASE;
static uint8_t* endh = (uint8_t*)(HEAP_BASE + HEAP_SIZE);

static inline uint32_t align4k(uint32_t x){ return (x + 0xFFFu) & ~0xFFFu; }

void* kmalloc(size_t n){
    if (n == 0) return 0;
    /* пусть будет 16-байтное выравнивание для простоты,
       крупные блоки округлим до страницы, чтобы не фрагментить сильно */
    uint32_t need = (uint32_t)n;
    if (need >= 4096) need = align4k(need);
    else              need = (need + 15u) & ~15u;

    if (brk + need > endh) return 0;
    void* p = brk;
    brk += need;
    memset(p, 0, need);
    return p;
}

void* kcalloc(size_t n, size_t sz){
    size_t bytes = n * sz;
    return kmalloc(bytes);
}

void* krealloc(void* old, size_t newsize){
    if (!old) return kmalloc(newsize);
    void* p = kmalloc(newsize);
    // без информации о старом размере просто не копируем; для демо достаточно
    return p;
}

void kfree(void* p){ (void)p; /* бамп-аллокатор: free игнорируем */ }
