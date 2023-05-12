#pragma once

#include <xv6/types.h>

#define KMAP_MAX_SIZE 32
struct kmap {
  ulong phys_start;
  ulong phys_end;
  ulong perm;
  int heap;
};
extern struct kmap kmap[KMAP_MAX_SIZE];

void kmap_add(ulong pa_start, ulong pa_end, ulong perm, int heap);
void *kalloc(void);
void kfree(void *va);
void kinit1(void);
void kinit2(void);
