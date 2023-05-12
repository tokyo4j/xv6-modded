#include <xv6/proc.h>
#include <xv6/syscall.h>
#include <xv6/trap.h>
#include <xv6/types.h>

ulong sys_fork(void) { return fork(); }

ulong sys_exit(void) {
  exit();
  return 0; // not reached
}

ulong sys_wait(void) { return wait(); }

ulong sys_kill(void) {
  int pid;

  if (argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

ulong sys_getpid(void) { return myproc()->pid; }

ulong sys_sbrk(void) {
  ulong addr;
  int n;

  if (argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if (growproc(n) < 0)
    return -1;
  return addr;
}

ulong sys_sleep(void) {
  int n;
  uint ticks0;

  if (argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n) {
    if (myproc()->killed) {
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
ulong sys_uptime(void) {
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
