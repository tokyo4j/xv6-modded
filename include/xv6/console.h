#pragma once

void consoleinit(void);
void cprintf(const char *fmt, ...);
void consoleintr(int (*getc)(void));
void panic(const char *s) __attribute__((noreturn));

#define BACKSPACE 0x100
