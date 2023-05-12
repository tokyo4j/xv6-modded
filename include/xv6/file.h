#pragma once

#include <xv6/fs.h>
#include <xv6/sleeplock.h>
#include <xv6/stat.h>
#include <xv6/types.h>

struct file {
  enum { FD_NONE, FD_PIPE, FD_INODE } type;
  int ref; // reference count
  char readable;
  char writable;
  struct pipe *pipe;
  struct inode *ip;
  uint off;
};

// table mapping major device number to
// device functions
struct devsw {
  int (*read)(struct inode *ip, char *dst, int n);
  int (*write)(struct inode *ip, const char *src, int n);
};

extern struct devsw devsw[];

// major number of console device
#define CONSOLE 1

struct file *filealloc(void);
void fileclose(struct file *f);
struct file *filedup(struct file *f);
void fileinit(void);
int fileread(struct file *f, char *addr, int n);
int filestat(struct file *f, struct stat *st);
int filewrite(struct file *f, char *addr, int n);
