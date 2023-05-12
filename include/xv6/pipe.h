#pragma once

#include <xv6/spinlock.h>
#include <xv6/types.h>

#define PIPESIZE 512
struct pipe {
  struct spinlock lock;
  char data[PIPESIZE];
  uint nread;    // number of bytes read
  uint nwrite;   // number of bytes written
  int readopen;  // read fd is still open
  int writeopen; // write fd is still open
};

int pipealloc(struct file **f0, struct file **f1);
void pipeclose(struct pipe *p, int writable);
int piperead(struct pipe *p, char *addr, int n);
int pipewrite(struct pipe *p, const char *addr, int n);
