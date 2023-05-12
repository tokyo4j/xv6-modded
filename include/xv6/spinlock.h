// Mutual exclusion lock.

#pragma once

#include <xv6/types.h>

struct spinlock {
  uint locked; // Is the lock held?

  // For debugging:
  const char *name; // Name of lock.
  struct cpu *cpu;  // The cpu holding the lock.
  ulong pcs[10];    // The call stack (an array of program counters)
                    // that locked the lock.
};

void acquire(struct spinlock *lk);
void getcallerpcs(ulong pcs[]);
int holding(struct spinlock *lk);
void initlock(struct spinlock *lk, const char *name);
void release(struct spinlock *lk);
void pushcli(void);
void popcli(void);