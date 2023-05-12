#include <xv6/console.h>
#include <xv6/misc.h>
#include <xv6/proc.h>
#include <xv6/systbl.h>
#include <xv6/trap.h>
#include <xv6/types.h>

// User code makes a system call with INT T_SYSCALL.
// System call number in %eax.
// Arguments on the stack, from the user call to the C
// library system call function. The saved user %esp points
// to a saved program counter, and then the first argument.

// Fetch the unsigned long at addr
int fetchlong(ulong addr, ulong *ip) {
  struct proc *curproc = myproc();

  if (addr >= curproc->sz || addr + sizeof(ulong) > curproc->sz)
    return -1;
  *ip = *(ulong *)(addr);
  return 0;
}

// Fetch the nul-terminated string at addr from the current process.
// Doesn't actually copy the string - just sets *pp to point at it.
// Returns length of string, not including nul.
int fetchstr(ulong addr, char **pp) {
  char *s, *ep;
  struct proc *curproc = myproc();

  if (addr >= curproc->sz)
    return -1;
  *pp = (char *)addr;
  ep = (char *)curproc->sz;
  for (s = *pp; s < ep; s++) {
    if (*s == 0)
      return s - *pp;
  }
  return -1;
}

// Fetch the nth 64-bit system call argument.
int arglong(int n, ulong *ip) {
  switch (n) {
    case 0:
      *ip = myproc()->tf->rdi;
      return 0;
    case 1:
      *ip = myproc()->tf->rsi;
      return 0;
    case 2:
      *ip = myproc()->tf->rdx;
      return 0;
    case 3:
      *ip = myproc()->tf->rcx;
      return 0;
    case 4:
      *ip = myproc()->tf->r8;
      return 0;
    case 5:
      *ip = myproc()->tf->r9;
      return 0;
    default:
      return fetchlong(myproc()->tf->rsp + sizeof(ulong) * (n + 1), ip);
  }
}

// Fetch the nth 32-bit system call argument.
int argint(int n, int *ip) {
  ulong i;
  if (arglong(n, &i) < 0)
    return -1;
  *ip = (int)i;
  return 0;
}

// Fetch the nth word-sized system call argument as a pointer
// to a block of memory of size bytes.  Check that the pointer
// lies within the process address space.
int argptr(int n, char **pp, int size) {
  ulong i;
  struct proc *curproc = myproc();

  if (arglong(n, &i) < 0)
    return -1;
  if (size < 0 || i >= curproc->sz || i + size > curproc->sz)
    return -1;
  *pp = (char *)i;
  return 0;
}

// Fetch the nth word-sized system call argument as a string pointer.
// Check that the pointer is valid and the string is nul-terminated.
// (There is no shared writable memory, so the string can't change
// between this check and being used by the kernel.)
int argstr(int n, char **pp) {
  ulong addr;
  if (arglong(n, &addr) < 0)
    return -1;
  return fetchstr(addr, pp);
}

extern ulong sys_chdir(void);
extern ulong sys_close(void);
extern ulong sys_dup(void);
extern ulong sys_exec(void);
extern ulong sys_exit(void);
extern ulong sys_fork(void);
extern ulong sys_fstat(void);
extern ulong sys_getpid(void);
extern ulong sys_kill(void);
extern ulong sys_link(void);
extern ulong sys_mkdir(void);
extern ulong sys_mknod(void);
extern ulong sys_open(void);
extern ulong sys_pipe(void);
extern ulong sys_read(void);
extern ulong sys_sbrk(void);
extern ulong sys_sleep(void);
extern ulong sys_unlink(void);
extern ulong sys_wait(void);
extern ulong sys_write(void);
extern ulong sys_uptime(void);

static ulong (*syscalls[])(void) = {
    [SYS_fork] = sys_fork,     [SYS_exit] = sys_exit,
    [SYS_wait] = sys_wait,     [SYS_pipe] = sys_pipe,
    [SYS_read] = sys_read,     [SYS_kill] = sys_kill,
    [SYS_exec] = sys_exec,     [SYS_fstat] = sys_fstat,
    [SYS_chdir] = sys_chdir,   [SYS_dup] = sys_dup,
    [SYS_getpid] = sys_getpid, [SYS_sbrk] = sys_sbrk,
    [SYS_sleep] = sys_sleep,   [SYS_uptime] = sys_uptime,
    [SYS_open] = sys_open,     [SYS_write] = sys_write,
    [SYS_mknod] = sys_mknod,   [SYS_unlink] = sys_unlink,
    [SYS_link] = sys_link,     [SYS_mkdir] = sys_mkdir,
    [SYS_close] = sys_close,
};

void syscall(void) {
  int num;
  struct proc *curproc = myproc();

  num = (int)curproc->tf->rax;
  if (num > 0 && num < NELEM(syscalls) && syscalls[num]) {
    curproc->tf->rax = syscalls[num]();
  } else {
    cprintf("%d %s: unknown sys call %d\n", curproc->pid, curproc->name, num);
    curproc->tf->rax = -1;
  }
}
