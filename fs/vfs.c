// fs/vfs.c
#include "vfs.h"

/* реализация на RAMFS */
void       ramfs_init(void);
const char* ramfs_read(const char* path);
int        ramfs_write(const char* path, const char* s, size_t len, int append);
int        ramfs_delete(const char* path);
int        ramfs_rename(const char* oldp, const char* newp);
void       ramfs_ls(void);
void       ramfs_stat(size_t* total, size_t* used, int* files);

void vfs_init(void){ ramfs_init(); }
const char* vfs_read(const char* p){ return ramfs_read(p); }
int  vfs_write(const char* p,const char* s,size_t n,int a){ return ramfs_write(p,s,n,a); }
int  vfs_delete(const char* p){ return ramfs_delete(p); }
int  vfs_rename(const char* a,const char* b){ return ramfs_rename(a,b); }
void vfs_ls(void){ ramfs_ls(); }
void vfs_stat(size_t* t,size_t* u,int* f){ ramfs_stat(t,u,f); }
