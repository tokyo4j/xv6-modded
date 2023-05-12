#pragma once

#include <xv6/bio.h>

void ideinit(void);
void ideintr(void);
void iderw(struct buf *b);