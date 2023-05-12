#pragma once

#include <xv6/types.h>

#define FB_DEPTH 4 // in bytes

void fb_init(ulong fb_pa, int w, int h, int depth, int pitch);
void fb_putc(int c);
void fb_setup_double_buffering(void);
void fb_commit(void);
