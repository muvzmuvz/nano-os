#pragma once
#include <stdint.h>
#include <stddef.h>

/* выравнивание до 4К */
static inline uint32_t align4k(uint32_t x) { return (x + 0xFFFu) & ~0xFFFu; }

/* безопасная копия строки с обрезкой и терминированием */
void k_strlcpy(char* dst, const char* src, int n);

/* строковые */
size_t kstrlen(const char* s);
int    kstrcmp(const char* a, const char* b);
int    kstrncmp(const char* a, const char* b, size_t n);

/* память */
void*  kmemcpy(void* dst, const void* src, size_t n);
void*  kmemset(void* dst, int v, size_t n);

/* числа */
int    k_atoi(const char* s);

/* мелочи */
static inline int isdigit_c(int c){ return c>='0' && c<='9'; }
