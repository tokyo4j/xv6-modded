// Mutual exclusion spin locks.

#include <xv6/console.h>
#include <xv6/memlayout.h>
#include <xv6/proc.h>
#include <xv6/spinlock.h>
#include <xv6/types.h>
#include <xv6/x86.h>

void initlock(struct spinlock *lk, const char *name) {
  lk->name = name;
  lk->locked = 0;
  lk->cpu = 0;
}

// Acquire the lock.
// Loops (spins) until the lock is acquired.
// Holding a lock for a long time may cause
// other CPUs to waste time spinning to acquire it.
void acquire(struct spinlock *lk) {
  pushcli(); // disable interrupts to avoid deadlock.
  if (holding(lk))
    panic("acquire");

  // The xchg is atomic.
  while (xchg(&lk->locked, 1) != 0)
    ;

  // Tell the C compiler and the processor to not move loads or stores
  // past this point, to ensure that the critical section's memory
  // references happen after the lock is acquired.
  __sync_synchronize();

  // Record info about lock acquisition for debugging.
  lk->cpu = mycpu();
  getcallerpcs(lk->pcs);
}

// Release the lock.
void release(struct spinlock *lk) {
  if (!holding(lk))
    panic("release");

  lk->pcs[0] = 0;
  lk->cpu = 0;

  // Tell the C compiler and the processor to not move loads or stores
  // past this point, to ensure that all the stores in the critical
  // section are visible to other cores before the lock is released.
  // Both the C compiler and the hardware may re-order loads and
  // stores; __sync_synchronize() tells them both not to.
  __sync_synchronize();

  // Release the lock, equivalent to lk->locked = 0.
  // This code can't use a C assignment, since it might
  // not be atomic. A real OS would use C atomics here.
  asm volatile("movl $0, %0" : "+m"(lk->locked) :);

  popcli();
}

// Record the current call stack in pcs[] by following the %rbp chain.
void getcallerpcs(ulong pcs[]) {
  ulong *rbp;
  int i;

  asm("movq %%rbp, %0" : "=m"(rbp));
  rbp = (ulong *)*rbp;

  for (i = 0; i < 10; i++) {
    if (rbp == 0 || rbp < (ulong *)KERNBASE || rbp == (ulong *)-1L)
      break;
    pcs[i] = rbp[1];       // saved %rip
    rbp = (ulong *)rbp[0]; // saved %rbp
  }
  for (; i < 10; i++)
    pcs[i] = 0;
}

// Check whether this cpu is holding the lock.
int holding(struct spinlock *lock) {
  int r;
  pushcli();
  r = lock->locked && lock->cpu == mycpu();
  popcli();
  return r;
}

// Pushcli/popcli are like cli/sti except that they are matched:
// it takes two popcli to undo two pushcli.  Also, if interrupts
// are off, then pushcli, popcli leaves them off.

void pushcli(void) {
  int rflags;

  rflags = readrflags();
  cli();
  if (mycpu()->ncli == 0)
    mycpu()->intena = rflags & FL_IF;
  mycpu()->ncli += 1;
}

void popcli(void) {
  if (readrflags() & FL_IF)
    panic("popcli - interruptible");
  if (--mycpu()->ncli < 0)
    panic("popcli");
  if (mycpu()->ncli == 0 && mycpu()->intena)
    sti();
}
