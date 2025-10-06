// fs/ramfs.c
#include <stdint.h>
#include <stddef.h>
#include "util.h"
#include "tty.h"

/* Простая плоская FS: до 64 файлов, имена до 31 симв., буфер под данные аллоцируется. */
#define RAMFS_MAX_FILES 64

typedef struct {
    char     name[32];
    char*    data;
    size_t   len;
    size_t   cap;
    int      used;
} node_t;

static node_t fs[RAMFS_MAX_FILES];
static size_t total_cap = 256*1024; /* для статистики: «воображаемый диск» 256КБ */
extern void* kmalloc(size_t);       /* из kernel/heap.c */

static node_t* find(const char* path){
    if(!path || !*path) return 0;
    /* допускаем "name" и "/name" */
    const char* p = (*path=='/')? path+1 : path;
    for(int i=0;i<RAMFS_MAX_FILES;i++)
        if(fs[i].used && strcmp(fs[i].name,p)==0) return &fs[i];
    return 0;
}
static node_t* create(const char* path){
    const char* p = (*path=='/')? path+1 : path;
    for(int i=0;i<RAMFS_MAX_FILES;i++) if(!fs[i].used){
        memset(fs[i].name,0,32);
        for(int j=0;j<31 && p[j]; j++) fs[i].name[j]=p[j];
        fs[i].data=0; fs[i].len=0; fs[i].cap=0; fs[i].used=1;
        return &fs[i];
    }
    return 0;
}

void ramfs_init(void){
    memset(fs,0,sizeof(fs));
    /* два стартовых файла */
    const char* h =
        "hello from ramfs!\n"
        "you can 'type hello.txt'\n";
    const char* i =
        "nano-os demo files\n"
        "hello.txt\n"
        "info.txt\n";
    ramfs_write("hello.txt", h, strlen(h), 0);
    ramfs_write("info.txt",  i, strlen(i), 0);
}

const char* ramfs_read(const char* path){
    node_t* n = find(path);
    return n? n->data : 0;
}

int ramfs_write(const char* path, const char* s, size_t len, int append){
    node_t* n = find(path);
    if(!n){ n = create(path); if(!n) return -1; }

    size_t need = append? (n->len + len) : len;
    if(need > n->cap){
        size_t newcap = (need + 255) & ~255u; /* кратно 256 */
        char* nd = (char*)kmalloc(newcap);
        if(!nd) return -2;
        if(append && n->data && n->len) memcpy(nd, n->data, n->len);
        n->data = nd; n->cap = newcap;
    }
    if(append){
        memcpy(n->data + n->len, s, len);
        n->len += len;
    } else {
        memcpy(n->data, s, len);
        n->len = len;
    }
    return 0;
}

int ramfs_delete(const char* path){
    node_t* n = find(path);
    if(!n) return -1;
    n->used=0; n->data=0; n->len=0; n->cap=0; n->name[0]=0;
    return 0;
}

int ramfs_rename(const char* oldp, const char* newp){
    node_t* n = find(oldp);
    if(!n) return -1;
    const char* p = (*newp=='/')? newp+1 : newp;
    memset(n->name,0,32);
    for(int j=0;j<31 && p[j]; j++) n->name[j]=p[j];
    return 0;
}

void ramfs_ls(void){
    for(int i=0;i<RAMFS_MAX_FILES;i++)
        if(fs[i].used){ vga_puts(fs[i].name); vga_puts("\n"); }
}

void ramfs_stat(size_t* total,size_t* used,int* files){
    size_t u=0; int f=0;
    for(int i=0;i<RAMFS_MAX_FILES;i++)
        if(fs[i].used){ u += fs[i].cap; f++; }
    if(total) *total = total_cap;
    if(used)  *used  = u;
    if(files) *files = f;
}
