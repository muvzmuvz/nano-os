#pragma once
#include <stdint.h>

/* Мини-состояние FAT32 (только то, что реально используем) */
typedef struct {
    uint8_t  drive;          // 0/1 — какая шина в ATA PIO
    uint8_t  valid;          // 1, если смонтировано
    uint32_t part_lba_begin; // LBA начала раздела

    /* BPB */
    uint16_t byps;           // BytesPerSec
    uint8_t  spc;            // SecPerClus
    uint16_t rsvd;           // RsvdSecCnt
    uint8_t  nfats;          // NumFATs
    uint32_t fatsz;          // FATSz32
    uint32_t rootclus;       // RootClus

    /* производные */
    uint32_t first_fat_lba;  // LBA FAT0
    uint32_t first_data_lba; // LBA начала области данных (кластер 2)
} fat32_t;

/* API ядра FAT32 */
int  fat_mount(uint8_t drive, fat32_t* out);
int  fat_read_file(fat32_t* fs, const char* path, void* buf, uint32_t buf_sz, uint32_t* out_read);
void fat_ls_root(fat32_t* fs);
