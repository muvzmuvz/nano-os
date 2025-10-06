// kernel/heap.h
#ifndef HEAP_H
#define HEAP_H

#include <stdint.h>
#include <stddef.h>

// Инициализация кучи. Вызывается один раз при старте ядра.
void kheap_init(void* start, void* end);

// Выделение памяти
void* kmalloc(size_t size);
void* kcalloc(size_t n, size_t sz);
void* krealloc(void* old, size_t newsize);

// Освобождение памяти
void kfree(void* p);

#endif // HEAP_H