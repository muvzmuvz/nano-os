// kernel/keyboard.c
#include <stdint.h>

/* Порты контроллера клавиатуры */
#define KBD_DATA  0x60
#define KBD_STAT  0x64

static inline uint8_t inb(uint16_t p){ uint8_t v; __asm__ __volatile__("inb %1,%0":"=a"(v):"Nd"(p)); return v; }

/* public keycodes (совместимы с ASCII там, где возможно) */
enum {
    KB_NONE = -1,
    KB_ENTER = '\n',
    KB_BACKSPACE = 8,
    KB_LEFT = 256, KB_RIGHT, KB_UP, KB_DOWN, KB_HOME, KB_END, KB_DEL
};

/* Таблицы scancode set 1: обычная и с Shift */
static const char keymap[128] = {
/*00*/ 0,  27,'1','2','3','4','5','6','7','8','9','0','-','=', '\b',
/*0F*/ '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',  0,
/*1F*/ 'a','s','d','f','g','h','j','k','l',';','\'','`',  0, '\\',
/*2F*/ 'z','x','c','v','b','n','m',',','.','/',  0,   '*', 0,  ' ',
/*39*/  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   0,    0,  0,   0,
/*49*/  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   0,    0,  0,   0,
/*59*/  0,  0,  0,  0,  0,  0,  0,  0
};

static const char keymap_shift[128] = {
/*00*/ 0,  27,'!','@','#','$','%','^','&','*','(',')','_','+', '\b',
/*0F*/ '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',  0,
/*1F*/ 'A','S','D','F','G','H','J','K','L',':','"','~',  0,  '|',
/*2F*/ 'Z','X','C','V','B','N','M','<','>','?',  0,   '*', 0,  ' ',
/*39*/  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   0,    0,  0,   0,
/*49*/  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   0,    0,  0,   0,
/*59*/  0,  0,  0,  0,  0,  0,  0,  0
};

/* Состояние модификаторов */
static volatile int shift = 0;
static volatile int caps  = 0;
/* E0-префикс (клавиши курсора/Ins/Delete...) */
static volatile int e0_prefix = 0;

/* Очередь «последняя клавиша» */
static volatile int last_key = KB_NONE;

/* --- API --- */
void kb_init(void){
    /* Сбросим мусор из буфера контроллера */
    for(int i=0;i<16;i++){
        uint8_t st = inb(KBD_STAT);
        if(st & 1) (void)inb(KBD_DATA);
    }
    shift = 0; caps = 0; e0_prefix = 0; last_key = KB_NONE;
}

void kb_on_irq(void){
    uint8_t sc = inb(KBD_DATA);

    if (sc == 0xE0) { e0_prefix = 1; return; }   /* расширенный код */
    if (sc & 0x80) {                              /* break-code */
        uint8_t make = sc & 0x7F;
        if (!e0_prefix) {
            if (make==0x2A || make==0x36) shift = 0;  /* L/R Shift release */
        }
        e0_prefix = 0;
        return;
    }

    if (e0_prefix) {
        e0_prefix = 0;
        /* Стрелки/дом/енд/делит из набора E0 */
        switch (sc) {
            case 0x4B: last_key = KB_LEFT;  return;
            case 0x4D: last_key = KB_RIGHT; return;
            case 0x48: last_key = KB_UP;    return;
            case 0x50: last_key = KB_DOWN;  return;
            case 0x47: last_key = KB_HOME;  return;
            case 0x4F: last_key = KB_END;   return;
            case 0x53: last_key = KB_DEL;   return;
            default: return;
        }
    }

    /* Make codes обычные */
    if (sc == 0x2A || sc == 0x36) { shift = 1; return; }       /* L/R Shift press */
    if (sc == 0x3A) { caps ^= 1; return; }                     /* Caps toggle */

    char c = (shift ? keymap_shift[sc] : keymap[sc]);
    if (!shift && caps && c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
    if (c) {
        if (c == '\n') last_key = KB_ENTER;
        else           last_key = (int)c;
    }
}

int kb_getkey(void){
    int k = last_key;
    last_key = KB_NONE;
    return k;
}
