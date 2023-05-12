#pragma once

#include <xv6/spinlock.h>
#include <xv6/types.h>

// Gate descriptors for interrupts and traps
struct gatedesc {
  ushort off_15_0; // low 16 bits of offset in segment
  ushort cs;       // code segment selector
  uint ist : 3;    // Interrupt Stack Table offset
  uint rsv1 : 5;
  uint type : 4;    // type(STS_{IG32,TG32})
  uint s : 1;       // must be 0 (system)
  uint dpl : 2;     // descriptor(meaning new) privilege level
  uint p : 1;       // Present
  ushort off_31_16; // middle 16 bits of offset in segment
  uint off_63_32;   // high 32 bits of offset in segment
  uint rsv2;
};

#define STS_IG32 0xE // 32-bit Interrupt Gate
#define STS_TG32 0xF // 32-bit Trap Gate

// Set up a normal interrupt/trap gate descriptor.
// - istrap: 1 for a trap (= exception) gate, 0 for an interrupt gate.
//   interrupt gate clears FL_IF, trap gate leaves FL_IF alone
// - sel: Code segment selector for interrupt/trap handler
// - off: Offset in code segment for interrupt/trap handler
// - dpl: Descriptor Privilege Level -
//        the privilege level required for software to invoke
//        this interrupt/trap gate explicitly using an int instruction.
#define SETGATE(gate, istrap, sel, off, d) \
  { \
    (gate).off_15_0 = (ulong)(off)&0xffff; \
    (gate).cs = (sel); \
    (gate).ist = 0; \
    (gate).rsv1 = 0; \
    (gate).type = (istrap) ? STS_TG32 : STS_IG32; \
    (gate).s = 0; \
    (gate).dpl = (d); \
    (gate).p = 1; \
    (gate).off_31_16 = ((ulong)(off) >> 16) & 0xffff; \
    (gate).off_63_32 = ((ulong)(off) >> 32) & 0xffffffff; \
  }

//  Layout of the trap frame built on the stack by the
//  hardware and by trapasm.S, and passed to trap().
struct trapframe {
  // rest of trap frame
  ushort gs;
  ushort padding0;
  uint padding1;
  ushort fs;
  ushort padding2;
  uint padding3;
  ushort es;
  ushort padding4;
  uint padding5;
  ushort ds;
  ushort padding6;
  uint padding7;

  ulong r9;
  ulong r8;
  ulong rcx;
  ulong rdx;
  ulong rsi;
  ulong rdi;

  ulong rax;

  ulong r15;
  ulong r14;
  ulong r13;
  ulong r12;
  ulong rbp;
  ulong rbx;

  uint trapno;
  uint padding8;

  // below here defined by x86 hardware
  ulong err;
  ulong rip;
  ushort cs;
  ushort padding9;
  uint padding10;
  ulong rflags;

  // below here only when crossing rings, such as from user to kernel
  ulong rsp;
  ushort ss;
  ushort padding11;
  uint padding12;
};

void idtinit(void);
void tvinit(void);

extern uint ticks;
extern struct spinlock tickslock;
