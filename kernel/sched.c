// kernel/sched.c
#include <stdint.h>
#include "tty.h"
#include "util.h"

#define MAX_THREADS 16
typedef struct ctx_t { uint32_t esp, ebp, ebx, esi, edi, eip; } ctx_t;
typedef enum { UNUSED, READY, RUNNING } tstate;

typedef struct thread {
    ctx_t ctx;
    uint8_t *kstack;
    tstate st;
    char name[16];
} thread_t;

static thread_t th[MAX_THREADS];
static int current = -1;

extern void ctx_switch(ctx_t* old, ctx_t* new);

void sched_init(){
    kmemset(th, 0, sizeof(th));
}

int thread_create(void (*entry)(void), const char* name){
    for(int i=0;i<MAX_THREADS;i++){
        if(th[i].st==UNUSED){
            th[i].kstack = (uint8_t*)0x00200000 + i*0x4000;
            uint32_t *sp = (uint32_t*)(th[i].kstack + 0x4000);
            *(--sp) = (uint32_t)entry;
            th[i].ctx.eip = (uint32_t)entry;
            th[i].ctx.esp = (uint32_t)sp;
            th[i].st = READY;
            kmemset(th[i].name, 0, 16);
            for(int j=0;j<15&&name[j];j++) th[i].name[j]=name[j];
            return i;
        }
    }
    return -1;
}

void sched_tick(){
    int start = current;
    for(int i=1;i<=MAX_THREADS;i++){
        int n = (start+i)%MAX_THREADS;
        if(th[n].st==READY){
            int prev=current, next=n;
            if(prev==next){ th[next].st=RUNNING; return; }
            th[next].st=RUNNING;
            current=next;
            if(prev>=0){
                th[prev].st=READY;
                ctx_switch(&th[prev].ctx, &th[next].ctx);
            } else {
                ctx_switch(&(ctx_t){0}, &th[next].ctx);
            }
            return;
        }
    }
}

void sched_start(){
    sched_tick();
    while(1){ __asm__ __volatile__("hlt"); }
}
