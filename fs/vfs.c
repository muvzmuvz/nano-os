// fs/vfs.c
#include <stdint.h>
#include "tty.h"
#include "fs/vfs.h"

/* ==== RAMFS хук (твои реальные функции) ==== */
extern const char* ramfs_read (const char* path);
extern int         ramfs_write(const char* path, const char* data, size_t len, int overwrite);
extern int         ramfs_delete(const char* path);
extern int         ramfs_rename(const char* a, const char* b);
extern void        ramfs_stat  (size_t* total, size_t* used, int* files);
extern void        ramfs_ls    (void);

/* ==== Таблица монтирований ==== */
typedef struct { const char* prefix; const vfs_ops_t* ops; } mount_t;
#define MAX_MOUNTS 4
static mount_t mounts[MAX_MOUNTS];
static int mounts_cnt = 0;

static int starts_with_prefix(const char* s, const char* pfx, const char** rest){
    const char* s0 = s;
    while (*pfx && *s && *s == *pfx){ s++; pfx++; }
    if (*pfx == 0 && (*s == 0 || *s == '/')) { if (rest){ if (*s=='/') s++; *rest = s; } return 1; }
    (void)s0;
    return 0;
}

int vfs_mount(const char* prefix, const vfs_ops_t* ops){
    if (!prefix || !ops) return -1;
    if (mounts_cnt >= MAX_MOUNTS) return -1;
    mounts[mounts_cnt].prefix = prefix;
    mounts[mounts_cnt].ops    = ops;
    mounts_cnt++;
    return 0;
}

/* ==== Базовые операции ==== */
void vfs_init(void){
    /* RAMFS уже инициализируется в своём модуле — ничего не делаем тут */
}

static const vfs_ops_t* match_mount(const char* path, const char** rel){
    for (int i=0;i<mounts_cnt;i++){
        if (starts_with_prefix(path, mounts[i].prefix, rel)) return mounts[i].ops;
    }
    return 0;
}

const char* vfs_read(const char* path){
    const char* rel = 0;
    const vfs_ops_t* m = match_mount(path, &rel);
    if (m && m->read){
        const char* s = m->read(rel ? rel : "");
        if (s) return s;
    }
    /* По-умолчанию — RAMFS */
    return ramfs_read(path);
}

int vfs_write(const char* path, const char* data, size_t len, int overwrite){
    /* Пишем пока только в RAMFS */
    (void)path; (void)data; (void)len; (void)overwrite;
    return ramfs_write(path, data, len, overwrite);
}

int vfs_delete(const char* path){
    return ramfs_delete(path);
}

int vfs_rename(const char* a, const char* b){
    return ramfs_rename(a,b);
}

void vfs_stat(size_t* total, size_t* used, int* files){
    ramfs_stat(total, used, files);
}

void vfs_ls(void){
    /* Сначала RAMFS (корень), затем смонтированные префиксы */
    ramfs_ls();
    for (int i=0;i<mounts_cnt;i++){
        vga_puts("\n");
        vga_puts(mounts[i].prefix);
        vga_puts(":\n");
        if (mounts[i].ops && mounts[i].ops->ls) mounts[i].ops->ls();
    }
}
