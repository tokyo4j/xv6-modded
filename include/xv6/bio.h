#pragma once

#include <xv6/sleeplock.h>
#include <xv6/types.h>

#define BSIZE 512 // block size

void binit(void);
struct buf *bread(uint dev, uint blockno);
void brelse(struct buf *b);
void bwrite(struct buf *b);

struct buf {
  int flags;
  uint dev;
  uint blockno;
  struct sleeplock lock;
  uint refcnt;
  struct buf *prev; // LRU cache list
  struct buf *next;
  struct buf *qnext; // disk queue
  uchar data[BSIZE];
};

#define B_VALID 0x2 // buffer has been read from disk
#define B_DIRTY 0x4 // buffer needs to be written to disk
