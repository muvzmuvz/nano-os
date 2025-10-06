// fs/fat32_vfs.c — тонкий адаптер только для vfs_ops
#include <stddef.h>
#include <stdint.h>
#include "fs/vfs.h"

/* Реальные реализации живут в fs/fat32.c */
extern const char* fat_vfs_read(const char* rel);
extern void        fat_vfs_ls(void);

static const vfs_ops_t ops = {
    .read = fat_vfs_read,
    .ls   = fat_vfs_ls
};

const vfs_ops_t* fat32_vfs_ops(void){ return &ops; }
