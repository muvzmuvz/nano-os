#include <stdint.h>
#include "util.h"

/* Каталог страниц и 4 таблицы по 4К записей (4К*4К = 16МиБ) */
static uint32_t __attribute__((aligned(4096))) pgdir[1024];
static uint32_t __attribute__((aligned(4096))) pt0[1024];
static uint32_t __attribute__((aligned(4096))) pt1[1024];
static uint32_t __attribute__((aligned(4096))) pt2[1024];
static uint32_t __attribute__((aligned(4096))) pt3[1024];

void paging_enable_identity_16mb(void)
{
    /* обнулим каталог */
    for (int i = 0; i < 1024; i++) pgdir[i] = 0;

    /* Заполним 4 таблицы: каждая покрывает 4МиБ диапазон */
    for (int i = 0; i < 1024; i++) {
        pt0[i] = ((uint32_t)(i * 0x1000u)            ) | 0x3; /* P|RW */
        pt1[i] = ((uint32_t)(i * 0x1000u + 0x00400000)) | 0x3;
        pt2[i] = ((uint32_t)(i * 0x1000u + 0x00800000)) | 0x3;
        pt3[i] = ((uint32_t)(i * 0x1000u + 0x00C00000)) | 0x3;
    }

    /* Каталог: первые 4 PDE указывают на наши таблицы */
    pgdir[0] = ((uint32_t)pt0) | 0x3;
    pgdir[1] = ((uint32_t)pt1) | 0x3;
    pgdir[2] = ((uint32_t)pt2) | 0x3;
    pgdir[3] = ((uint32_t)pt3) | 0x3;

    /* Загрузим CR3 и включим PG в CR0 */
    __asm__ __volatile__("mov %0, %%cr3" :: "r"(pgdir) : "memory");
    uint32_t cr0;
    __asm__ __volatile__("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= (1u << 31);
    __asm__ __volatile__("mov %0, %%cr0" :: "r"(cr0) : "memory");
}
