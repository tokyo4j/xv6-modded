// Long-term locks for processes

#pragma once

#include <xv6/spinlock.h>
#include <xv6/types.h>

struct sleeplock {
  uint locked;        // Is the lock held?
  struct spinlock lk; // spinlock protecting this sleep lock

  // For debugging:
  char *name; // Name of lock.
  int pid;    // Process holding lock
};
