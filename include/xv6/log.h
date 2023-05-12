#pragma once

#include <xv6/bio.h>

void initlog(int dev);
void log_write(struct buf *b);
void begin_op(void);
void end_op(void);