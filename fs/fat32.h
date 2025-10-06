// fs/fat32.h
#pragma once
#include <stdint.h>
#include <stddef.h>
#include "fs/vfs.h"

typedef struct {
    uint8_t  valid;
    uint8_t  drive;      // 0 = первый IDE диск
    uint32_t part_lba;   // старт LBA раздела
    uint32_t bps;        // bytes per sector
    uint32_t spc;        // sectors per cluster
    uint32_t rsvd;       // reserved sectors
    uint32_t nfat;       // количество FAT
    uint32_t fatsz;      // секторов на FAT
    uint32_t root_clus;  // кластер корня (обычно 2)
    uint32_t data_start; // LBA начала области данных (кластер 2)
    uint32_t fat_start;  // LBA FAT
} fat32_t;

/* Низкоуровневый драйвер (реализован в твоём fs/fat32.c) */
int  fat_mount(uint8_t drive, fat32_t* out);  // 0=ok
void fat_ls_root(const fat32_t* fs);
int  fat_read_file(const fat32_t* fs, const char* path,
                   char* out, size_t out_sz, size_t* out_len);

/* Доп. помощники/обвязка для шелла и VFS */
int  fat_is_mounted(void);
int  fat_mount_shell(uint8_t drive);
const char* fat_vfs_read(const char* rel); // читает в статический буфер (демо)
void fat_vfs_ls(void);

/* ops для vfs_mount("/fat", fat32_vfs_ops()) */
const vfs_ops_t* fat32_vfs_ops(void);
