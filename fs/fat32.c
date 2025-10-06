// fs/fat32.c  — VFS-обёртка и простые shell-команды
#include <stdint.h>
#include "../kernel/tty.h"
#include "../kernel/util.h"
#include "fat32_core.h"

static fat32_t g_fs;

int fat_is_mounted(void){ return g_fs.valid; }

const char* fat_vfs_read(const char* path){
    static char g_static_buf[4096];
    if (!g_fs.valid) return 0;
    uint32_t got=0;
    if (fat_read_file(&g_fs, path, g_static_buf, sizeof(g_static_buf)-1, &got)==0){
        g_static_buf[got]=0;
        return g_static_buf;
    }
    return 0;
}

void fat_vfs_ls(void){
    if (!g_fs.valid){ vga_puts("FAT not mounted\n"); return; }
    fat_ls_root(&g_fs);
}

int fat_mount_shell(uint8_t drive){
    int rc = fat_mount(drive, &g_fs);
    if (rc==0) vga_puts("FAT32 mounted.\n");
    else       vga_puts("FAT mount failed\n");
    return rc;
}
