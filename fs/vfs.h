// fs/vfs.h
#pragma once
#include <stdint.h>
#include <stddef.h>

void       vfs_init(void);

/* базовые операции */
const char* vfs_read(const char* path);                        // nullptr если нет
int        vfs_write(const char* path, const char* s, size_t len, int append); // 0=ok
int        vfs_delete(const char* path);                       // 0=ok
int        vfs_rename(const char* oldp, const char* newp);     // 0=ok

/* служебное */
void       vfs_ls(void);
void       vfs_stat(size_t* total, size_t* used, int* files);
