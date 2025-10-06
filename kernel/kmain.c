// kernel/kmain.c
#include <stdint.h>
#include <string.h>      // strlen, memcpy (OK в freestanding, но без вызовов strncpy)
#include "tty.h"
#include "util.h"
#include "keyboard.h"
#include "fs/vfs.h"

/* локальный безопасный копирующий хелпер (без libc) */
static void k_strlcpy(char* dst, const char* src, int n){
    if (!dst || n<=0) return;
    int i=0;
    while (i < n-1 && src && src[i]) { dst[i] = src[i]; ++i; }
    dst[i] = 0;
}

/* ---- Инициализация подсистем ---- */
void gdt_init(void);
void paging_enable_identity_16mb(void);
void idt_init(void);
void pic_init(void);
void pit_init(void);
void kb_init(void);

/* ---- Планировщик/треды ---- */
void  sched_init(void);
int   thread_create(void (*entry)(void), const char* name);
void  sched_start(void);

/* ---- Демонстрационный тред ---- */
void user_ticks(void);

/* ================== Конфигурация шелла ================== */
#define SHELL_BUFSZ      128
#define HIST_MAX         16
#define NAME_MAX         64
#define MATCH_MAX        32

static const char* const builtins[] = {
    "help","dir","ls","type","copy","del","ren","cls","clear",
    "echo","mem","ver","ticks","halt", 0
};

/* ===== История команд ===== */
static char history[HIST_MAX][SHELL_BUFSZ];
static int  hist_cnt = 0;
static int  hist_head = 0;
static int  hist_view = -1;

static void hist_push(const char* s){
    if (!s || !*s) return;
    if (hist_cnt > 0) {
        int last = (hist_head + HIST_MAX - 1) % HIST_MAX;
        if (strcmp(history[last], s) == 0) return;
    }
    k_strlcpy(history[hist_head], s, SHELL_BUFSZ);            // <-- было strncpy
    hist_head = (hist_head + 1) % HIST_MAX;
    if (hist_cnt < HIST_MAX) hist_cnt++;
}

static const char* hist_get(int idx_from_oldest){
    if (idx_from_oldest < 0 || idx_from_oldest >= hist_cnt) return 0;
    int oldest = (hist_head - hist_cnt + HIST_MAX) % HIST_MAX;
    int real   = (oldest + idx_from_oldest) % HIST_MAX;
    return history[real];
}

/* ===== Перерисовка хвоста строки ===== */
static void redraw_from(const char* buf, int len, int from, uint16_t startpos, int clear_one){
    vga_cursor_off();
    vga_move_to(startpos + from);
    for (int i = from; i < len; ++i) vga_putchar(buf[i]);
    if (clear_one) vga_putchar(' ');
    vga_cursor_move(startpos + from);
}

/* ====== помощники строки ====== */
static int is_space(int c){ return c==' ' || c=='\t'; }

static int token_start(const char* buf, int len, int cur){
    (void)len;
    int i = cur;
    while (i>0 && !is_space(buf[i-1])) --i;
    return i;
}

static void prompt(uint16_t* p_startpos){
    vga_puts("> ");
    *p_startpos = vga_getpos();
}

static int collect_matches(const char* pref, char out[][NAME_MAX], int max, int want_cmds, int want_files){
    int n = 0;
    if (want_cmds) {
        for (int i=0; builtins[i]; ++i){
            if (strncmp(builtins[i], pref, strlen(pref))==0 && n<max){
                k_strlcpy(out[n], builtins[i], NAME_MAX);     // <-- было strncpy
                n++;
            }
        }
    }
#ifdef HAVE_VFS_LIST_MATCHES
    if (want_files) n += vfs_list_matches(pref, &out[n], max-n);
#else
    (void)want_files;(void)pref;(void)out;(void)max;
#endif
    return n;
}

static void insert_str(char* buf, int* p_len, int* p_cur, int bufsz, const char* s){
    int cur=*p_cur, len=*p_len;
    int add = (int)strlen(s);
    if (len + add >= bufsz) add = bufsz-1-len;
    if (add<=0) return;
    for (int i=len-1;i>=cur;--i) buf[i+add]=buf[i];
    memcpy(buf+cur, s, add);
    len += add; cur += add;
    buf[len]=0;
    *p_len=len; *p_cur=cur;
}

