// kernel/keyboard.h
#pragma once
#ifndef KB_TAB
#define KB_TAB 9   /* ASCII '\t' */
#endif
/* публичные коды клавиш (ASCII там, где возможно) */
enum {
    KB_NONE = -1,
    KB_ENTER = '\n',
    KB_BACKSPACE = 8,
    KB_LEFT = 256, KB_RIGHT, KB_UP, KB_DOWN, KB_HOME, KB_END, KB_DEL
};

/* API клавиатуры */
void kb_init(void);
int  kb_getkey(void);
