#include <xv6/apic.h>
#include <xv6/bio.h>
#include <xv6/console.h>
#include <xv6/file.h>
#include <xv6/ide.h>
#include <xv6/kalloc.h>
#include <xv6/mb2.h>
#include <xv6/memlayout.h>
#include <xv6/misc.h>
#include <xv6/mmu.h>
#include <xv6/pic.h>
#include <xv6/proc.h>
#include <xv6/seg.h>
#include <xv6/string.h>
#include <xv6/trap.h>
#include <xv6/types.h>
#include <xv6/uart.h>
#include <xv6/vm.h>

static void startothers(void);
static void mpmain(void) __attribute__((noreturn));
extern char end[]; // first address after kernel loaded from ELF file

__attribute__((__aligned__(PGSIZE))) pte_t entry_pml4[NR_PTE];
__attribute__((__aligned__(PGSIZE))) pte_t entry_pml3[NR_PTE];

// Bootstrap processor starts running C code here.
// Allocate a real stack and switch to it, first
// doing some setup required for memory allocator to work.
void main(void) {
  mb2_init();    // framebuffer, memory map, ACPI
  kinit1();      // page allocator
  kvmalloc();    // kernel page table
  lapicinit();   // interrupt controller
  seginit();     // segment descriptors
  picinit();     // disable pic
  ioapicinit();  // another interrupt controller
  consoleinit(); // console hardware
  uartinit();    // serial port
  pinit();       // process table
  tvinit();      // trap vectors
  binit();       // buffer cache
  fileinit();    // file table
  ideinit();     // disk
  kinit2();      // must come after startothers()
  userinit();    // first user process
  startothers(); // start other processorsc
  mpmain();      // finish this processor's setup
}

// Other CPUs jump here from entryother.S.
static void mpenter(void) {
  switchkvm();
  seginit();
  lapicinit();
  mpmain();
}

// Common CPU setup code.
static void mpmain(void) {
  cprintf("cpu%d: starting %d\n", cpuid(), cpuid());
  idtinit();                    // load idt register
  xchg(&(mycpu()->started), 1); // tell startothers() we're up
  scheduler();                  // start running processes
}

// Start the non-boot (AP) processors.
static void startothers(void) {
  uchar *code;
  struct cpu *c;
  char *stack;

  // Write entry code to unused memory at 0x7000.
  // The linker has placed the image of entryother.S in
  // _binary_entryother_start.
  code = (uchar *)P2V(0x7000UL);
  memmove(code, _binary_kernel_bin_entryother_start,
          (uint)(ulong)_binary_kernel_bin_entryother_size);

  for (c = cpus; c < cpus + ncpu; c++) {
    if (c == mycpu()) // We've started already.
      continue;

    // Tell entryother.S what stack to use, where to enter, and what
    // pgdir to use. We cannot use kpgdir yet, because the AP processor
    // is running in low  memory, so we use entrypgdir for the APs too.
    stack = kalloc();
    *((ulong *)code - 1) = (ulong)(stack + KSTACKSIZE);
    *((ulong *)code - 2) = (ulong)mpenter;
    *((ulong *)code - 3) = V2P((ulong)entry_pml4);

    lapicstartap(c->apicid, (uint)V2P((ulong)code));

    // wait for cpu to finish mpmain()
    while (c->started == 0)
      ;
  }
}
