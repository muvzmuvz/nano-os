// fs/vfs.h
#pragma once
#include <stddef.h>
#include <stdint.h>

/* Базовый VFS для RAMFS уже есть — оставляем существующие API */
void   vfs_init(void);
void   vfs_ls(void);
const char* vfs_read(const char* path);
int    vfs_write(const char* path, const char* data, size_t len, int overwrite);
int    vfs_delete(const char* path);
int    vfs_rename(const char* a, const char* b);
void   vfs_stat(size_t* total, size_t* used, int* files);

/* Добавляем примитивные mount-точки по префиксу (например, "/fat") */
typedef struct {
    const char* (*read)(const char* relpath);  // вернуть статический буфер (демо)
    void        (*ls)(void);                   // распечатать список (демо)
} vfs_ops_t;

int vfs_mount(const char* prefix, const vfs_ops_t* ops);  // 0=ok
