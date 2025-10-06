// kernel/heap.c
#include "heap.h"
#include "util.h" // k*_ ф-ции: align4k, kmemset, kmemcpy
#include "tty.h"  // для отладочного вывода, если нужно

struct block_header {
    size_t size;
    struct block_header* next;
    int free;
};

#define HDR_SZ ( (sizeof(struct block_header) + sizeof(void*) - 1) & ~(sizeof(void*) - 1) )

static void* heap_start = 0;
static void* heap_end = 0;
static struct block_header* first_block = 0;
static struct block_header* free_list_head = 0;

void kheap_init(void* start, void* end) {
    heap_start = start;
    heap_end   = end;

    heap_start = (void*)align4k((uint32_t)heap_start);
    heap_end   = (void*)align4k((uint32_t)heap_end);

    first_block        = (struct block_header*)heap_start;
    first_block->size  = (size_t)((uint8_t*)heap_end - (uint8_t*)heap_start) - HDR_SZ;
    first_block->next  = 0;
    first_block->free  = 1;
    free_list_head     = first_block;
}

void* kmalloc(size_t size) {
    if (size == 0) return 0;
    size = (size + sizeof(void*) - 1) & ~(sizeof(void*) - 1);

    struct block_header* current = free_list_head;
    struct block_header* prev = 0;

    while (current) {
        if (current->free && current->size >= size) {
            if (current->size > size + HDR_SZ) {
                struct block_header* new_split = (struct block_header*)((uint8_t*)current + HDR_SZ + size);
                new_split->size = current->size - size - HDR_SZ;
                new_split->free = 1;
                new_split->next = current->next;

                current->size = size;
                current->next = new_split;
            }

            if (prev) prev->next = current->next;
            else      free_list_head = current->next;

            current->free = 0;
            return (void*)((uint8_t*)current + HDR_SZ);
        }
        prev = current;
        current = current->next;
    }

    vga_puts("kmalloc: Out of memory");
    return 0;
}

void* kcalloc(size_t n, size_t sz) {
    size_t bytes = n * sz;
    void* p = kmalloc(bytes);
    if (p) kmemset(p, 0, bytes);
    return p;
}

void* krealloc(void* old, size_t newsize) {
    if (!old) return kmalloc(newsize);
    if (newsize == 0) { kfree(old); return 0; }

    newsize = (newsize + sizeof(void*) - 1) & ~(sizeof(void*) - 1);
    struct block_header* old_hdr = (struct block_header*)((uint8_t*)old - HDR_SZ);

    if (!old_hdr->free) {
        size_t old_size = old_hdr->size;
        if (newsize <= old_size) return old;

        void* new_ptr = kmalloc(newsize);
        if (new_ptr) {
            kmemcpy(new_ptr, old, old_size);
            kfree(old);
            return new_ptr;
        }
        return 0;
    } else {
        vga_puts("krealloc: Attempted to realloc a free block");
        return 0;
    }
}

void kfree(void* p) {
    if (!p) return;
    struct block_header* hdr = (struct block_header*)((uint8_t*)p - HDR_SZ);
    if (hdr->free) { vga_puts("kfree: Double free detected"); return; }
    hdr->free = 1;
    hdr->next = free_list_head;
    free_list_head = hdr;
}
