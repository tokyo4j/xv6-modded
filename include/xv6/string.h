#pragma once

#include <stdarg.h>
#include <xv6/types.h>

int memcmp(const void *v1, const void *v2, uint n);
void *memmove(void *dst, const void *src, uint n);
void *memset(void *dst, int c, uint n);
void *memcpy_fast(void *dst, const void *src, uint n);
char *safestrcpy(char *s, const char *t, int n);
int strlen(const char *s);
int strncmp(const char *p, const char *q, uint n);
char *strncpy(char *s, const char *t, int n);

// sprintf.c
int vsprintf(char *dst, const char *fmt, va_list arg);
void sprintf(char *dst, const char *fmt, ...);