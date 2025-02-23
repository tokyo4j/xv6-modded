#include <xv6/apic.h>
#include <xv6/console.h>
#include <xv6/ide.h>
#include <xv6/kbd.h>
#include <xv6/proc.h>
#include <xv6/seg.h>
#include <xv6/syscall.h>
#include <xv6/trap.h>
#include <xv6/traptbl.h>
#include <xv6/types.h>
#include <xv6/uart.h>

// Interrupt descriptor table (shared by all CPUs).
static __attribute__((aligned(16))) struct gatedesc idt[256];
extern ulong vectors[]; // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;

void tvinit(void) {
  int i;

  for (i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE << 3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE << 3, vectors[T_SYSCALL], DPL_USER);

  initlock(&tickslock, "time");
}

void idtinit(void) { lidt(idt, sizeof(idt)); }

void trap(struct trapframe *tf) {
  if (tf->trapno == T_SYSCALL) {
    if (myproc()->killed)
      exit();
    myproc()->tf = tf;
    syscall();
    if (myproc()->killed)
      exit();
    return;
  }

  switch (tf->trapno) {
    case T_IRQ0 + IRQ_TIMER:
      if (cpuid() == 0) {
        acquire(&tickslock);
        ticks++;
        wakeup(&ticks);
        release(&tickslock);
      }
      lapiceoi();
      break;
    case T_IRQ0 + IRQ_IDE:
      ideintr();
      lapiceoi();
      break;
    case T_IRQ0 + IRQ_IDE + 1:
      // Bochs generates spurious IDE1 interrupts.
      break;
    case T_IRQ0 + IRQ_KBD:
      kbdintr();
      lapiceoi();
      break;
    case T_IRQ0 + IRQ_COM1:
      uartintr();
      lapiceoi();
      break;
    case T_IRQ0 + 7:
    case T_IRQ0 + IRQ_SPURIOUS:
      cprintf("cpu%d: spurious interrupt at %x:%x\n", cpuid(), tf->cs, tf->rip);
      lapiceoi();
      break;

    default:
      if (myproc() == 0 || (tf->cs & 3) == 0) {
        // In kernel, it must be our mistake.
        cprintf("unexpected trap %d from cpu %d rip %x (cr2=0x%x)\n",
                tf->trapno, cpuid(), tf->rbp, rcr2());
        panic("trap");
      }
      // In user space, assume process misbehaved.
      cprintf("pid %d %s: trap %d err %d on cpu %d "
              "rip 0x%x addr 0x%x--kill proc\n",
              myproc()->pid, myproc()->name, tf->trapno, tf->err, cpuid(),
              tf->rip, rcr2());
      myproc()->killed = 1;
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running
  // until it gets to the regular system call return.)
  if (myproc() && myproc()->killed && (tf->cs & 3) == DPL_USER)
    exit();

  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  if (myproc() && myproc()->state == RUNNING &&
      tf->trapno == T_IRQ0 + IRQ_TIMER)
    yield();

  // Check if the process has been killed since we yielded
  if (myproc() && myproc()->killed && (tf->cs & 3) == DPL_USER)
    exit();
}