/* ===================== DOS-подобный шелл ===================== */
static void shell(void) {
    vga_puts("\nType 'help'.\n");
    uint16_t startpos; prompt(&startpos);

    char buf[SHELL_BUFSZ] = {0};
    int  len = 0, cur = 0;

    vga_cursor_move(startpos);

    for (;;) {
        int k = kb_getkey();
        if (k == KB_NONE) { __asm__ __volatile__("hlt"); continue; }

        if (k == KB_ENTER) {
            vga_cursor_off();
            vga_move_to(startpos + len);
            vga_putchar('\n');
            buf[len] = 0;

            hist_push(buf);
            hist_view = -1;

            if (buf[0] == 0) {
                /* пусто */
            }
            else if (strcmp(buf,"help")==0) {
                vga_puts("help, dir, type <f>, copy <a> <b>, del <f>, ren <a> <b>, cls, echo <t>, mem, ver, ticks, halt\n");
            }
            else if (strcmp(buf,"dir")==0 || strcmp(buf,"ls")==0) {
                vfs_ls();
            }
            else if (strncmp(buf,"type ",5)==0) {
                const char* p=buf+5; while(*p==' ') p++;
                const char* s=vfs_read(p);
                vga_puts(s? s : "file not found\n");
            }
            else if (strncmp(buf,"copy ",5)==0) {
                char a[64]={0}, b[64]={0};
                const char* p=buf+5; while(*p==' ') p++;
                int i=0; while(*p && *p!=' ' && i<63){ a[i++]=*p++; }
                while(*p==' ') p++;
                i=0; while(*p && i<63){ b[i++]=*p++; }
                const char* s=vfs_read(a);
                if(!s) vga_puts("source not found\n");
                else if(vfs_write(b,s,strlen(s),0)==0) vga_puts("copied\n");
                else vga_puts("copy failed\n");
            }
            else if (strncmp(buf,"del ",4)==0) {
                const char* p=buf+4; while(*p==' ') p++;
                vga_puts(vfs_delete(p)==0 ? "deleted\n" : "no such file\n");
            }
            else if (strncmp(buf,"ren ",4)==0) {
                char a[64]={0}, b[64]={0};
                const char* p=buf+4; while(*p==' ') p++;
                int i=0; while(*p && *p!=' ' && i<63){ a[i++]=*p++; }
                while(*p==' ') p++;
                i=0; while(*p && i<63){ b[i++]=*p++; }
                vga_puts(vfs_rename(a,b)==0 ? "renamed\n" : "rename failed\n");
            }
            else if (strcmp(buf,"cls")==0 || strcmp(buf,"clear")==0) {
                vga_clear();
            }
            else if (strncmp(buf,"echo ",5)==0) {
                const char* p=buf+5; while(*p==' ') p++; vga_puts(p); vga_puts("\n");
            }
            else if (strcmp(buf,"mem")==0) {
                size_t tot,used; int files; vfs_stat(&tot,&used,&files);
                vga_puts("RAMFS total: "); vga_putdec((uint32_t)tot);
                vga_puts(" used: ");       vga_putdec((uint32_t)used);
                vga_puts(" files: ");      vga_putdec((uint32_t)files);
                vga_puts("\n");
            }
            else if (strcmp(buf,"ver")==0) {
                vga_puts("nano-os DOS-like shell 0.4\n");
            }
            else if (strcmp(buf,"ticks")==0) {
                thread_create(user_ticks,"ticks");
            }
            else if (strcmp(buf,"halt")==0) {
                vga_puts("halt\n"); while(1){ __asm__ __volatile__("hlt"); }
            }
            else {
                vga_puts("?\n");
            }

            prompt(&startpos);
            buf[0]=0; len=0; cur=0;
            vga_cursor_move(startpos);
            continue;
        }

        /* --- история (↑/↓) --- */
        if (k == KB_UP || k == KB_DOWN){
            if (hist_cnt == 0) continue;

            if (hist_view < 0) hist_view = hist_cnt;
            if (k == KB_UP)   { if (hist_view > 0) hist_view--; }
            else              { if (hist_view < hist_cnt) hist_view++; }

            vga_cursor_off();
            vga_move_to(startpos);
            for (int i=0;i<len;i++) vga_putchar(' ');
            vga_move_to(startpos);

            if (hist_view == hist_cnt) {
                buf[0]=0; len=cur=0;
            } else {
                const char* h = hist_get(hist_view);
                k_strlcpy(buf, h, SHELL_BUFSZ);             // <-- было strncpy
                len = cur = (int)strlen(buf);
                for (int i=0;i<len;i++) vga_putchar(buf[i]);
            }
            vga_cursor_move(startpos + cur);
            continue;
        }

        /* --- Tab-completion --- */
#ifdef KB_TAB
        if (k == KB_TAB || k == '\t')
#else
        if (k == '\t')
#endif
        {
            int t0 = token_start(buf, len, cur);
            int on_cmd = (t0 == 0);
            char pref[NAME_MAX]={0};
            int preflen = cur - t0;
            if (preflen > NAME_MAX-1) preflen = NAME_MAX-1;
            memcpy(pref, buf + t0, preflen); pref[preflen]=0;

            char cand[MATCH_MAX][NAME_MAX];
            int n = collect_matches(pref, cand, MATCH_MAX, on_cmd, !on_cmd);

            if (n == 0){
#ifndef HAVE_VFS_LIST_MATCHES
                if (!on_cmd){ vga_putchar('\n'); vfs_ls(); prompt(&startpos); }
#endif
                vga_cursor_off();
                vga_move_to(startpos);
                for (int i=0;i<len;i++) vga_putchar(buf[i]);
                vga_cursor_move(startpos + cur);
                continue;
            }
            if (n == 1){
                const char* full = cand[0];
                const char* suf  = full + preflen;
                insert_str(buf, &len, &cur, SHELL_BUFSZ, suf);
                redraw_from(buf, len, t0, startpos, 0);
                vga_cursor_move(startpos + cur);
                continue;
            }

            vga_putchar('\n');
            for (int i=0;i<n;i++){
                vga_puts(cand[i]);
                if (i+1<n) vga_puts("  ");
            }
#ifndef HAVE_VFS_LIST_MATCHES
            if (!on_cmd) { vga_puts("  "); vga_puts("(dir for all files)"); }
#endif
            vga_putchar('\n');
            prompt(&startpos);
            vga_cursor_off();
            vga_move_to(startpos);
            for (int i=0;i<len;i++) vga_putchar(buf[i]);
            vga_cursor_move(startpos + cur);
            continue;
        }

        /* --- навигация и редактирование --- */
        switch (k) {
            case KB_LEFT:
                if (cur>0) { cur--; vga_cursor_move(startpos + cur); }
                break;
            case KB_RIGHT:
                if (cur<len){ cur++; vga_cursor_move(startpos + cur); }
                break;
            case KB_HOME:
                cur = 0; vga_cursor_move(startpos + cur);
                break;
            case KB_END:
                cur = len; vga_cursor_move(startpos + cur);
                break;
            case KB_BACKSPACE:
                if (cur>0) {
                    cur--;
                    for (int i=cur; i<len-1; ++i) buf[i]=buf[i+1];
                    len--;
                    redraw_from(buf, len, cur, startpos, 1);
                    vga_cursor_move(startpos + cur);
                }
                break;
            case KB_DEL:
                if (cur<len) {
                    for (int i=cur; i<len-1; ++i) buf[i]=buf[i+1];
                    len--;
                    redraw_from(buf, len, cur, startpos, 1);
                    vga_cursor_move(startpos + cur);
                }
                break;
            default:
                if (k>=32 && k<127 && len < SHELL_BUFSZ-1) {
                    vga_cursor_off();
                    for (int i=len; i>cur; --i) buf[i]=buf[i-1];
                    buf[cur] = (char)k;
                    len++; cur++;
                    vga_move_to(startpos + (cur-1));
                    for (int i = cur-1; i < len; ++i) vga_putchar(buf[i]);
                    vga_cursor_move(startpos + cur);
                }
                break;
        }
    }
}

void kmain(uint32_t multiboot_magic, uint32_t multiboot_info) {
    (void)multiboot_magic; (void)multiboot_info;

    vga_setcolor(0x0F);
    vga_clear();
    vga_puts("nano-os: hello!\n");
    vga_cursor_on();

    gdt_init();
    paging_enable_identity_16mb();
    idt_init(); pic_init(); pit_init(); kb_init();

    vfs_init();

    __asm__ __volatile__("sti");

    sched_init();
    thread_create(shell, "shell");
    sched_start();

    while (1) { __asm__ __volatile__("hlt"); }
}
